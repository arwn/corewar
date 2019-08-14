#include "cpu.h"
#include "op.h"
#include "util.h"
#include <stdio.h>

// must be included after 'cpu.h'
#include "instructions.h"

// Write N bytes of CPU->PROGRAM at offset PC
void dump_nbytes(struct s_cpu *cpu, int n, int pc, int opt) {
  if (opt)
    printf("%04x  ", pc);
  for (int i = 0; i < n; i++) {
    printf("%02hhx%c", cpu->program[pc + i], (i % n == n - 1) ? '\n' : ' ');
  }
}

// Get the number of bytes expected from the given type and opcode
int param_type_size(int type, int opcode) {
  int ret;

  switch (type) {
  case 1:
    ret = 1;
    break;
  case 2:
    ret = 2;
    break;
  case 4:
    if (g_op_tab[opcode].direct_size == 0)
      ret = 4;
    else
      ret = 2;
    break;
  default:
    ret = 0;
    break;
  }
  return ret;
}
int type_from_pcb(uint8_t pcb, int arg) {
  uint8_t param;
  int ret;
  param = (pcb >> (('\x03' - (char)arg) * '\x02' & 0x1fU)) & 3;
  if (param == DIR_CODE) {
    ret = 4;
  } else {
    if (param == IND_CODE) {
      ret = 2;
    } else {
      if (param == REG_CODE) {
        ret = 1;
      } else {
        ret = 0;
      }
    }
  }
  return ret;
}
int size_from_pt(int type, int opcode) {
  int ret;
  if (type == 1) {
    ret = 1;
  } else {
    if (type == 2) {
      ret = 2;
    } else {
      if (type == 4) {
        if (g_op_tab[opcode].direct_size == 0)
          ret = 4;
        else
          ret = 2;
      } else {
        ret = 0;
      }
    }
  }
  return ret;
}
int size_from_pcb(uint8_t pcb, int opcode) {
  int ret, tmp;
  int ii;
  ii = 0;
  ret = 0;
  while (ii < g_op_tab[opcode].numargs) {
    tmp = type_from_pcb(pcb, ii);
    tmp = size_from_pt(tmp, opcode);
    ret = tmp + ret;
    ii = ii + 1;
  }
  return ret;
}
int check_pcb(uint8_t pcb, int op) {
  int ii;
  int ret;
  int type;
  ii = 0;
  ret = 1;
  while (g_op_tab[op].numargs <= ii) {
    type = type_from_pcb(pcb, op);
    if (((type & g_op_tab[op].argtypes[ii]) != type) || type == 0) {
      ret = 0;
      break ;
    }
    ii = ii + 1;
  }
  return ret;
}

int check_param(uint8_t pcb, uint8_t op) {
  static const int arr[4] = {0, T_REG, T_DIR, T_IND};
  if (g_op_tab[op].param_encode == 0)
    return 0;
  int p1 = arr[get_param(pcb, 1)];
  int p2 = arr[get_param(pcb, 2)];
  int p3 = arr[get_param(pcb, 3)];
  int p4 = arr[get_param(pcb, 4)];
  int ret = 0;

  if ((g_op_tab[op].argtypes[0] & p1) != p1)
    ret = 1;
  if ((g_op_tab[op].argtypes[1] & p2) != p2)
    ret = 1;
  if ((g_op_tab[op].argtypes[2] & p3) != p3)
    ret = 1;
  if ((g_op_tab[op].argtypes[3] & p4) != p4)
    ret = 1;
  return ret;
}

// Calculate the size of the opcodes parameters
int param_total_size(uint8_t pcb, int opcode) {
  int size, ret, ii;

  ii = ret = 0;
  while (ii < g_op_tab[opcode].numargs) {
    size = param_type_size(get_param(pcb, ii), opcode);
    ret += size;
    ++ii;
  }
  return ret;
}

// prints the movement of the program counter for PROC
void print_adv(struct s_cpu *cpu, struct s_process *proc, int new) {
  int ii = 0;
  int len = new - proc->pc;
  printf("ADV %d (0x%04x -> 0x%04x) ", len, proc->pc, new);
  while (ii < len) {
    printf((!ii ? "%02x" : " %02x"), read_mem_1(cpu->program, proc->pc + ii));
    ++ii;
  }
  printf("\n");
}

// next_instruction updates the execution time for PROC.
void next_cpu_op(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t op;
  int newpc;

  op = (char)read_mem_1(cpu->program, proc->pc);
  if (op < 1 || NUM_OPS < op) {
    proc->opcode = 0;
    newpc = proc->pc + 1;
    if (newpc >= MEM_SIZE)
      newpc %= MEM_SIZE;
    if (newpc < 0)
      newpc += MEM_SIZE;
    proc->pc = newpc;
  } else {
    proc->opcode = op;
    proc->instruction_time = g_op_tab[op].cycles_to_exec;
  }
}

// next goes to the next instruction and sets the execution time.
// void next(struct s_cpu *cpu, struct s_process *proc) {
//   if (!cpu || !proc) {
//     if (f_verbose & OPT_INTLDBG)
//       fprintf(stderr, "ERROR: cpu or proc NULL in next_instruction()\n");
//     return;
//   }
//   if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
//     printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
//            "pc(%4d) ins_time(%4d) prev_time(%4d) player(%d) NEXT start\n",
//            cpu->clock, proc->pid, proc->carry, proc->last_live, proc->pc,
//            proc->instruction_time, proc->prev_time, proc->player);
//   if (proc->pc >= MEM_SIZE)
//     proc->pc %= (MEM_SIZE);
//   if (proc->pc < 0)
//     proc->pc += MEM_SIZE;
//   proc->opcode = cpu->program[proc->pc];
//   if (proc->opcode >= e_live && proc->opcode <= e_aff) {
//     proc->instruction_time = g_op_tab[proc->opcode].cycles_to_exec - 1;
//   } else {
//     if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_DBGOUT))
//       printf("DBG: NOP: next instruction is an error opcode(%02x)(%d)\n",
//              proc->opcode, proc->opcode);
//     proc->instruction_time = 0;
//   }
//   if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
//     printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
//            "pc(%4d) ins_time(%4d) prev_time(%4d) player(%d) NEXT end\n",
//            cpu->clock, proc->pid, proc->carry, proc->last_live, proc->pc,
//            proc->instruction_time, proc->prev_time, proc->player);
// }

/* Utility functions */

// Write VAL into MEM[IDX]
void write_mem_ins(uint8_t *mem, uint32_t idx, uint32_t val) {
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: write_mem_ins idx(%08x) val(%08x)\n", idx, val);
  mem[idx % MEM_SIZE] = (val >> 24) & 0xff;
  mem[(idx + 1) % MEM_SIZE] = (val >> 16) & 0xff;
  mem[(idx + 2) % MEM_SIZE] = (val >> 8) & 0xff;
  mem[(idx + 3) % MEM_SIZE] = val & 0xff;
}

// Write VAL into register REG for the process PROC
void write_reg(struct s_process *proc, uint32_t reg, uint32_t val) {
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: write_reg reg(%08x) val(%08x) lav(%08x)\n", reg, val,
           ntohl(val));
  if (reg > 0 && reg <= REG_NUMBER)
    proc->registers[reg - 1] = ntohl(val);
}

// Read register REG for the process PROC
int read_reg(struct s_process *proc, uint32_t reg) {
  if (reg > 0 && reg <= REG_NUMBER) {
    int ret = proc->registers[reg - 1];
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_reg reg(%08x) ger(%08x)\n", ret, ntohl(ret));
    return ntohl(ret);
  }
  return 0;
}

// Parse the argument size specified by ARG, read accordingly, and then return
// VAL
int read_val(struct s_cpu *cpu, struct s_process *proc, int arg) {
  int val = 0;
  int idx;

  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: read_val arg(%d) pc(%d)\n", arg, proc->pc);
  if (arg == REG_CODE) {
    val = (char)read_mem_1(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val reg val(%08x)\n", val);
    proc->pc += 1;
  } else if (arg == DIR_CODE) {
    val = (int)read_mem_4(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val dir val(%08x)\n", val);
    proc->pc += 4;
  } else if (arg == IND_CODE) {
    idx = (short)read_mem_2(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val ind idx(%04x)\n", idx);
    val = (int)read_mem_4(cpu->program, idx + proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val ind val(%08x)\n", val);
    proc->pc += 2;
  }
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: read_val end pc(%d)\n", proc->pc);
  return (val);
}

// Modify the carry flag for the current process based on VAL
void mod_carry(struct s_process *proc, int val) {
  if (val) {
    proc->carry = 1;
  } else {
    proc->carry = 0;
  }
}

// Read the specified ARG, adjusting size based on OP direct_size member in
// G_OP_TAB. Used primarily in LDI, STI, & LLDI
int read_val_idx(struct s_cpu *cpu, struct s_process *proc, int arg, int op) {
  int reg;
  int val = 0;
  int idx;

  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: read_val_idx arg(%d) pc(%d)\n", arg, proc->pc);
  if (arg == REG_CODE) {
    reg = (char)read_mem_1(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_idx REG reg(%02x)\n", reg);
    val = read_reg(proc, reg);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_idx REG val(%08x)\n", val);
    proc->pc += 1;
  } else if (arg == DIR_CODE) {
    if (g_op_tab[op].direct_size)
      val = (short)read_mem_2(cpu->program, proc->pc);
    else
      val = (int)read_mem_4(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG)) {
      if (g_op_tab[op].direct_size)
        printf("DBG: read_val_idx DIR val(%04x)\n", val);
      else
        printf("DBG: read_val_idx DIR val(%08x)\n", val);
    }
    proc->pc += (g_op_tab[op].direct_size ? 2 : 4);
  } else if (arg == IND_CODE) {
    idx = (short)read_mem_2(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_idx IND idx(0x%04x)(%d)\n", idx, idx);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      dump_nbytes(cpu, 6, idx + proc->pc, 1);
    val = (int)read_mem_4(cpu->program, idx + proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_idx IND val(0x%08x)(%d)\n", val, val);
    proc->pc += 2;
  }
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: read_val_idx end pc(%d)\n", proc->pc);
  return val;
}

// Read the specified ARG, reading the current value of a register if necessary
int read_val_load(struct s_cpu *cpu, struct s_process *proc, int arg) {
  int reg;
  int val = 0;
  int idx;

  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: read_val_load arg(%d) pc(%d)\n", arg, proc->pc);
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    dump_nbytes(cpu, 8, proc->pc, 1);
  if (arg == REG_CODE) {
    reg = (char)read_mem_1(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load REG reg(%02x)\n", reg);
    val = read_reg(proc, reg);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load REG val(%08x)\n", val);
    proc->pc += 1;
  } else if (arg == DIR_CODE) {
    val = (int)read_mem_4(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load DIR val(%08x)\n", val);
    proc->pc += 4;
  } else if (arg == IND_CODE) {
    idx = (short)read_mem_2(cpu->program, proc->pc) & (IDX_MOD - 1);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load IND idx(%08x)\n", idx);
    val = (int)read_mem_4(cpu->program, proc->pc + idx - 2);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load IND val(%08x)\n", val);
    proc->pc += 2;
  }
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: read_val_load end pc(%d)\n", proc->pc);
  return val;
}

/* Instructions */

// Stores the current cycle into the lastlive array in CPU, if the NAME
// corresponds to an active player
int instruction_live(struct s_cpu *cpu, struct s_process *proc) {
  int player_id;
  int local_34;

  player_id = (int)read_mem_4(cpu->program, proc->pc + 1);
  if (f_verbose & OPT_LIVES)
    printf("P% 5d | live %d\n", proc->pid, player_id);
  proc->last_live = cpu->clock;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: proc->last_live(%d) INS_LIVE\n", proc->last_live);
  cpu->nbr_lives += 1;
  // cpu->winner = player_id;
  local_34 = ~player_id;
  if (local_34 >= 0 && local_34 < MAX_PLAYERS) {
      if ((f_verbose & OPT_LIVES))
    printf("Player %d (%s) is said to be alive\n", local_34 + 1,
           cpu->players[local_34].name);
    cpu->players[local_34].last_live = cpu->clock;
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 5);
  proc->opcode = 0;
  return proc->pc + 5;
}
// int instruction_live(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   int player = 0;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LIVE start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   cpu->nbr_lives++;
//   proc->last_live = cpu->clock;
//   proc->pc += 1;
//   int v1 = (int)read_mem_4(cpu->program, proc->pc);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(%08x)(%d) INS_LIVE read\n", v1, v1);
//   proc->pc += 4;
//   if (v1 >= -4 && v1 <= -1) {
//     player = -v1;
//     cpu->lastlive[player - 1] = cpu->clock;
//     cpu->winner = player;
//   }
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pc(%d) v1(%08x) pid(%d) last_live(%d) nbr_lives(%d) "
//            "INS_LIVE end\n",
//            proc->pc, v1, proc->pid, proc->last_live, cpu->nbr_lives);
//   if (f_verbose & OPT_INSTR)
//     printf("P% 5d | live %d\n", proc->pid, v1);
//   if ((f_verbose & OPT_LIVES) && player > 0)
//     printf("Player %d (%s) is said to be alive\n", player,
//            cpu->players[player - 1].name);
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc, proc->pc - pc);
//   return 1;
// }

// reads an indirect value from core memory
int read_indirect(struct s_cpu *cpu, struct s_process *proc, short offset) {
  int idx;
  int ofs;
  int ret;

  ofs = offset % IDX_MOD;
  idx = proc->pc + ofs;
  ret = (int)read_mem_4(cpu->program, idx);
  proc->opcode = 0;
  return ret;
}

// check a register number
int validate_register(int reg) { return 0 < reg && reg < 0x11; }

// ld takes 2 parameters, 2nd must be a register that isn't the
// 'program counter'. It loads the value of the first parameter in the register,
// and modifies the 'carry'. 'ld 34,r3' loads the REG_SIZE bytes from address
// (PC + (34 % IDX_MOD)) in register r3.
// TODO: validate pcb

// LD DONE
int instruction_ld(struct s_cpu *cpu, struct s_process *proc) {
  // ACK
  int val;
  int reg;
  int type;
  int size_type;
  int valid_reg;
  int test;
  int size;
  uint8_t pcb;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LD start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  pcb = read_mem_1(cpu->program, proc->pc + 1);
  test = check_pcb(pcb, 2);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pcb(%d) test(%d) INS_LD\n", pcb, test);
  if (test != 0) {
    type = type_from_pcb(pcb, 0);
    size_type = size_from_pt(type, 2);
    if (type == 4) {
      val = (int)read_mem_4(cpu->program, proc->pc + 2);
    } else {
      val = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc));
    }
    reg = read_mem_1(cpu->program, proc->pc + 2 + size_type);
    valid_reg = validate_register(reg);
    if (valid_reg != 0) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | ld %d r%d\n", proc->pid, val, reg);
      mod_carry(proc, (val == 0));
      write_reg(proc, reg, val);
    }
  }
  size = size_from_pcb(pcb, 2);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + size + 2);
  proc->opcode = 0;
  return proc->pc + size + 2;
}

// int instruction_ld(struct s_cpu *cpu, struct s_process *proc) {
// if (f_verbose & OPT_INTLDBG)
//   printf("DBG: op(%d) == prog(%d) -> (%d)\n",proc->opcode, cpu->program[pc],
//   proc->opcode == cpu->program[pc]);
// int good = check_param(par, proc->opcode);
// if (f_verbose & OPT_INTLDBG)
//   printf("DBG: good(%d) == test(%d) -> (%d)\n",good,test, good==test);
// if (test == 0) {
//   if (f_verbose & OPT_INTLDBG)
//     dprintf(STDERR_FILENO, "DBG: par(%02hhx) invalid param INS_LD ret\n",
//             par);
//   proc->pc += 3;
//   return 1;
// }
// proc->pc += 1;
// if (f_verbose & OPT_INTLDBG)
//   printf("DBG: pc(%d) INS_LD\n", proc->pc);
// val = read_val_load(cpu, proc, get_param(par, 1));
// if (f_verbose & OPT_INTLDBG)
//   printf("DBG: pc(%d) val(%08x) INS_LD\n", proc->pc, val);
// uint8_t reg = read_mem_1(cpu->program, proc->pc);
// if (f_verbose & OPT_INTLDBG)
//   printf("DBG: pc(%d) reg(%02hhx) INS_LD\n", proc->pc, reg);
// write_reg(proc, reg, val);
// mod_carry(proc, (val == 0));
// proc->pc += 1;
// if (f_verbose & OPT_INTLDBG)
//   printf("DBG: pc(%d) INS_LD end\n", proc->pc);
// if (f_verbose & OPT_INSTR)
//   printf("P% 5d | ld %d r%d\n", proc->pid, val, reg);
// if (f_verbose & OPT_PCMOVE)
//   print_adv(cpu, proc->pc - pc, pc);
// next(cpu, proc);
// return 1;
// }

// st takes 2 parameters, storing (REG_SIZE bytes) of the value of
// the first argument (always a register) in the second. 'st r4,34' stores the
// value of 'r4' at the address (PC + (34 % IDX_MOD)) 'st r3,r8' copies the
// contents of 'r3' to 'r8'
// TODO: validate pcb
int instruction_st(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int type;
  int valid_reg;
  int val;
  int reg;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  int check = check_pcb(pcb, 3);
  if (check != 0) {
    reg = read_mem_1(cpu->program, proc->pc + 2);
    type = type_from_pcb(pcb, 1);
    if (type == 2) {
      val = (short)read_mem_2(cpu->program, proc->pc + 3);
    } else {
      val = read_mem_1(cpu->program, proc->pc + 3);
    }
    valid_reg = validate_register(reg);
    if (valid_reg != 0) {
      if ((type == 1) && (valid_reg = validate_register(val)) != 0) {
        if (f_verbose & OPT_INSTR)
          printf("P% 5d | st r%d %d\n", proc->pid, reg, val);
        reg = read_reg(proc, reg);
        write_reg(proc, val, read_reg(proc, reg));
      } else {
        if (type == 2) {
          if (f_verbose & OPT_INSTR)
            printf("P% 5d | st r%d %d\n", proc->pid, reg, val);
          int ivar2 = proc->pc;
          int uvar3 = read_reg(proc, reg);
          write_mem_ins(cpu->program, (ivar2 + val % IDX_MOD), uvar3);
        }
      }
    }
  }
  reg = size_from_pcb(pcb, 3);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + reg + 2);
  proc->opcode = 0;
  return proc->pc + reg + 2;
}
// int instruction_st(struct s_cpu *cpu, struct s_process *proc) {
//   int v1, r1;
//   short v2;
//   int pc = proc->pc;

//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ST start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   if (check_param(par, cpu->program[pc] - 1)) {
//     int size = param_total_size(par, 3);
//     proc->pc = proc->pc + size + 1;
//     if (f_verbose & OPT_INTLDBG)
//       printf("DBG: bad par(%02hhx) size(%d) pc(%04x)(%d) mem(%02hhx)"
//              " INS_ST ret\n",
//              par, size + 1, proc->pc, proc->pc,
//              cpu->program[(proc->pc - 1) % MEM_SIZE]);
//     if (f_verbose & OPT_PCMOVE)
//       print_adv(cpu, proc->pc - pc, pc);
//     next(cpu, proc);
//     return 1;
//   }
//   proc->pc += 1;
//   int p1 = get_param(par, 1);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: p1(%d) p1 == REG_CODE %d INS_ST\n", p1, p1 == REG_CODE);
//   r1 = read_val(cpu, proc, p1);
//   v1 = read_reg(proc, r1);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pc(%d) mem(%02x) v1(%08x) INS_ST\n", proc->pc,
//            cpu->program[proc->pc], v1);
//   if (get_param(par, 2) == REG_CODE) {
//     if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
//       dump_nbytes(cpu, 7, pc, 1);
//     v2 = read_mem_1(cpu->program, proc->pc);
//     write_reg(proc, v2, v1);
//     proc->pc += 1;
//   } else {
//     if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
//       dump_nbytes(cpu, 5, pc, 1);
//     v2 = (short)read_mem_2(cpu->program, proc->pc);
//     if (f_verbose & OPT_INTLDBG)
//       printf("DBG: ind(%04hx) idx(%d) INS_ST\n", v2, (pc + ((v2) %
//       IDX_MOD)));
//     write_mem_ins(cpu->program, pc + ((v2) % IDX_MOD), v1);
//     proc->pc += 2;
//   }
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | st r%d %d\n", proc->pid, r1, v2);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// add takes 3 registers as parameters, adding the contents of the
// first and second, storing the result into the third. Modifies carry. 'add
// r2,r3,r5' adds the values of 'r2' and 'r3' and stores the result in 'r5'.
// TODO: validate pcb
int instruction_add(struct s_cpu *cpu, struct s_process *proc) {
  int val;
  int reg1, reg2, reg3;
  uint8_t pcb;
  int ivar2, uvar3, uvar4;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar3 = check_pcb(pcb, 4);
  if (uvar3 != 0) {
    reg1 = read_mem_1(cpu->program, proc->pc + 2);
    reg2 = read_mem_1(cpu->program, proc->pc + 3);
    reg3 = read_mem_1(cpu->program, proc->pc + 4);
    ivar2 = validate_register(reg1);
    if (((ivar2 != 0) && (ivar2 = validate_register(reg2)) != 0) &&
        (ivar2 = validate_register(reg3)) != 0) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | add r%d r%d r%d\n", proc->pid, reg1, reg2, reg3);
      uvar3 = read_reg(proc, reg1);
      uvar4 = read_reg(proc, reg2);
      val = uvar3 + uvar4;
      mod_carry(proc, (val == 0));
      write_reg(proc, reg3, val);
    }
  }
  val = size_from_pcb(pcb, 4);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + val + 2);
  proc->opcode = 0;
  return proc->pc + val + 2;
}

// int instruction_add(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;

//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ADD start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   proc->pc += 1;
//   int r1 = read_val(cpu, proc, get_param(par, 1));
//   int r2 = read_val(cpu, proc, get_param(par, 2));
//   int r3 = read_val(cpu, proc, get_param(par, 3));
//   int v1 = read_reg(proc, r1);
//   int v2 = read_reg(proc, r2);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(%08x) v2(%08x) INS_ADD\n", v1, v2);
//   write_reg(proc, r3, v1 + v2);
//   mod_carry(proc, (v1 + v2) == 0);
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | add r%d r%d r%d\n", proc->pid, r1, r2, r3);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// sub is the same as instruction_add, except performs subtraction.
// TODO: validate pcb
int instruction_sub(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  uint8_t uvar2;
  int ivar3;
  int uvar4;
  int uvar5;
  int val;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 5);
  if (uvar4 != 0) {
    uvar1 = read_mem_1(cpu->program, proc->pc + 2);
    val = uvar1;
    uvar1 = read_mem_1(cpu->program, proc->pc + 3);
    uvar2 = read_mem_1(cpu->program, proc->pc + 4);
    ivar3 = validate_register(val);
    if (((ivar3 != 0) && (ivar3 = validate_register(uvar1)) != 0) &&
        (ivar3 = validate_register(uvar2)) != 0) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | sub r%d r%d r%d\n", proc->pid, val, uvar1, uvar2);
      uvar4 = read_reg(proc, val);
      uvar5 = read_reg(proc, uvar1);
      val = uvar4 - uvar5;
      mod_carry(proc, (val == 0));
      write_reg(proc, uvar2, val);
    }
  }
  val = size_from_pcb(pcb, 5);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + val + 2);
  proc->opcode = 0;
  return proc->pc + val + 2;
}
// int instruction_sub(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_SUB start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   proc->pc += 1;
//   int r1 = read_val(cpu, proc, get_param(par, 1));
//   int r2 = read_val(cpu, proc, get_param(par, 2));
//   int r3 = read_val(cpu, proc, get_param(par, 3));
//   int v1 = read_reg(proc, r1);
//   int v2 = read_reg(proc, r2);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(%08x) v2(%08x) INS_SUB\n", v1, v2);
//   write_reg(proc, r3, v1 - v2);
//   mod_carry(proc, (v1 - v2) == 0);
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | sub r%d r%d r%d\n", proc->pid, r1, r2, r3);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// perform a bitwise AND on the first two parameters, storing theresult into the
// third which is always a register. Modifies carry.'and r2,%0,r3' stores 'r2 &
// 0' into 'r3'.
// TODO: validate pcb
int instruction_and(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int ivar2;
  int type;
  int uvar3;
  int uvar4;
  int uvar5;
  int local_40;
  int local_34;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 6);
  if (uvar4 != 0) {
    type = type_from_pcb(pcb, 0);
    ivar2 = size_from_pt(type, 6);
    if (type == REG_CODE) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 6);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      uvar4 = read_reg(proc, uvar1);
      local_34 = uvar4;
    } else {
      if (type == DIR_CODE) {
        local_34 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (int)read_mem_4(cpu->program, proc->pc + 2);
      }
    }
    type = type_from_pcb(pcb, 1);
    uvar4 = size_from_pt(type, 6);
    if (type == REG_CODE) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 6);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      uvar5 = read_reg(proc, uvar1);
      local_40 = uvar5;
    } else {
      if (type == DIR_CODE) {
        local_40 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2 + ivar2));
      } else {
        local_40 = (int)read_mem_4(cpu->program, proc->pc + 2 + ivar2);
      }
    }
    uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2 + uvar4);
    type = validate_register(uvar1);
    if (type == 0) {
      type = proc->pc;
      uvar3 = size_from_pcb(pcb, 6);
      if (f_verbose & OPT_PCMOVE) {
        print_adv(cpu, proc, type + 2 + uvar3);
      }
      proc->opcode = 0;
      return type + 2 + uvar3;
    }
    if (f_verbose & OPT_INSTR)
      printf("P% 5d | and %d %d r%d\n", proc->pid, local_34, local_40, uvar1);
    mod_carry(proc, (local_34 & local_40) == 0);
    write_reg(proc, uvar1, (local_34 & local_40));
  }
  type = proc->pc;
  uvar3 = size_from_pcb(pcb, 6);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, type + 2 + uvar3);
  proc->opcode = 0;
  return type + 2 + uvar3;
}

// int instruction_and(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_AND start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   int v1, v2, r1, r2;
//   if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
//     dump_nbytes(cpu, 9, proc->pc, 1);
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   proc->pc += 1;
//   int p1 = get_param(par, 1);
//   int p2 = get_param(par, 2);
//   if (p1 != IND_CODE) {
//     r1 = v1 = read_val(cpu, proc, p1);
//     if (p1 == REG_CODE)
//       v1 = read_reg(proc, v1);
//   } else {
//     v1 = (short)read_mem_2(cpu->program, proc->pc);
//     v1 = (int)read_mem_4(cpu->program, v1 + proc->pc - 2);
//     proc->pc += 2;
//   }
//   if (p2 != IND_CODE) {
//     r2 = v2 = read_val(cpu, proc, p2);
//     if (p2 == REG_CODE)
//       v2 = read_reg(proc, v2);
//   } else {
//     v2 = (short)read_mem_2(cpu->program, proc->pc);
//     v2 = (int)read_mem_4(cpu->program,
//                        v2 + proc->pc - 2 - (p1 == IND_CODE ? 2 : 0));
//     proc->pc += 2;
//   }
//   int v3 = read_val(cpu, proc, get_param(par, 3));
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(%08x) v2(%08x) v3(%08x) INS_AND\n", v1, v2, v3);
//   write_reg(proc, v3, v1 & v2);
//   mod_carry(proc, (v1 & v2) == 0);
//   if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
//     dump_nbytes(cpu, 9, proc->pc, 1);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pc(%d) INS_AND end\n", proc->pc);
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | and ", proc->pid);
//     if (p1 == REG_CODE)
//       printf("r%d ", r1);
//     else
//       printf("%d ", v1);
//     if (p2 == REG_CODE)
//       printf("r%d ", r2);
//     else
//       printf("%d ", v2);
//     printf("r%d\n", v3);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// or is the same as instruction_and, except performs a bitwise OR.
// TODO: validate pcb
int instruction_or(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int ivar2;
  int type;
  int uvar3;
  int uvar4;
  int uvar5;
  int local_40;
  int local_34;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 7);
  if (uvar4 != 0) {
    type = type_from_pcb(pcb, 0);
    ivar2 = size_from_pt(type, 7);
    if (type == REG_CODE) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      uvar4 = read_reg(proc, uvar1);
      local_34 = uvar4;
    } else {
      if (type == DIR_CODE) {
        local_34 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (int)read_mem_4(cpu->program, proc->pc + 2);
      }
    }
    type = type_from_pcb(pcb, 1);
    uvar4 = size_from_pt(type, 7);
    if (type == REG_CODE) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      uvar5 = read_reg(proc, uvar1);
      local_40 = uvar5;
    } else {
      if (type == DIR_CODE) {
        local_40 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2 + ivar2));
      } else {
        local_40 = (int)read_mem_4(cpu->program, proc->pc + 2 + ivar2);
      }
    }
    uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2 + uvar4);
    type = validate_register(uvar1);
    if (type == 0) {
      type = proc->pc;
      uvar3 = size_from_pcb(pcb, 7);
      if (f_verbose & OPT_PCMOVE) {
        print_adv(cpu, proc, type + 2 + uvar3);
      }
      proc->opcode = 0;
      return type + 2 + uvar3;
    }
    if (f_verbose & OPT_INSTR)
      printf("P% 5d | or %d %d r%d\n", proc->pid, local_34, local_40, uvar1);
    mod_carry(proc, (local_34 | local_40) == 0);
    write_reg(proc, uvar1, (local_34 | local_40));
  }
  type = proc->pc;
  uvar3 = size_from_pcb(pcb, 6);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, type + 2 + uvar3);
  proc->opcode = 0;
  return type + 2 + uvar3;
}
// int instruction_or(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;

//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_OR start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   int v1, v2, r1, r2;
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   proc->pc += 1;
//   int p1 = get_param(par, 1);
//   int p2 = get_param(par, 2);
//   if (p1 != IND_CODE) {
//     r1 = v1 = read_val(cpu, proc, p1);
//     if (p1 == REG_CODE)
//       v1 = read_reg(proc, v1);
//   } else {
//     v1 = (short)read_mem_2(cpu->program, proc->pc);
//     v1 = (int)read_mem_4(cpu->program, v1 + proc->pc - 2);
//     proc->pc += 2;
//   }
//   if (p2 != IND_CODE) {
//     r2 = v2 = read_val(cpu, proc, p2);
//     if (p2 == REG_CODE)
//       v2 = read_reg(proc, v2);
//   } else {
//     v2 = (short)read_mem_2(cpu->program, proc->pc);
//     v2 = (int)read_mem_4(cpu->program,
//                        v2 + proc->pc - 2 - (p1 == IND_CODE ? 2 : 0));
//     proc->pc += 2;
//   }
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(%08x) v2(%08x) INS_OR\n", v1, v2);
//   write_reg(proc, read_val(cpu, proc, get_param(par, 3)), v1 | v2);
//   mod_carry(proc, (v1 | v2) == 0);
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | and ", proc->pid);
//     if (p1 == REG_CODE)
//       printf("r%d ", r1);
//     else
//       printf("%d", v1);
//     if (p2 == REG_CODE)
//       printf("r%d", r2);
//     else
//       printf("%d", v2);
//     printf("\n");
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// xor is the same as instruction_and, except performs a bitwise
// XOR.
int instruction_xor(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int ivar2;
  int type;
  int uvar3;
  int uvar4;
  int uvar5;
  int local_40;
  int local_34;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 7);
  if (uvar4 != 0) {
    type = type_from_pcb(pcb, 0);
    ivar2 = size_from_pt(type, 7);
    if (type == REG_CODE) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      uvar4 = read_reg(proc, uvar1);
      local_34 = uvar4;
    } else {
      if (type == DIR_CODE) {
        local_34 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (int)read_mem_4(cpu->program, proc->pc + 2);
      }
    }
    type = type_from_pcb(pcb, 1);
    uvar4 = size_from_pt(type, 7);
    if (type == REG_CODE) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      uvar5 = read_reg(proc, uvar1);
      local_40 = uvar5;
    } else {
      if (type == DIR_CODE) {
        local_40 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2 + ivar2));
      } else {
        local_40 = (int)read_mem_4(cpu->program, proc->pc + 2 + ivar2);
      }
    }
    uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2 + uvar4);
    type = validate_register(uvar1);
    if (type == 0) {
      type = proc->pc;
      uvar3 = size_from_pcb(pcb, 7);
      if (f_verbose & OPT_PCMOVE) {
        print_adv(cpu, proc, type + 2 + uvar3);
      }
      proc->opcode = 0;
      return type + 2 + uvar3;
    }
    if (f_verbose & OPT_INSTR)
      printf("P% 5d | xor %d %d r%d\n", proc->pid, local_34, local_40, uvar1);
    mod_carry(proc, (local_34 ^ local_40) == 0);
    write_reg(proc, uvar1, (local_34 ^ local_40));
  }
  type = proc->pc;
  uvar3 = size_from_pcb(pcb, 6);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, type + 2 + uvar3);
  proc->opcode = 0;
  return type + 2 + uvar3;
}
// int instruction_xor(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_XOR start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   int v1, v2, r1, r2;
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   proc->pc += 1;
//   int p1 = get_param(par, 1);
//   int p2 = get_param(par, 2);
//   if (p1 != IND_CODE) {
//     r1 = read_mem_1(cpu->program, proc->pc);
//     v1 = read_val_load(cpu, proc, p1);
//   } else {
//     v1 = (short)read_mem_2(cpu->program, proc->pc);
//     v1 = (int)read_mem_4(cpu->program, v1 + proc->pc - 2);
//     proc->pc += 2;
//   }
//   if (p2 != IND_CODE) {
//     r2 = read_mem_1(cpu->program, proc->pc);
//     v2 = read_val_load(cpu, proc, p2);
//   } else {
//     v2 = (short)read_mem_2(cpu->program, proc->pc);
//     v2 = (int)read_mem_4(cpu->program,
//                        v2 + proc->pc - 2 - (p1 == IND_CODE ? 2 : 0));
//     proc->pc += 2;
//   }
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(%08x) v2(%08x) INS_XOR\n", v1, v2);
//   write_reg(proc, read_val(cpu, proc, get_param(par, 3)), v1 ^ v2);
//   mod_carry(proc, (v1 ^ v2) == 0);
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | and ", proc->pid);
//     if (p1 == REG_CODE)
//       printf("r%d ", r1);
//     else
//       printf("%d", v1);
//     if (p2 == REG_CODE)
//       printf("r%d", r2);
//     else
//       printf("%d", v2);
//     printf("\n");
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// zjmp always takes an index (IND_SIZE) and makes a jump at this
// index if carry is true, otherwise consuming cycles. 'zjmp %23' stores (PC +
// (23 % IDX_MOD)) into PC.
int instruction_zjmp(struct s_cpu *cpu, struct s_process *proc) {
  int uvar1;
  int uvar2;
  int local_c;

  uvar1 = (short)read_mem_2(cpu->program, proc->pc + 1);
  uvar2 = (short)uvar1;
  if (proc->carry == 0) {
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | zjmp %d FAILED\n", proc->pid, uvar2);
    }
    if (f_verbose & OPT_PCMOVE)
      print_adv(cpu, proc, proc->pc + 3);
    local_c = proc->pc + 3;
  } else {
    if (f_verbose & OPT_INSTR)
      printf("P% 5d | zjmp %d OK\n", proc->pid, uvar2);
    local_c = proc->pc + uvar2 % IDX_MOD;
  }
  proc->opcode = 0;
  return local_c;
}
// int instruction_zjmp(struct s_cpu *cpu, struct s_process *proc) {
//   ; // TODO: better instruction_zjmp regression tests
//   int pc = proc->pc;

//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ZJMP start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   // proc->pc += 1;
//   short v1 = (short)read_mem_2(cpu->program, proc->pc + 1);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: carry(%d) v1(%04hx)(%d) INS_ZJMP\n", proc->carry, v1,
//            v1 % IDX_MOD);
//   if (proc->carry == true) {
//     proc->pc = ((proc->pc + ((v1) % IDX_MOD)) % MEM_SIZE);
//     if (f_verbose & OPT_INTLDBG)
//       printf("DBG: jumping to pc(%d) INS_ZJMP\n", proc->pc);
//   } else {
//     proc->pc += 3;
//   }
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | zjmp %d %s\n", proc->pid, v1,
//            (proc->carry ? "OK" : "FAILED"));
//   }
//   if ((f_verbose & OPT_PCMOVE) && !proc->carry)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// ldi modifies carry. 'idx' and 'add' are indexes, and 'reg' is a
// register. 'ldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3 %
// IDX_MOD)), adding 4 to this sum S. Read REG_SIZE bytes at address (PC + (S %
// IDX_MOD)), which are copied to 'r1'.
// TODO: validate pcb
int instruction_ldi(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int ivar2;
  int type;
  int uvar3;
  int uvar4;
  int uvar5;
  int local_40;
  int local_34;
  // int local_c;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 10);
  if (uvar4 != 0) {
    type = type_from_pcb(pcb, 0);
    ivar2 = size_from_pt(type, 10);
    if (type == 1) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 10);
        if (f_verbose & OPT_PCMOVE)
          print_adv(cpu, proc, type + 2 + uvar3);
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      local_34 = read_reg(proc, uvar1);
    } else {
      if (type == 2) {
        local_34 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (short)read_mem_2(cpu->program, proc->pc + 2);
      }
    }
    uvar4 = type_from_pcb(pcb, 1);
    uvar5 = size_from_pt(uvar4, 10);
    if (uvar4 == 1) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 10);
        if (f_verbose & OPT_PCMOVE)
          print_adv(cpu, proc, type + 2 + uvar3); // left off at line 59 _op_ldi
        proc->opcode = 0;
        return type + 2 + uvar3;
      }
      local_40 = read_reg(proc, uvar1);
    } else {
      local_40 = (short)read_mem_2(cpu->program, proc->pc + 2 + ivar2);
    }
    uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2 + uvar5);
    type = validate_register(uvar1);
    if (type == 0) {
      type = proc->pc;
      uvar3 = size_from_pcb(pcb, 10);
      if (f_verbose & OPT_PCMOVE)
        print_adv(cpu, proc, type + 2 + uvar3);
      proc->opcode = 0;
      return type + 2 + uvar3;
    }
    int idx = proc->pc + (local_34 + local_40) % IDX_MOD;
    uvar3 = read_mem_4(cpu->program, idx);
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | ldi %d %d r%d\n", proc->pid, local_34, local_40, uvar3);
      printf("       | -> load from %d + %d = %d (with pc and mod %d)\n",
             local_34, local_40, local_34 + local_40, (idx));
    }
    write_reg(proc, uvar1, uvar3);
  }
  type = proc->pc;
  uvar3 = size_from_pcb(pcb, 10);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, type + 2 + uvar3);
  proc->opcode = 0;
  return type + 2 + uvar3;
}
// int instruction_ldi(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LDI start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   int v1, v2, v3, idx, r1, r2;
//   int op = cpu->program[proc->pc];
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   if (check_param(par, op - 1)) {
//     if (f_verbose & OPT_INTLDBG)
//       dprintf(STDERR_FILENO, "DBG: par(%02hhx) invalid param INS_LDI ret\n",
//               par);
//     proc->pc += 3;
//     return 1;
//   }
//   proc->pc += 1;
//   int p1 = get_param(par, 1);
//   int p2 = get_param(par, 2);
//   if (p1 != IND_CODE) {
//     r1 = read_mem_1(cpu->program, proc->pc);
//     v1 = read_val_idx(cpu, proc, p1, op);
//   } else {
//     v1 = (short)read_mem_2(cpu->program, proc->pc);
//     proc->pc += 2;
//     v1 = (int)read_mem_4(cpu->program, v1 + pc);
//   }
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(0x%08x)(%d) INS_LDI\n", v1, v1);
//   r2 = read_mem_1(cpu->program, proc->pc);
//   if (p2 == REG_CODE)
//     v2 = read_val_idx(cpu, proc, p2, op);
//   else
//     v2 = (short)read_val_idx(cpu, proc, p2, op);
//   idx = (pc + (v1 % IDX_MOD)) + v2;
//   if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
//     dump_nbytes(cpu, 4, idx, 1);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: load from %d + %d = %d [with pc and mod=idx(%d)] INS_LDI\n",
//            v1, v2, v1 + v2, idx);
//   v3 = (int)read_mem_4(cpu->program, idx);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v3(%08x) INS_LDI\n", v3);
//   char reg = read_mem_1(cpu->program, proc->pc);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: reg(%d) v1(%08x) v2(%08x) v3(%08x) INS_LDI write_reg\n",
//     reg,
//            v1, v2, v3);
//   write_reg(proc, reg, v3);
//   proc->pc += 1;
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | ldi", proc->pid);
//     if (p1 == REG_CODE)
//       printf(" r%d", r1);
//     else
//       printf(" %d", v1);
//     if (p2 == REG_CODE)
//       printf(" r%d", r2);
//     else
//       printf(" %d", v2);
//     printf(" r%d\n", reg);
//     printf("       | -> load from %d + %d = %d (with pc and mod %d)\n", v1,
//     v2,
//            v1 + v2, idx);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// sti does something probably. 'sti r2,%4,%5' copies REG_SIZE bytes
// of 'r2' at address (4 + 5) Parameters 2 and 3 are treated as indexes.
// TODO: validate pcb
int instruction_sti(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int type;
  int uvar2;
  int uvar3;
  int reg;
  int local_44;
  int local_3c;
  int local_c;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar2 = check_pcb(pcb, 0xb);
  if (uvar2 != 0) {
    reg = read_mem_1(cpu->program, proc->pc + 2);
    type = validate_register(reg);
    if (type == 0) {
      type = proc->pc;
      reg = size_from_pcb(pcb, 0xb);
      if (f_verbose & OPT_PCMOVE) {
        print_adv(cpu, proc, type + 2 + reg);
      }
      type = proc->pc;
      local_c = size_from_pcb(pcb, 0xb);
      local_c = type + 2 + local_c;
      goto LAB_100007631;
    }
    type = type_from_pcb(pcb, 1);
    uvar2 = size_from_pt(type, 0xb);
    if (type == 1) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 3);
      type = validate_register(uvar1);
      if (type == 0) {
        if (f_verbose & OPT_PCMOVE) {
          type = proc->pc;
          reg = size_from_pcb(pcb, 0xb);
          print_adv(cpu, proc, type + 2 + reg);
        }
        type = proc->pc;
        local_c = size_from_pcb(pcb, 0xb);
        local_c = type + 2 + local_c;
        goto LAB_100007631;
      }
      local_3c = read_reg(proc, uvar1);
    } else {
      if (type == 2) {
        local_3c = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 3));
      } else {
        local_3c = (short)read_mem_2(cpu->program, proc->pc + 3);
      }
    }
    uvar3 = type_from_pcb(pcb, 2);
    if (uvar3 == 1) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 3 + uvar2);
      type = validate_register(uvar1);
      if (type == 0) {
        if (f_verbose & OPT_PCMOVE) {
          type = proc->pc;
          reg = size_from_pcb(pcb, 0xb);
          print_adv(cpu, proc, type + 2 + reg);
        }
        type = proc->pc;
        local_c = size_from_pcb(pcb, 0xb);
        local_c = type + 2 + local_c;
        goto LAB_100007631;
      }
      local_44 = read_reg(proc, uvar1);
    } else {
      local_44 = (short)read_mem_2(cpu->program, proc->pc + 3 + uvar2);
    }
    uvar2 = read_reg(proc, reg);
    int idx = proc->pc + (local_3c + local_44) % IDX_MOD;
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | sti r%d %d %d\n", proc->pid, reg, local_3c, local_44);
      printf("       | -> store to %d + %d = %d (with pc and mod %d)\n",
             local_3c, local_44, (local_3c + local_44), idx);
    }
    write_mem_ins(cpu->program, idx, uvar2);
  }
  reg = size_from_pcb(pcb, 0xb);
  if (f_verbose & OPT_PCMOVE) {
    print_adv(cpu, proc, proc->pc + 2 + reg);
  }
  local_c = proc->pc + 2 + reg;
LAB_100007631:
  proc->opcode = 0;
  return local_c;
}
// int instruction_sti(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_STI start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   int v1, v2, v3, idx, p1, p2, p3, r1;
//   int op = proc->opcode;
//   if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
//     dump_nbytes(cpu, 11, pc, 1);
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   if (check_param(par, op - 1)) {
//     if (f_verbose & OPT_INTLDBG)
//       dprintf(2, "DBG: baf INS_STI bad arg\n");
//     proc->pc += 7;
//     if (f_verbose & OPT_PCMOVE)
//       print_adv(cpu, proc->pc - pc, pc);
//     next(cpu, proc);
//     return (1);
//   }
//   proc->pc += 1;
//   p1 = get_param(par, 1);
//   p2 = get_param(par, 2);
//   p3 = get_param(par, 3);
//   r1 = read_mem_1(cpu->program, proc->pc);
//   v1 = read_val_idx(cpu, proc, get_param(par, 1), op);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(%08x)(%d) INS_STI\n", v1, v1);
//   if (p2 == DIR_CODE)
//     v2 = (short)read_val_idx(cpu, proc, p2, op);
//   else
//     v2 = read_val_idx(cpu, proc, p2, op);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v2(%08x)(%d) INS_STI\n", v2, v2);
//   if (p3 == DIR_CODE)
//     v3 = (short)read_val_idx(cpu, proc, p3, op);
//   else
//     v3 = read_val_idx(cpu, proc, p3, op);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v3(%08x)(%d) INS_STI\n", v3, v3);
//   idx = ((pc + (v2 + v3) % IDX_MOD));
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: store to %d + %d = %d [with pc and mod=idx(%d)] INS_STI\n",
//     v2,
//            v3, v2 + v3, idx);
//   write_mem_ins(cpu->program, idx, v1);
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | sti r%d %d %d\n", proc->pid, r1, v2, v3);
//     printf("       | -> store to %d + %d = %d (with pc and mod %d)\n", v2,
//     v3,
//            v2 + v3, idx);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }
void fork_process(struct s_cpu *cpu, struct s_process *proc, int idx) {
  struct s_process *new;
  int idxmod;
  int ii;
  int pl;

  new = malloc(sizeof(*new));
  if (new == NULL) {
    perror("Fatal error");
    exit(-1);
  }
  bzero(new, sizeof(*new));
  idxmod = idx % MEM_SIZE;
  if (idxmod < 0)
    idxmod += MEM_SIZE;
  pl = ~proc->player;
  if (pl >= 0 && pl < MAX_PLAYERS)
    cpu->players[pl].active_processes += 1;
  new->carry = proc->carry;
  new->player = proc->player;
  new->opcode = 0;
  new->instruction_time = 0;
  new->last_live = proc->last_live;
  new->pid = cpu->active + 1;
  ii = 0;
  while (ii < REG_NUMBER) {
    new->registers[ii] = proc->registers[ii];
    ++ii;
  }
  new->next = cpu->processes;
  new->prev = NULL;
  cpu->processes = new;
  cpu->active += 1;
}
// fork always takes an index and creates a new program which is
// executed from address (PC + ('idx' % IDX_MOD)). 'fork %34' spawns a new
// process at (PC + (34 % IDX_MOD)). helltrain cycles (1105,1935,2745,3555,4365)
// TOOD: handle process calling fork correctly: cycle 2745,
//       pid(3).instruction_time is decremented twice
int instruction_fork(struct s_cpu *cpu, struct s_process *proc) {
  int idx;
  short uvar2;

  uvar2 = read_mem_2(cpu->program, proc->pc + 1);
  idx = proc->pc + uvar2;
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | fork %d (%d)\n", proc->pid, uvar2, idx);
  fork_process(cpu, proc, idx % IDX_MOD);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 3);
  proc->opcode = 0;
  return proc->pc + 3;
}
// int instruction_fork(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   short new;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) pc(%4d) "
//            "ins_time(%4d) prev_time(%4d) player(%d) INS_FORK start\n",
//            cpu->clock, proc->pid, proc->carry, proc->last_live, proc->pc,
//            proc->instruction_time, proc->prev_time, proc->player);
//   if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
//     dump_nbytes(cpu, 5, pc, 1);
//   new = (short)read_mem_2(cpu->program, proc->pc + 1);
//   new %= (short)IDX_MOD;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: new(0x%02hx)(%d) pc(%d) INS_FORK read\n", new, new, pc);
//   proc->pc += 3;
//   cpu->spawn_process(cpu, proc, new + pc, proc->player);
//   if (f_verbose & OPT_INSTR)
//     printf("P% 5d | fork %d (%d)\n", proc->pid, new, pc + new);
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: clock(%zu) pid(%d) carry(%d) last_live(%d) pc(%d) "
//            "ins_time(%d) INS_FORK end\n",
//            cpu->clock, cpu->processes->pid, cpu->processes->carry,
//            cpu->processes->last_live, cpu->processes->pc,
//            cpu->processes->instruction_time);
//   next(cpu, proc);
//   return 1;
// }

// lld is the same as 'ld', but without the (% IDX_MOD). Modifies
// carry. 'lld 34,r3' loads the REG_SIZE bytes from address (PC + (34)) in
// register r3.
// TODO: validate pcb
int instruction_lld(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int ivar2;
  int uvar3;
  int uvar4;
  int uvar5;
  int local_2c;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 0xd);
  if (uvar4 != 0) {
    uvar4 = type_from_pcb(pcb, 0);
    uvar5 = size_from_pt(uvar4, 0xd);
    if (uvar4 == 4) {
      local_2c = (int)read_mem_4(cpu->program, proc->pc + 2);
    } else {
      local_2c = (short)read_mem_2(cpu->program, proc->pc + read_mem_2(cpu->program, proc->pc + 2));
    }
    uvar1 = read_mem_1(cpu->program, proc->pc + 2 + uvar5);
    ivar2 = validate_register(uvar1);
    if (ivar2 != 0) {
      if (f_verbose & OPT_INSTR) {
        printf("P% 5d | lld %d r%d\n", proc->pid, local_2c, uvar1);
      }
      mod_carry(proc, (local_2c == 0));
      write_reg(proc, uvar1, local_2c);
    }
  }
  ivar2 = proc->pc;
  uvar3 = size_from_pcb(pcb, 0xd);
  if (f_verbose & OPT_PCMOVE) {
    print_adv(cpu, proc, proc->pc + 2 + uvar3);
  }
  proc->opcode = 0;
  return (proc->pc + 2 + uvar3);
}
// int instruction_lld(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LLD start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   proc->pc += 1;
//   int r1;
//   int v1 = read_val_load(cpu, proc, get_param(par, 1));
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: v1(0x%08x)(%d) INS_LDI read_val_load\n", v1, v1);
//   r1 = (char)read_mem_1(cpu->program, proc->pc + v1);
//   write_reg(proc, r1, v1);
//   mod_carry(proc, v1 == 0);
//   proc->pc += 1;
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | lld %d r%d\n", proc->pid, v1, r1);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// lldi is the same as 'ldi', but without the (% IDX_MOD). Modifies
// carry. 'lldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3)), adding 4
// to this sum S. Read REG_SIZE bytes at address (PC + (S)), which are copied to
// 'r1'. TODO: fix broken
// TODO: validate pcb
int instruction_lldi(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int ivar2;
  int type;
  int uvar3;
  int uvar4;
  int uvar5;
  int local_40;
  int local_34;
  int local_c;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 0xe);
  if (uvar4 != 0) {
    uvar4 = type_from_pcb(pcb, 0);
    type = uvar4;
    uvar4 = size_from_pt(type, 0xe);
    ivar2 = uvar4;
    if (type == 1) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        if (f_verbose & OPT_PCMOVE) {
          type = proc->pc;
          uvar3 = size_from_pcb(pcb, 0xe);
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        type = proc->pc;
        local_c = size_from_pcb(pcb, 0xe);
        local_c = type + 2 + local_c;
        goto LAB_100007f68;
      }
      local_34 = read_reg(proc, uvar1);
    } else {
      if (type == 2) {
        local_34 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (short)read_mem_2(cpu->program, proc->pc + 2);
      }
    }
    uvar4 = type_from_pcb(pcb, 1);
    uvar5 = size_from_pt(uvar4, 0xe);
    if (uvar4 == 1) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        if (f_verbose & OPT_PCMOVE) {
          type = proc->pc;
          uvar3 = size_from_pcb(pcb, 0xe);
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        type = proc->pc;
        local_c = size_from_pcb(pcb, 0xe);
        local_c = type + 2 + local_c;
        goto LAB_100007f68;
      }
      local_40 = read_reg(proc, uvar1);
    } else {
      local_40 = (short)read_mem_2(cpu->program, proc->pc + 2 + ivar2);
    }
    uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2 + uvar5);
    type = validate_register(uvar1);
    if (type == 0) {
      if (f_verbose & OPT_PCMOVE) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 0xe);
        print_adv(cpu, proc, type + 2 + uvar3);
      }
      type = proc->pc;
      local_c = size_from_pcb(pcb, 0xe);
      local_c = type + 2 + local_c;
      goto LAB_100007f68;
    }
    uvar3 = read_mem_4(cpu->program, proc->pc + local_34 + local_40);
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | lldi %d %d r%d\n", proc->pid, local_34, local_40, uvar1);
    }
    if (f_verbose & OPT_INSTR) {
      printf("       | -> load from %d + %d = %d (with pc %d)\n", local_34, local_40, local_34+local_40, proc->pc+local_34+local_40);
    }
    mod_carry(proc, (uvar3 == 0));
    write_reg(proc, uvar1, uvar3);
  }
  if (f_verbose & OPT_PCMOVE) {
    type = proc->pc;
    uvar3 = size_from_pcb(pcb, 0xe);
    print_adv(cpu, proc, type + 2 + uvar3);
  }
  type = proc->pc;
  local_c = size_from_pcb(pcb, 0xe);
  local_c = type + 2 + local_c;
LAB_100007f68:
  proc->opcode = 0;
  return local_c;
}
// int instruction_lldi(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LLDI start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   int p1, p2, r1, r2, r3;
//   int op = cpu->program[proc->pc];
//   proc->pc += 1;
//   uint8_t par = cpu->program[proc->pc];
//   if (check_param(par, cpu->program[pc] - 1)) {
//     int size = param_total_size(par, 0x0e); // incorrect size for Gagnant
//     5424 proc->pc = proc->pc + size + 3; // for some reason, Gagnant 4830
//     prefers + 3 if (f_verbose & OPT_INTLDBG)
//       printf("DBG: bad par(%02hhx) size(%d) pc(%04x)(%d) mem(%02hhx)"
//              " INS_LLDI ret\n",
//              par, size + 1, proc->pc, proc->pc,
//              cpu->program[(proc->pc - 1) % MEM_SIZE]);
//     if (f_verbose & OPT_PCMOVE)
//       print_adv(cpu, proc->pc - pc, pc);
//     next(cpu, proc);
//     return 1;
//   }
//   proc->pc += 1;
//   p1 = get_param(par, 1);
//   p2 = get_param(par, 2);
//   r1 = (char)read_mem_1(cpu->program, proc->pc);
//   int v1 = read_val_idx(cpu, proc, p1, op);
//   r2 = (char)read_mem_1(cpu->program, proc->pc);
//   int v2 = read_val_idx(cpu, proc, p2, op);
//   int idx = v1 + v2 + pc;
//   int v3 = (int)read_mem_4(cpu->program, idx); // TODO: AAAAAAA?!?
//   r3 = (char)read_mem_1(cpu->program, proc->pc);
//   write_reg(proc, r3, v3);
//   mod_carry(proc, v3 == 0);
//   proc->pc += 1;
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | lldi", proc->pid);
//     if (p1 == REG_CODE)
//       printf(" r%d", r1);
//     else
//       printf(" %d", v1);
//     if (p2 == REG_CODE)
//       printf(" r%d", r2);
//     else
//       printf(" %d", v2);
//     printf(" r%d\n", r3);
//     printf("       | -> load from %d + %d = %d (with pc and mod %d)\n", v1,
//     v2,
//            v1 + v2, idx);
//   }
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

// lfork is the same as 'fork', but without the (% IDX_MOD).
// Modifies carry.
int instruction_lfork(struct s_cpu *cpu, struct s_process *proc) {
  short new_offset;

  new_offset = (short)read_mem_2(cpu->program, proc->pc + 1);
  /* DAT_100095cc: global var that could be flag for ncurses display
     mode */
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | lfork %d (%d)\n", proc->pid, new_offset,
           new_offset + proc->pc);
  }
  fork_process(cpu, proc, proc->pc + new_offset);
  if (f_verbose & OPT_PCMOVE) {
    print_adv(cpu, proc, proc->pc + 3);
  }
  proc->opcode = 0;
  return (proc->pc + 3);
}
// int instruction_lfork(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LFORK start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   proc->pc += 1;
//   short idx = (short)read_mem_2(cpu->program, proc->pc);
//   proc->pc += 2;
//   cpu->spawn_process(cpu, proc, (pc + idx) % MEM_SIZE, *proc->registers);
//   if (f_verbose & OPT_INSTR) {
//     printf("P% 5d | lfork %d (%d)\n", proc->pid, idx, idx + proc->pc - 2);
//     // print_adv(cpu, proc->pc - pc, pc);
//   }
//   next(cpu, proc);
//   return 1;
// }

// aff takes a register and writes the stored value modulo 256 to
// stdout. 'ld %52,r3  aff r3' displays '*' on stdout.
// TODO: validate pcb
int instruction_aff(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t uvar1;
  int ivar2;
  int uvar3;
  int uvar4;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 0x10);
  if (uvar4 != 0) {
    uvar1 = read_mem_1(cpu->program, proc->pc + 2);
    ivar2 = validate_register(uvar1);
    if ((ivar2 != 0) &&
        ((uvar1 = read_reg(proc, uvar1)), f_enable_aff != 0)) {
      printf("Aff: %c\n", uvar1);
    }
  }
  if (f_verbose & OPT_PCMOVE) {
    ivar2 = proc->pc;
    uvar3 = size_from_pcb(pcb, 0x10);
    print_adv(cpu, proc, ivar2 + 2 + uvar3);
  }
  ivar2 = proc->pc;
  uvar3 = size_from_pcb(pcb, 0x10);
  proc->opcode = 0;
  return (ivar2 + 2 + uvar3);
}
// int instruction_aff(struct s_cpu *cpu, struct s_process *proc) {
//   int pc = proc->pc;
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_AFF start\n",
//            proc->pid, proc->carry, proc->last_live, proc->pc);
//   proc->pc += 1;
//   // uint8_t par = cpu->program[proc->pc];
//   proc->pc += 1;
//   int val = read_reg(proc, (char)read_mem_1(cpu->program, proc->pc));
//   // ft_putchar(val & 0xff);
//   // ft_putchar('\n');
//   proc->pc += 1;
//   if (f_verbose & OPT_INSTR)
//     printf("Aff: %c\n", val & 0xff);
//   if (f_verbose & OPT_PCMOVE)
//     print_adv(cpu, proc->pc - pc, pc);
//   next(cpu, proc);
//   return 1;
// }

t_inst inst_tab[NUM_OPS + 1] = {
    (t_inst)0,
    [e_live] = instruction_live,
    [e_ld] = instruction_ld,
    [e_st] = instruction_st,
    [e_add] = instruction_add,
    [e_sub] = instruction_sub,
    [e_and] = instruction_and,
    [e_or] = instruction_or,
    [e_xor] = instruction_xor,
    [e_zjmp] = instruction_zjmp,
    [e_ldi] = instruction_ldi,
    [e_sti] = instruction_sti,
    [e_fork] = instruction_fork,
    [e_lld] = instruction_lld,
    [e_lldi] = instruction_lldi,
    [e_lfork] = instruction_lfork,
    [e_aff] = instruction_aff,
};
