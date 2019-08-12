#include "cpu.h"
#include "op.h"
#include "util.h"
#include <stdio.h>

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

extern header_t h;
void print_adv(struct s_cpu *cpu, int len, int initial_pc) {
  int ii = 0;
  printf("ADV %d (0x%04x -> 0x%04x) ", len, initial_pc, initial_pc + len);
  while (ii < len) {
    printf((!ii ? "%02x" : " %02x"),
           read_mem_byte(cpu->program, initial_pc + ii));
    ++ii;
  }
  printf("\n");
}

// next goes to the next instruction and sets the execution time.
void next(struct s_cpu *cpu, struct s_process *proc) {
  if (!cpu || !proc) {
    if (f_verbose & OPT_INTLDBG)
      fprintf(stderr, "ERROR: cpu or proc NULL in next()\n");
    return;
  }
  if (proc->pc >= MEM_SIZE)
    proc->pc %= (MEM_SIZE);
  if (proc->pc < 0)
    proc->pc += MEM_SIZE;
  int instruction = cpu->program[proc->pc];
  proc->opcode = cpu->program[proc->pc];

  if (instruction >= e_live && instruction <= e_aff) {
    proc->instruction_time = g_op_tab[instruction - 1].cycles_to_exec - 1;
  } else {
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_DBGOUT))
      puts("NOP: next instruction is an error");
    proc->instruction_time = 0;
  }
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: clock(%zu) pid(%d) carry(%d) last_live(%d) pc(%d) "
           "ins_time(%d) ins(%02x) NEXT end\n",
           cpu->clock, proc->pid, proc->carry, proc->last_live, proc->pc,
           proc->instruction_time, instruction);
}

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
    val = read_mem_byte(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val reg val(%08x)\n", val);
    proc->pc += 1;
  } else if (arg == DIR_CODE) {
    val = read_mem_long(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val dir val(%08x)\n", val);
    proc->pc += 4;
  } else if (arg == IND_CODE) {
    idx = read_mem_word(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val ind idx(%04x)\n", idx);
    val = read_mem_long(cpu->program, idx + proc->pc);
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
    reg = read_mem_byte(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_idx REG reg(%02x)\n", reg);
    val = read_reg(proc, reg);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_idx REG val(%08x)\n", val);
    proc->pc += 1;
  } else if (arg == DIR_CODE) {
    if (g_op_tab[op - 1].direct_size)
      val = read_mem_word(cpu->program, proc->pc);
    else
      val = read_mem_long(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG)) {
      if (g_op_tab[op - 1].direct_size)
        printf("DBG: read_val_idx DIR val(%04x)\n", val);
      else
        printf("DBG: read_val_idx DIR val(%08x)\n", val);
    }
    proc->pc += (g_op_tab[op - 1].direct_size ? 2 : 4);
  } else if (arg == IND_CODE) {
    idx = read_mem_word(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_idx IND idx(0x%04x)(%d)\n", idx, idx);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      dump_nbytes(cpu, 6, idx + proc->pc, 1);
    val = read_mem_long(cpu->program, idx + proc->pc);
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
    reg = read_mem_byte(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load REG reg(%02x)\n", reg);
    val = read_reg(proc, reg);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load REG val(%08x)\n", val);
    proc->pc += 1;
  } else if (arg == DIR_CODE) {
    val = read_mem_long(cpu->program, proc->pc);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load DIR val(%08x)\n", val);
    proc->pc += 4;
  } else if (arg == IND_CODE) {
    idx = read_mem_word(cpu->program, proc->pc) & (IDX_MOD - 1);
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_val_load IND idx(%08x)\n", idx);
    val = read_mem_long(cpu->program, proc->pc + idx - 2);
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
  int pc = proc->pc;
  int player = 0;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LIVE start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  cpu->nbr_lives++;
  proc->last_live = cpu->clock;
  proc->pc += 1;
  int v1 = read_mem_long(cpu->program, proc->pc);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(%08x)(%d) INS_LIVE read\n", v1, v1);
  proc->pc += 4;
  if (v1 >= -4 && v1 <= -1) {
    player = -v1;
    cpu->lastlive[player - 1] = cpu->clock;
    cpu->winner = player;
  }
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%d) v1(%08x) pid(%d) last_live(%d) nbr_lives(%d) "
           "INS_LIVE end\n",
           proc->pc, v1, proc->pid, proc->last_live, cpu->nbr_lives);
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | live %d\n", proc->pid, v1);
  if ((f_verbose & OPT_LIVES) && player > 0)
    printf("Player %d (%s) is said to be alive\n", player,
           cpu->players[player - 1].name);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// ld takes 2 parameters, 2nd must be a register that isn't the
// 'program counter'. It loads the value of the first parameter in the register,
// and modifies the 'carry'. 'ld 34,r3' loads the REG_SIZE bytes from address
// (PC + (34 % IDX_MOD)) in register r3.
int instruction_ld(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LD start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  int val;
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  if (check_param(par, cpu->program[pc] - 1)) {
    if (f_verbose & OPT_INTLDBG)
      dprintf(STDERR_FILENO, "DBG: par(%02hhx) invalid param INS_LD ret\n",
              par);
    proc->pc += 3;
    return 1;
  }
  proc->pc += 1;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%d) INS_LD\n", proc->pc);
  val = read_val_load(cpu, proc, get_param(par, 1));
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%d) val(%08x) INS_LD\n", proc->pc, val);
  uint8_t reg = read_mem_byte(cpu->program, proc->pc);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%d) reg(%02hhx) INS_LD\n", proc->pc, reg);
  write_reg(proc, reg, val);
  mod_carry(proc, (val == 0));
  proc->pc += 1;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%d) INS_LD end\n", proc->pc);
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | ld %d r%d\n", proc->pid, val, reg);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// st takes 2 parameters, storing (REG_SIZE bytes) of the value of
// the first argument (always a register) in the second. 'st r4,34' stores the
// value of 'r4' at the address (PC + (34 % IDX_MOD)) 'st r3,r8' copies the
// contents of 'r3' to 'r8'
int instruction_st(struct s_cpu *cpu, struct s_process *proc) {
  int v1, r1;
  short v2;
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ST start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  if (check_param(par, cpu->program[pc] - 1)) {
    int size = param_total_size(par, 3);
    proc->pc = proc->pc + size + 1;
    if (f_verbose & OPT_INTLDBG)
      printf("DBG: par(%02hhx) size(%d) pc(%04x)(%d) mem(%02hhx) invalid param"
             " INS_ST ret\n", par, size + 1, proc->pc, proc->pc,
             cpu->program[(proc->pc-1) % MEM_SIZE]);
    if (f_verbose & OPT_PCMOVE)
      print_adv(cpu, proc->pc - pc, pc);
    return 1;
  }
  proc->pc += 1;
  int p1 = get_param(par, 1);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: p1(%d) p1 == REG_CODE %d INS_ST\n", p1, p1 == REG_CODE);
  r1 = read_val(cpu, proc, p1);
  v1 = read_reg(proc, r1);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%d) mem(%02x) v1(%08x) INS_ST\n", proc->pc,
           cpu->program[proc->pc], v1);
  if (get_param(par, 2) == REG_CODE) {
    if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
      dump_nbytes(cpu, 7, pc, 1);
    v2 = read_mem_byte(cpu->program, proc->pc);
    write_reg(proc, v2, v1);
    proc->pc += 1;
  } else {
    if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
      dump_nbytes(cpu, 5, pc, 1);
    v2 = read_mem_word(cpu->program, proc->pc);
    if (f_verbose & OPT_INTLDBG)
      printf("DBG: ind(%04hx) idx(%d) INS_ST\n", v2, (pc + ((v2) % IDX_MOD)));
    write_mem_ins(cpu->program, pc + ((v2) % IDX_MOD), v1);
    proc->pc += 2;
  }
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | st r%d ", proc->pid, r1);
    if (get_param(par, 2) == REG_CODE)
      printf("r%d\n", v2);
    else
      printf("%d\n", v2);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// add takes 3 registers as parameters, adding the contents of the
// first and second, storing the result into the third. Modifies carry. 'add
// r2,r3,r5' adds the values of 'r2' and 'r3' and stores the result in 'r5'.
int instruction_add(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ADD start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  int r1 = read_val(cpu, proc, get_param(par, 1));
  int r2 = read_val(cpu, proc, get_param(par, 2));
  int r3 = read_val(cpu, proc, get_param(par, 3));
  int v1 = read_reg(proc, r1);
  int v2 = read_reg(proc, r2);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(%08x) v2(%08x) INS_ADD\n", v1, v2);
  write_reg(proc, r3, v1 + v2);
  mod_carry(proc, (v1 + v2) == 0);
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | add r%d r%d r%d\n", proc->pid, r1, r2, r3);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// sub is the same as instruction_add, except performs subtraction.
int instruction_sub(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_SUB start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  int r1 = read_val(cpu, proc, get_param(par, 1));
  int r2 = read_val(cpu, proc, get_param(par, 2));
  int r3 = read_val(cpu, proc, get_param(par, 3));
  int v1 = read_reg(proc, r1);
  int v2 = read_reg(proc, r2);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(%08x) v2(%08x) INS_SUB\n", v1, v2);
  write_reg(proc, r3, v1 - v2);
  mod_carry(proc, (v1 - v2) == 0);
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | sub r%d r%d r%d\n", proc->pid, r1, r2, r3);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// perform a bitwise AND on the first two parameters, storing theresult into the
// third which is always a register. Modifies carry.'and r2,%0,r3' stores 'r2 &
// 0' into 'r3'.
int instruction_and(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_AND start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  int v1, v2, r1, r2;
  if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
    dump_nbytes(cpu, 9, proc->pc, 1);
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = v1 = read_val(cpu, proc, p1);
    if (p1 == REG_CODE)
      v1 = read_reg(proc, v1);
  } else {
    v1 = read_mem_word(cpu->program, proc->pc);
    v1 = read_mem_long(cpu->program, v1 + proc->pc - 2);
    proc->pc += 2;
  }
  if (p2 != IND_CODE) {
    r2 = v2 = read_val(cpu, proc, p2);
    if (p2 == REG_CODE)
      v2 = read_reg(proc, v2);
  } else {
    v2 = read_mem_word(cpu->program, proc->pc);
    v2 = read_mem_long(cpu->program,
                       v2 + proc->pc - 2 - (p1 == IND_CODE ? 2 : 0));
    proc->pc += 2;
  }
  int v3 = read_val(cpu, proc, get_param(par, 3));
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(%08x) v2(%08x) v3(%08x) INS_AND\n", v1, v2, v3);
  write_reg(proc, v3, v1 & v2);
  mod_carry(proc, (v1 & v2) == 0);
  if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
    dump_nbytes(cpu, 9, proc->pc, 1);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%d) INS_AND end\n", proc->pc);
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | and ", proc->pid);
    if (p1 == REG_CODE)
      printf("r%d ", r1);
    else
      printf("%d ", v1);
    if (p2 == REG_CODE)
      printf("r%d ", r2);
    else
      printf("%d ", v2);
    printf("r%d\n", v3);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// or is the same as instruction_and, except performs a bitwise OR.
int instruction_or(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_OR start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  int v1, v2, r1, r2;
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = v1 = read_val(cpu, proc, p1);
    if (p1 == REG_CODE)
      v1 = read_reg(proc, v1);
  } else {
    v1 = read_mem_word(cpu->program, proc->pc);
    v1 = read_mem_long(cpu->program, v1 + proc->pc - 2);
    proc->pc += 2;
  }
  if (p2 != IND_CODE) {
    r2 = v2 = read_val(cpu, proc, p2);
    if (p2 == REG_CODE)
      v2 = read_reg(proc, v2);
  } else {
    v2 = read_mem_word(cpu->program, proc->pc);
    v2 = read_mem_long(cpu->program,
                       v2 + proc->pc - 2 - (p1 == IND_CODE ? 2 : 0));
    proc->pc += 2;
  }
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(%08x) v2(%08x) INS_OR\n", v1, v2);
  write_reg(proc, read_val(cpu, proc, get_param(par, 3)), v1 | v2);
  mod_carry(proc, (v1 | v2) == 0);
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | and ", proc->pid);
    if (p1 == REG_CODE)
      printf("r%d ", r1);
    else
      printf("%d", v1);
    if (p2 == REG_CODE)
      printf("r%d", r2);
    else
      printf("%d", v2);
    printf("\n");
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// xor is the same as instruction_and, except performs a bitwise
// XOR.
int instruction_xor(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_XOR start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  int v1, v2, r1, r2;
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = read_mem_byte(cpu->program, proc->pc);
    v1 = read_val_load(cpu, proc, p1);
  } else {
    v1 = read_mem_word(cpu->program, proc->pc);
    v1 = read_mem_long(cpu->program, v1 + proc->pc - 2);
    proc->pc += 2;
  }
  if (p2 != IND_CODE) {
    r2 = read_mem_byte(cpu->program, proc->pc);
    v2 = read_val_load(cpu, proc, p2);
  } else {
    v2 = read_mem_word(cpu->program, proc->pc);
    v2 = read_mem_long(cpu->program,
                       v2 + proc->pc - 2 - (p1 == IND_CODE ? 2 : 0));
    proc->pc += 2;
  }
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(%08x) v2(%08x) INS_XOR\n", v1, v2);
  write_reg(proc, read_val(cpu, proc, get_param(par, 3)), v1 ^ v2);
  mod_carry(proc, (v1 ^ v2) == 0);
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | and ", proc->pid);
    if (p1 == REG_CODE)
      printf("r%d ", r1);
    else
      printf("%d", v1);
    if (p2 == REG_CODE)
      printf("r%d", r2);
    else
      printf("%d", v2);
    printf("\n");
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// zjmp always takes an index (IND_SIZE) and makes a jump at this
// index if carry is true, otherwise consuming cycles. 'zjmp %23' stores (PC +
// (23 % IDX_MOD)) into PC.
int instruction_zjmp(struct s_cpu *cpu, struct s_process *proc) {
  ; // TODO: better instruction_zjmp regression tests
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ZJMP start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  proc->pc += 1;
  short v1 = read_mem_word(cpu->program, proc->pc);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: carry(%d) v1(%04hx)(%d) INS_ZJMP\n", proc->carry, v1,
           v1 % IDX_MOD);
  if (proc->carry == true) {
    proc->pc = ((proc->pc + ((v1 - 1) % IDX_MOD)) % MEM_SIZE);
    if (f_verbose & OPT_INTLDBG)
      printf("DBG: jumping to pc(%d) INS_ZJMP\n", proc->pc);
  } else {
    proc->pc += 2;
  }
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | zjmp %d %s\n", proc->pid, v1,
           (proc->carry ? "OK" : "FAILED"));
  }
  if ((f_verbose & OPT_PCMOVE) && !proc->carry)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// ldi modifies carry. 'idx' and 'add' are indexes, and 'reg' is a
// register. 'ldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3 %
// IDX_MOD)), adding 4 to this sum S. Read REG_SIZE bytes at address (PC + (S %
// IDX_MOD)), which are copied to 'r1'.
int instruction_ldi(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LDI start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  int v1, v2, v3, idx, r1, r2;
  int op = cpu->program[proc->pc];
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  if (check_param(par, op - 1)) {
    if (f_verbose & OPT_INTLDBG)
      dprintf(STDERR_FILENO, "DBG: par(%02hhx) invalid param INS_LDI ret\n",
              par);
    proc->pc += 3;
    return 1;
  }
  proc->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = read_mem_byte(cpu->program, proc->pc);
    v1 = read_val_idx(cpu, proc, p1, op);
  } else {
    v1 = read_mem_word(cpu->program, proc->pc);
    proc->pc += 2;
    v1 = read_mem_long(cpu->program, v1 + pc);
  }
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(0x%08x)(%d) INS_LDI\n", v1, v1);
  r2 = read_mem_byte(cpu->program, proc->pc);
  if (p2 == REG_CODE)
    v2 = read_val_idx(cpu, proc, p2, op);
  else
    v2 = (short)read_val_idx(cpu, proc, p2, op);
  idx = (pc + (v1 % IDX_MOD)) + v2;
  if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
    dump_nbytes(cpu, 4, idx, 1);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: load from %d + %d = %d [with pc and mod=idx(%d)] INS_LDI\n",
           v1, v2, v1 + v2, idx);
  v3 = read_mem_long(cpu->program, idx);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v3(%08x) INS_LDI\n", v3);
  char reg = read_mem_byte(cpu->program, proc->pc);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: reg(%d) v1(%08x) v2(%08x) v3(%08x) INS_LDI write_reg\n", reg,
           v1, v2, v3);
  write_reg(proc, reg, v3);
  proc->pc += 1;
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | ldi", proc->pid);
    if (p1 == REG_CODE)
      printf(" r%d", r1);
    else
      printf(" %d", v1);
    if (p2 == REG_CODE)
      printf(" r%d", r2);
    else
      printf(" %d", v2);
    printf(" r%d\n", reg);
    printf("       | -> load from %d + %d = %d (with pc and mod %d)\n", v1, v2,
           v1 + v2, idx);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// sti does something probably. 'sti r2,%4,%5' copies REG_SIZE bytes
// of 'r2' at address (4 + 5) Parameters 2 and 3 are treated as indexes.
int instruction_sti(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_STI start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  int v1, v2, v3, idx, p1, p2, p3, r1;
  int op = proc->opcode;
  if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
    dump_nbytes(cpu, 11, pc, 1);
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  if (check_param(par, op - 1)) {
    if (f_verbose & OPT_INTLDBG)
      dprintf(2, "DBG: baf INS_STI bad arg\n");
    proc->pc += 7;
    if (f_verbose & OPT_PCMOVE)
      print_adv(cpu, proc->pc - pc, pc);
    next(cpu, proc);
    return (1);
  }
  proc->pc += 1;
  p1 = get_param(par, 1);
  p2 = get_param(par, 2);
  p3 = get_param(par, 3);
  r1 = read_mem_byte(cpu->program, proc->pc);
  /* v1 = read_reg(proc, r1);
  proc->pc += 1;
  if (p2 != IND_CODE) {
    r2 = read_mem_byte(cpu->program, proc->pc);
    v2 = read_val_idx(cpu, proc, p2, op);
  } else {
    v2 = read_mem_word(cpu->program, proc->pc);
    proc->pc += 2;
    v2 = read_mem_long(cpu->program, v2 + pc);
  }
  if (p3 == REG_CODE) {
    r3 = read_mem_byte(cpu->program, proc->pc);
    proc->pc += 1;
    v3 = read_reg(proc, r3);
  } else {
    v3 = read_mem_word(cpu->program, proc->pc);
  } */
  v1 = read_val_idx(cpu, proc, get_param(par, 1), op);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(%08x)(%d) INS_STI\n", v1, v1);
  if (p2 == DIR_CODE)
    v2 = (short)read_val_idx(cpu, proc, p2, op);
  else
    v2 = read_val_idx(cpu, proc, p2, op);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v2(%08x)(%d) INS_STI\n", v2, v2);
  if (p3 == DIR_CODE)
    v3 = (short)read_val_idx(cpu, proc, p3, op);
  else
    v3 = read_val_idx(cpu, proc, p3, op);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v3(%08x)(%d) INS_STI\n", v3, v3);
  idx = ((pc + (v2 + v3) % IDX_MOD));
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: store to %d + %d = %d [with pc and mod=idx(%d)] INS_STI\n", v2,
           v3, v2 + v3, idx);
  write_mem_ins(cpu->program, idx, v1);
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | sti r%d", proc->pid, r1);
    // if (p2 == REG_CODE)
    //   printf(" r%d", r2);
    // else
    printf(" %d", v2);
    // if (p3 == REG_CODE)
    //   printf(" r%d\n", r3);
    // else
    printf(" %d\n", v3);
    printf("       | -> store to %d + %d = %d (with pc and mod %d)\n", v2, v3,
           v2 + v3, idx);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// fork always takes an index and creates a new program which is
// executed from address (PC + ('idx' % IDX_MOD)). 'fork %34' spawns a new
// process at (PC + (34 % IDX_MOD)). helltrain cycles (1105,1935,2745,3555,4365)
// TOOD: handle process calling fork correctly: cycle 2745,
//       pid(3).instruction_time is decremented twice
int instruction_fork(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;
  short new;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_FORK start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  if ((f_verbose & OPT_DBGOUT) && (f_verbose & OPT_INTLDBG))
    dump_nbytes(cpu, 5, pc, 1);
  new = (short)read_mem_word(cpu->program, proc->pc + 1);
  new %= (short)IDX_MOD;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: new(0x%02hx)(%d) pc(%d) INS_FORK read\n", new, new, pc);
  proc->pc += 3;
  cpu->spawn_process(cpu, proc, new + pc, *proc->registers);
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | fork %d (%d)\n", proc->pid, new, pc + new);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  /* if (f_verbose & OPT_INTLDBG)
    printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
           "pc(%4d) ins_time(%4d) INS_FORK prev_head start\n",
           cpu->clock, prev_head->pid, prev_head->carry, prev_head->last_live,
           prev_head->pc, prev_head->instruction_time);
  if (prev_head != 0 && prev_head->instruction_time != 0)
    prev_head->instruction_time++;
  else {
    uint8_t op = read_mem_byte(cpu->program, prev_head->pc);
    if (op >= 1 && op <= 16)
      prev_head->instruction_time = g_op_tab[op - 1].cycles_to_exec;
    else
      prev_head->instruction_time = 1;
  }
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
           "pc(%4d) ins_time(%4d) INS_FORK prev_head end\n",
           cpu->clock, prev_head->pid, prev_head->carry, prev_head->last_live,
           prev_head->pc, prev_head->instruction_time); */
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: clock(%zu) pid(%d) carry(%d) last_live(%d) pc(%d) "
           "ins_time(%d) INS_FORK end\n",
           cpu->clock, cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc,
           cpu->processes->instruction_time);
  next(cpu, proc);
  return 1;
}

// lld is the same as 'ld', but without the (% IDX_MOD). Modifies
// carry. 'lld 34,r3' loads the REG_SIZE bytes from address (PC + (34)) in
// register r3.
int instruction_lld(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LLD start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  int r1;
  int v1 = read_val_load(cpu, proc, get_param(par, 1));
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: v1(0x%08x)(%d) INS_LDI read_val_load\n", v1, v1);
  r1 = read_mem_byte(cpu->program, proc->pc + v1);
  write_reg(proc, r1, v1);
  mod_carry(proc, v1 == 0);
  proc->pc += 1;
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | lld %d r%d\n", proc->pid, v1, r1);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// lldi is the same as 'ldi', but without the (% IDX_MOD). Modifies
// carry. 'lldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3)), adding 4
// to this sum S. Read REG_SIZE bytes at address (PC + (S)), which are copied to
// 'r1'
int instruction_lldi(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LLDI start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  int p1, p2, r1, r2, r3;
  int op = cpu->program[proc->pc];
  proc->pc += 1;
  uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  p1 = get_param(par, 1);
  p2 = get_param(par, 2);
  r1 = read_mem_byte(cpu->program, proc->pc);
  int v1 = read_val_idx(cpu, proc, p1, op);
  r2 = read_mem_byte(cpu->program, proc->pc);
  int v2 = read_val_idx(cpu, proc, p2, op);
  int idx = v1 + v2 + pc;
  int v3 = read_mem_long(cpu->program, idx); // TODO: AAAAAAA?!?
  r3 = read_mem_byte(cpu->program, proc->pc);
  write_reg(proc, r3, v3);
  mod_carry(proc, v3 == 0);
  proc->pc += 1;
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | lldi", proc->pid);
    if (p1 == REG_CODE)
      printf(" r%d", r1);
    else
      printf(" %d", v1);
    if (p2 == REG_CODE)
      printf(" r%d", r2);
    else
      printf(" %d", v2);
    printf(" r%d\n", r3);
    printf("       | -> load from %d + %d = %d (with pc and mod %d)\n", v1, v2,
           v1 + v2, idx);
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}

// lfork is the same as 'fork', but without the (% IDX_MOD).
// Modifies carry.
int instruction_lfork(struct s_cpu *cpu, struct s_process *proc) {
  int pc = proc->pc;

  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LFORK start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  proc->pc += 1;
  short idx = read_mem_word(cpu->program, proc->pc);
  proc->pc += 2;
  cpu->spawn_process(cpu, proc, (pc + idx) % MEM_SIZE, *proc->registers);
  if (f_verbose & OPT_INSTR) {
    printf("P% 5d | lfork %d (%d)\n", proc->pid, idx, idx + proc->pc - 2);
    // print_adv(cpu, proc->pc - pc, pc);
  }
  next(cpu, proc);
  return 1;
}

// aff takes a register and writes the stored value modulo 256 to
// stdout. 'ld %52,r3  aff r3' displays '*' on stdout.
int instruction_aff(struct s_cpu *cpu, struct s_process *proc) {

  int pc = proc->pc;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_AFF start\n",
           proc->pid, proc->carry, proc->last_live, proc->pc);
  proc->pc += 1;
  // uint8_t par = cpu->program[proc->pc];
  proc->pc += 1;
  int val = read_reg(proc, read_mem_byte(cpu->program, proc->pc));
  // ft_putchar(val & 0xff);
  // ft_putchar('\n');
  proc->pc += 1;
  if (f_verbose & OPT_INSTR)
    printf("Aff: %c\n", val & 0xff);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc->pc - pc, pc);
  next(cpu, proc);
  return 1;
}
