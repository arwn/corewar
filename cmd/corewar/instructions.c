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

  if ((g_op_tab[op].argtypes[0] & p1) != p1) {
    ret = 1;
    // printf("DBG: p1(%d) bad\n", p1);
  }
  if ((g_op_tab[op].argtypes[1] & p2) != p2) {
    ret = 1;
    // printf("DBG: p2(%d) bad\n", p2);
  }
  if ((g_op_tab[op].argtypes[2] & p3) != p3) {
    ret = 1;
    // printf("DBG: p3(%d) bad\n", p3);
  }
  if ((g_op_tab[op].argtypes[3] & p4) != p4) {
    ret = 1;
    // printf("DBG: p4(%d) bad\n", p4);
  }
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
  printf("ADV %d (0x%04x -> 0x%04x) ", len, initial_pc, cpu->processes->pc);
  while (ii < len) {
    printf((!ii ? "%02x" : " %02x"),
           read_mem_byte(cpu->program, initial_pc + ii));
    ++ii;
  }
  printf("\n");
}

// next goes to the next instruction and sets the execution time.
void next(struct s_cpu *cpu) {
  if (!cpu || !cpu->processes) {
    if (f_verbose >= 2)
      fprintf(stderr, "ERROR: cpu or cpu->processes NULL in next()\n");
    return;
  }
  if (cpu->processes->pc >= MEM_SIZE)
    cpu->processes->pc %= (MEM_SIZE);
  if (cpu->processes->pc < 0)
    cpu->processes->pc += MEM_SIZE;
  int instruction = cpu->program[cpu->processes->pc];

  if (instruction >= e_live && instruction <= e_aff) {
    cpu->processes->instruction_time =
        g_op_tab[instruction - 1].cycles_to_exec - 1;
  } else {
    if (f_verbose >= 4)
      puts("NOP: next instruction is an error");
    cpu->processes->instruction_time = 0;
  }
  if (f_verbose >= 3)
    printf("DBG: clock(%zu) pid(%d) carry(%d) last_live(%d) pc(%d) "
           "ins_time(%d) ins(%02x) NEXT end\n",
           cpu->clock, cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc,
           cpu->processes->instruction_time, instruction);
}

/* Utility functions */

// Write VAL into MEM[IDX]
void write_mem_ins(uint8_t *mem, uint32_t idx, uint32_t val) {
  if (f_verbose >= 4)
    printf("DBG: write_mem_ins idx(%08x) val(%08x)\n", idx, val);
  mem[idx % MEM_SIZE] = (val >> 24) & 0xff;
  mem[(idx + 1) % MEM_SIZE] = (val >> 16) & 0xff;
  mem[(idx + 2) % MEM_SIZE] = (val >> 8) & 0xff;
  mem[(idx + 3) % MEM_SIZE] = val & 0xff;
}

// Write VAL into register REG for the current process in CPU
void write_reg(struct s_cpu *cpu, uint32_t reg, uint32_t val) {
  if (f_verbose >= 4)
    printf("DBG: write_reg reg(%08x) val(%08x) lav(%08x)\n", reg, val,
           ntohl(val));
  if (reg > 0 && reg <= REG_NUMBER)
    cpu->processes->registers[reg - 1] = ntohl(val);
}

// Read register REG for the current process in CPU
int read_reg(struct s_cpu *cpu, uint32_t reg) {
  if (reg > 0 && reg <= REG_NUMBER) {
    int ret = cpu->processes->registers[reg - 1];
    if (f_verbose >= 4)
      printf("DBG: read_reg reg(%08x) ger(%08x)\n", ret, ntohl(ret));
    return ntohl(ret);
  }
  return 0;
}

// Parse the argument size specified by ARG, read accordingly, and then return
// VAL
int read_val(struct s_cpu *cpu, int arg) {
  int val = 0;
  int idx;

  if (f_verbose >= 4)
    printf("DBG: read_val arg(%d) pc(%d)\n", arg, cpu->processes->pc);
  if (arg == REG_CODE) {
    val = read_mem_byte(cpu->program, cpu->processes->pc);
    if (f_verbose >= 3)
      printf("DBG: read_val reg val(%08x)\n", val);
    cpu->processes->pc += 1;
  } else if (arg == DIR_CODE) {
    val = read_mem_long(cpu->program, cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val dir val(%08x)\n", val);
    cpu->processes->pc += 4;
  } else if (arg == IND_CODE) {
    idx = read_mem_word(cpu->program, cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val ind idx(%04x)\n", idx);
    val = read_mem_long(cpu->program, idx + cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val ind val(%08x)\n", val);
    cpu->processes->pc += 2;
  }
  if (f_verbose >= 4)
    printf("DBG: read_val end pc(%d)\n", cpu->processes->pc);
  return (val);
}

// Modify the carry flag for the current process based on VAL
void mod_carry(struct s_cpu *cpu, int val) {
  if (val == 0) {
    cpu->processes->carry = 1;
  } else {
    cpu->processes->carry = 0;
  }
}

// Read the specified ARG, adjusting size based on OP direct_size member in
// G_OP_TAB. Used primarily in LDI, STI, & LLDI
int read_val_idx(struct s_cpu *cpu, int arg, int op) {
  int reg;
  int val = 0;
  int idx;

  if (f_verbose >= 4)
    printf("DBG: read_val_idx arg(%d) pc(%d)\n", arg, cpu->processes->pc);
  if (arg == REG_CODE) {
    reg = read_mem_byte(cpu->program, cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val_idx REG reg(%02x)\n", reg);
    val = read_reg(cpu, reg);
    if (f_verbose >= 4)
      printf("DBG: read_val_idx REG val(%08x)\n", val);
    cpu->processes->pc += 1;
  } else if (arg == DIR_CODE) {
    if (g_op_tab[op - 1].direct_size)
      val = read_mem_word(cpu->program, cpu->processes->pc);
    else
      val = read_mem_long(cpu->program, cpu->processes->pc);
    if (f_verbose >= 4) {
      if (g_op_tab[op - 1].direct_size)
        printf("DBG: read_val_idx DIR val(%04x)\n", val);
      else
        printf("DBG: read_val_idx DIR val(%08x)\n", val);
    }
    cpu->processes->pc += (g_op_tab[op - 1].direct_size ? 2 : 4);
  } else if (arg == IND_CODE) {
    idx = read_mem_word(cpu->program, cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val_idx IND idx(0x%04x)(%d)\n", idx, idx);
    if (f_verbose >= 4)
      dump_nbytes(cpu, 6, idx + cpu->processes->pc, 1);
    val = read_mem_long(cpu->program, idx + cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val_idx IND val(0x%08x)(%d)\n", val, val);
    cpu->processes->pc += 2;
  }
  if (f_verbose >= 4)
    printf("DBG: read_val_idx end pc(%d)\n", cpu->processes->pc);
  return val;
}

// Read the specified ARG, reading the current value of a register if necessary
int read_val_load(struct s_cpu *cpu, int arg) {
  int reg;
  int val = 0;
  int idx;

  if (f_verbose >= 4)
    printf("DBG: read_val_load arg(%d) pc(%d)\n", arg, cpu->processes->pc);
  if (f_verbose >= 4)
    dump_nbytes(cpu, 8, cpu->processes->pc, 1);
  if (arg == REG_CODE) {
    reg = read_mem_byte(cpu->program, cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val_load REG reg(%02x)\n", reg);
    val = read_reg(cpu, reg);
    if (f_verbose >= 4)
      printf("DBG: read_val_load REG val(%08x)\n", val);
    cpu->processes->pc += 1;
  } else if (arg == DIR_CODE) {
    val = read_mem_long(cpu->program, cpu->processes->pc);
    if (f_verbose >= 4)
      printf("DBG: read_val_load DIR val(%08x)\n", val);
    cpu->processes->pc += 4;
  } else if (arg == IND_CODE) {
    idx = read_mem_word(cpu->program, cpu->processes->pc) & (IDX_MOD - 1);
    if (f_verbose >= 4)
      printf("DBG: read_val_load IND idx(%08x)\n", idx);
    val = read_mem_long(cpu->program, cpu->processes->pc + idx - 2);
    if (f_verbose >= 4)
      printf("DBG: read_val_load IND val(%08x)\n", val);
    cpu->processes->pc += 2;
  }
  if (f_verbose >= 4)
    printf("DBG: read_val_load end pc(%d)\n", cpu->processes->pc);
  return val;
}

/* Instructions */

// Stores the current cycle into the lastlive array in CPU, if the NAME
// corresponds to an active player
int instruction_live(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LIVE start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->nbr_lives++;
  int name;
  /* TODO: check next four bytes against list of player names,
  and mark as living accordingly. */
  cpu->processes->pc += 1;
  name = read_mem_long(cpu->program, cpu->processes->pc);
  cpu->processes->pc += 4;
  int player = abs(name) - 1;
  if (name >= -4 && name <= -1 && player >= 0 && player <= 3) {
    cpu->lastlive[player] = cpu->clock;
    cpu->processes->last_live = cpu->clock;
  }
  if (f_verbose >= 3)
    printf("DBG: pc(%d) name(%08x) pid(%d) last_live(%d) nbr_lives(%d) "
           "INS_LIVE end\n",
           cpu->processes->pc, name, cpu->processes->pid,
           cpu->processes->last_live, cpu->nbr_lives);
  if (f_verbose == 1) {
    printf("P%5d | live %d\n", cpu->processes->pid, name);
    printf("Player %d (%s) is said to be alive\n", -name,
           h.prog_name); // TODO: get program name
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// ld takes 2 parameters, 2nd must be a register that isn't the
// 'program counter'. It loads the value of the first parameter in the register,
// and modifies the 'carry'. 'ld 34,r3' loads the REG_SIZE bytes from address
// (PC + (34 % IDX_MOD)) in register r3.
int instruction_ld(struct s_cpu *cpu) {

  int pc = cpu->processes->pc;
  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LD start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  int val;
  if (f_verbose >= 3)
    dump_nbytes(cpu, 10, pc, 1);
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  if (f_verbose >= 3)
    printf("DBG: pc(%d) INS_LD\n", cpu->processes->pc);
  val = read_val_load(cpu, get_param(par, 1));
  if (f_verbose >= 3)
    printf("DBG: pc(%d) val(%08x) INS_LD\n", cpu->processes->pc, val);
  uint8_t reg = read_mem_byte(cpu->program, cpu->processes->pc);
  if (f_verbose >= 3)
    printf("DBG: pc(%d) reg(%02hhx) INS_LD\n", cpu->processes->pc, reg);
  write_reg(cpu, reg, val);
  mod_carry(cpu,
            read_reg(cpu, read_mem_byte(cpu->program, cpu->processes->pc)));
  cpu->processes->pc += 1;
  if (f_verbose >= 3)
    printf("DBG: pc(%d) INS_LD end\n", cpu->processes->pc);
  if (f_verbose == 1) {
    printf("P%5d | ld %d r%d\n", cpu->processes->pid, val, reg);
    print_adv(cpu, cpu->processes->pc - pc, pc);
    // puts("ld executed");
  }
  next(cpu);
  return 1;
}

// monstrosity for preserving the signedness of values read from core memory
union fungus {
  uint64_t u64;
  int64_t i64;
  uint32_t u32;
  int32_t i32;
  uint16_t u16;
  int16_t i16;
  uint8_t u8;
  int8_t i8;
};

// st takes 2 parameters, storing (REG_SIZE bytes) of the value of
// the first argument (always a register) in the second. 'st r4,34' stores the
// value of 'r4' at the address (PC + (34 % IDX_MOD)) 'st r3,r8' copies the
// contents of 'r3' to 'r8'
int instruction_st(struct s_cpu *cpu) {
  int v1, v2, r1;
  union fungus amongus;
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ST start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  if (check_param(par, cpu->program[pc] - 1)) {
    cpu->processes->pc += 3;
    return 1;
    ;
  }
  cpu->processes->pc += 1;
  r1 = read_val(cpu, get_param(par, 1));
  v1 = read_reg(cpu, r1);
  if (f_verbose >= 3)
    printf("DBG: pc(%d) mem(%02x) v1(%08x) INS_ST\n", cpu->processes->pc,
           cpu->program[cpu->processes->pc], v1);
  if (get_param(par, 2) == REG_CODE) {
    if (f_verbose >= 3)
      dump_nbytes(cpu, 4, pc, 1);
    v2 = read_mem_byte(cpu->program, cpu->processes->pc);
    write_reg(cpu, v2, v1);
    cpu->processes->pc += 1;
  } else {
    if (f_verbose >= 3)
      dump_nbytes(cpu, 5, pc, 1);
    amongus.i16 = v2 = (int16_t)read_mem_word(cpu->program, cpu->processes->pc);
    if (f_verbose >= 3)
      printf("DBG: ind(%04hx) idx(%hd) amongus(%hd) INS_ST\n", (int16_t)v2,
             (short)(pc + ((v2) % IDX_MOD)), amongus.i16);
    write_mem_ins(cpu->program, pc + ((amongus.i16) % IDX_MOD), v1);
    cpu->processes->pc += 2;
  }
  if (f_verbose == 1) {
    printf("P%5d | st r%d ", cpu->processes->pid, r1);
    if (get_param(par, 2) == REG_CODE)
      printf("r%d\n", v2);
    else
      printf("%d\n", v2);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// add takes 3 registers as parameters, adding the contents of the
// first and second, storing the result into the third. Modifies carry. 'add
// r2,r3,r5' adds the values of 'r2' and 'r3' and stores the result in 'r5'.
int instruction_add(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ADD start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int r1 = read_val(cpu, get_param(par, 1));
  int r2 = read_val(cpu, get_param(par, 2));
  int r3 = read_val(cpu, get_param(par, 3));
  int v1 = read_reg(cpu, r1);
  int v2 = read_reg(cpu, r2);
  if (f_verbose >= 3)
    printf("DBG: v1(%08x) v2(%08x) INS_ADD\n", v1, v2);
  write_reg(cpu, r3, v1 + v2);
  mod_carry(cpu,
            read_reg(cpu, read_mem_byte(cpu->program, cpu->processes->pc)));
  if (f_verbose == 1) {
    printf("P%5d | add r%d r%d r%d\n", cpu->processes->pid, r1, r2, r3);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// sub is the same as instruction_add, except performs subtraction.
int instruction_sub(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_SUB start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int r1 = read_val(cpu, get_param(par, 1));
  int r2 = read_val(cpu, get_param(par, 2));
  int r3 = read_val(cpu, get_param(par, 3));
  int v1 = read_reg(cpu, r1);
  int v2 = read_reg(cpu, r2);
  if (f_verbose >= 3)
    printf("DBG: v1(%08x) v2(%08x) INS_SUB\n", v1, v2);
  write_reg(cpu, r3, v1 - v2);
  mod_carry(cpu,
            read_reg(cpu, read_mem_byte(cpu->program, cpu->processes->pc)));
  if (f_verbose == 1) {
    printf("P%5d | sub r%d r%d r%d\n", cpu->processes->pid, r1, r2, r3);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// perform a bitwise AND on the first two parameters, storing theresult into the
// third which is always a register. Modifies carry.'and r2,%0,r3' stores 'r2 &
// 0' into 'r3'.
int instruction_and(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_AND start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  int v1, v2, r1, r2;
  if (f_verbose >= 3)
    dump_nbytes(cpu, 9, cpu->processes->pc, 1);
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = v1 = read_val(cpu, p1);
    if (p1 == REG_CODE)
      v1 = read_reg(cpu, v1);
  } else {
    v1 = read_mem_word(cpu->program, cpu->processes->pc);
    v1 = read_mem_long(cpu->program, v1 + cpu->processes->pc - 2);
    cpu->processes->pc += 2;
  }
  if (p2 != IND_CODE) {
    r2 = v2 = read_val(cpu, p2);
    if (p2 == REG_CODE)
      v2 = read_reg(cpu, v2);
  } else {
    v2 = read_mem_word(cpu->program, cpu->processes->pc);
    v2 = read_mem_long(cpu->program,
                       v2 + cpu->processes->pc - 2 - (p1 == IND_CODE ? 2 : 0));
    cpu->processes->pc += 2;
  }
  int v3 = read_val(cpu, get_param(par, 3));
  if (f_verbose >= 3)
    printf("DBG: v1(%08x) v2(%08x) v3(%08x) INS_AND\n", v1, v2, v3);
  write_reg(cpu, v3, v1 & v2);
  mod_carry(cpu,
            read_reg(cpu, read_mem_byte(cpu->program, cpu->processes->pc)));
  if (f_verbose >= 3)
    dump_nbytes(cpu, 9, cpu->processes->pc, 1);
  if (f_verbose >= 3)
    printf("DBG: pc(%d) INS_AND end\n", cpu->processes->pc);
  if (f_verbose == 1) {
    printf("P%5d | and ", cpu->processes->pid);
    if (p1 == REG_CODE)
      printf("r%d ", r1);
    else
      printf("%d ", v1);
    if (p2 == REG_CODE)
      printf("r%d ", r2);
    else
      printf("%d ", v2);
    printf("r%d\n", v3);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// or is the same as instruction_and, except performs a bitwise OR.
int instruction_or(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_OR start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  int v1, v2, r1, r2;
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = v1 = read_val(cpu, p1);
    if (p1 == REG_CODE)
      v1 = read_reg(cpu, v1);
  } else {
    v1 = read_mem_word(cpu->program, cpu->processes->pc);
    v1 = read_mem_long(cpu->program, v1 + cpu->processes->pc - 2);
    cpu->processes->pc += 2;
  }
  if (p2 != IND_CODE) {
    r2 = v2 = read_val(cpu, p2);
    if (p2 == REG_CODE)
      v2 = read_reg(cpu, v2);
  } else {
    v2 = read_mem_word(cpu->program, cpu->processes->pc);
    v2 = read_mem_long(cpu->program,
                       v2 + cpu->processes->pc - 2 - (p1 == IND_CODE ? 2 : 0));
    cpu->processes->pc += 2;
  }
  if (f_verbose >= 3)
    printf("DBG: v1(%08x) v2(%08x) INS_OR\n", v1, v2);
  write_reg(cpu, read_val(cpu, get_param(par, 3)), v1 | v2);
  mod_carry(cpu,
            read_reg(cpu, read_mem_byte(cpu->program, cpu->processes->pc)));
  if (f_verbose == 1) {
    printf("P%5d | and ", cpu->processes->pid);
    if (p1 == REG_CODE)
      printf("r%d ", r1);
    else
      printf("%d", v1);
    if (p2 == REG_CODE)
      printf("r%d", r2);
    else
      printf("%d", v2);
    printf("\n");
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// xor is the same as instruction_and, except performs a bitwise
// XOR.
int instruction_xor(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_XOR start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  int v1, v2, r1, r2;
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = read_mem_byte(cpu->program, cpu->processes->pc);
    v1 = read_val_load(cpu, p1);
  } else {
    v1 = read_mem_word(cpu->program, cpu->processes->pc);
    v1 = read_mem_long(cpu->program, v1 + cpu->processes->pc - 2);
    cpu->processes->pc += 2;
  }
  if (p2 != IND_CODE) {
    r2 = read_mem_byte(cpu->program, cpu->processes->pc);
    v2 = read_val_load(cpu, p2);
  } else {
    v2 = read_mem_word(cpu->program, cpu->processes->pc);
    v2 = read_mem_long(cpu->program,
                       v2 + cpu->processes->pc - 2 - (p1 == IND_CODE ? 2 : 0));
    cpu->processes->pc += 2;
  }
  if (f_verbose >= 3)
    printf("DBG: v1(%08x) v2(%08x) INS_XOR\n", v1, v2);
  write_reg(cpu, read_val(cpu, get_param(par, 3)), v1 ^ v2);
  mod_carry(cpu,
            read_reg(cpu, read_mem_byte(cpu->program, cpu->processes->pc)));
  if (f_verbose == 1) {
    printf("P%5d | and ", cpu->processes->pid);
    if (p1 == REG_CODE)
      printf("r%d ", r1);
    else
      printf("%d", v1);
    if (p2 == REG_CODE)
      printf("r%d", r2);
    else
      printf("%d", v2);
    printf("\n");
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// zjmp always takes an index (IND_SIZE) and makes a jump at this
// index if carry is true, otherwise consuming cycles. 'zjmp %23' stores (PC +
// (23 % IDX_MOD)) into PC.
int instruction_zjmp(struct s_cpu *cpu) {
  ; // TODO: better instruction_zjmp regression tests
  // int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_ZJMP start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->processes->pc += 1;
  short v1 = (int16_t)read_mem_word(cpu->program, cpu->processes->pc);
  if (f_verbose >= 3)
    printf("DBG: carry(%d) v1(%04hx)(%d) INS_ZJMP\n", cpu->processes->carry, v1,
           v1 % IDX_MOD);
  if (cpu->processes->carry == true) {
    cpu->processes->pc =
        ((cpu->processes->pc + ((v1 - 1) % IDX_MOD)) % MEM_SIZE);
    if (f_verbose >= 3)
      printf("DBG: jumping to pc(%d) INS_ZJMP\n", cpu->processes->pc);
  } else {
    cpu->processes->pc += 2;
  }
  if (f_verbose == 1) {
    printf("P%5d | zjmp %d %s\n", cpu->processes->pid, v1,
           (cpu->processes->carry ? "OK" : "FAILED"));
    // print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// ldi modifies carry. 'idx' and 'add' are indexes, and 'reg' is a
// register. 'ldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3 %
// IDX_MOD)), adding 4 to this sum S. Read REG_SIZE bytes at address (PC + (S %
// IDX_MOD)), which are copied to 'r1'.
int instruction_ldi(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LDI start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  int v1, v2, v3, idx, r1, r2;
  int op = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int p1 = get_param(par, 1);
  int p2 = get_param(par, 2);
  if (p1 != IND_CODE) {
    r1 = read_mem_byte(cpu->program, cpu->processes->pc);
    v1 = read_val_idx(cpu, p1, op);
  } else {
    v1 = read_mem_word(cpu->program, cpu->processes->pc);
    cpu->processes->pc += 2;
    v1 = read_mem_long(cpu->program, v1 + pc);
  }
  if (f_verbose >= 3)
    printf("DBG: v1(0x%08x)(%d) INS_LDI\n", v1, v1);
  r2 = read_mem_byte(cpu->program, cpu->processes->pc);
  v2 = read_val_idx(cpu, p2, op);
  idx = (pc + (v1 % IDX_MOD)) + v2;
  dump_nbytes(cpu, 4, idx, 1);
  if (f_verbose >= 3)
    printf("DBG: load from %d + %d = %d [with pc and mod=idx(%d)] INS_LDI\n",
           v1, v2, v1 + v2, idx);
  v3 = read_mem_long(cpu->program, idx);
  if (f_verbose >= 3)
    printf("DBG: v3(%08x) INS_LDI\n", v3);
  char reg = read_mem_byte(cpu->program, cpu->processes->pc);
  if (f_verbose >= 3)
    printf("DBG: reg(%d) v1(%08x) v2(%08x) v3(%08x) INS_LDI write_reg\n", reg,
           v1, v2, v3);
  write_reg(cpu, reg, v3);
  cpu->processes->pc += 1;
  if (f_verbose == 1) {
    printf("P%5d | ldi", cpu->processes->pid);
    if (p1 == REG_CODE)
      printf(" r%d", r1);
    else
      printf(" %d", v1);
    if (p2 == REG_CODE)
      printf(" r%d", r2);
    else
      printf(" %d", v2);
    printf(" r%d\n", reg);
    printf("      | -> load from %d + %d = %d (with pc and mod %d)\n", v1, v2,
           v1 + v2, idx);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// sti does something probably. 'sti r2,%4,%5' copies REG_SIZE bytes
// of 'r2' at address (4 + 5) Parameters 2 and 3 are treated as indexes.
int instruction_sti(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_STI start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  int v1, v2, v3, idx, p2, p3, r1, r2 = 0, r3 = 0;
  int op = cpu->program[pc];
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  r1 = read_mem_byte(cpu->program, cpu->processes->pc);
  v1 = read_val_idx(cpu, get_param(par, 1), op);
  if (f_verbose >= 3)
    printf("DBG: v1(%08x)(%d) INS_STI\n", v1, v1);
  v2 = read_val_idx(cpu, (p2 = get_param(par, 2)), op);
  if (f_verbose >= 3)
    printf("DBG: v2(%08x)(%d) INS_STI\n", v2, v2);
  v3 = read_val_idx(cpu, (p3 = get_param(par, 3)), op);
  if (f_verbose >= 3)
    printf("DBG: v3(%08x)(%d) INS_STI\n", v3, v3);
  idx = ((pc + (v2 + v3) % IDX_MOD));
  if (f_verbose >= 3)
    printf("DBG: store to %d + %d = %d [with pc and mod=idx(%d)] INS_STI\n", v2,
           v3, v2 + v3, idx);
  write_mem_ins(cpu->program, idx, v1);
  if (f_verbose == 1) {
    printf("P%5d | stii r%d", cpu->processes->pid, r1);
    if (p2 == REG_CODE)
      printf(" r%d", r2);
    else
      printf(" %d", v2);
    if (p3 == REG_CODE)
      printf(" r%d\n", r3);
    else
      printf(" %d\n", v3);
    printf("      | -> store to %d + %d = %d (with pc and mod %d)\n", v2, v3,
           v2 + v3, idx);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// fork always takes an index and creates a new program which is
// executed from address (PC + ('idx' % IDX_MOD)). 'fork %34' spawns a new
// process at (PC + (34 % IDX_MOD)). helltrain cycles (1105,1935,2745,3555)
// TOOD: handle process calling fork correctly: cycle 2745,
//       pid(3).instruction_time is decremented twice
int instruction_fork(struct s_cpu *cpu) {
  struct s_process *prev_head = cpu->processes;
  int pc = cpu->processes->pc;
  dump_nbytes(cpu, 6, pc, cpu->program[pc]);
  union fungus amongus;
  short new;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_FORK start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  amongus.i16 = new =
      read_mem_word(cpu->program, cpu->processes->pc + 1); // % IDX_MOD;
  if (f_verbose >= 3)
    printf("DBG: new(0x%02hx) amongus(%hd) pc(%d) INS_FORK read\n", new,
           amongus.i16, pc + new);
  new %= IDX_MOD;
  if (f_verbose >= 3)
    printf("DBG: new(0x%02hx) amongus(%hd) pc(%d) INS_FORK mod\n", new,
           amongus.i16, pc + new);
  if (new < 0)
    new += MEM_SIZE;
  if (f_verbose >= 3)
    printf("DBG: new(0x%02hx) amongus(%hd) pc(%d) INS_FORK plusmem\n", new,
           amongus.i16, pc + new);
  cpu->processes->pc += 3;
  cpu->spawn_process(cpu, new + cpu->processes->pc - 3,
                     cpu->processes->registers[0]);
  if (f_verbose == 1) {
    printf("P%5d | fork %d (%d)\n", cpu->processes->pid, new, new);
    // print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  if (f_verbose >= 3)
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
  if (f_verbose >= 3)
    printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
           "pc(%4d) ins_time(%4d) INS_FORK prev_head end\n",
           cpu->clock, prev_head->pid, prev_head->carry, prev_head->last_live,
           prev_head->pc, prev_head->instruction_time);
  // struct s_process *proc = cpu->processes;
  // while (proc != 0) {
  //   int prev_time = proc->instruction_time;
  //   if (prev_time == 0 || (prev_time == 0 && proc->last_live == 0)) {
  //     uint8_t op = read_mem_byte(cpu->program, proc->pc);
  //     if (op >= 1 && op <= 16)
  //       proc->instruction_time = g_op_tab[op - 1].cycles_to_exec;
  //     else
  //       proc->instruction_time = 1;
  //     // next(cpu);
  //   }
  //   if (f_verbose >= 3)
  //     printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) pc(%4d) "
  //            "ins_time(%4d)(%4d) prv_time(%d) NEXT loop end\n",
  //            cpu->clock, proc->pid, proc->carry,
  //            proc->last_live, proc->pc, prev_time,
  //            proc->instruction_time, proc->prev_time);
  //   proc = proc->next;
  // }
  if (cpu->clock == 1934)
    ; // pause();
  // next(cpu);
  // if (f_verbose >= 3)
  //   printf("DBG: clock(%zu) pid(%d) carry(%d) last_live(%d) pc(%d) "
  //          "ins_time(%d) INS_FORK reset proc\n",
  //          cpu->clock, cpu->processes->pid, cpu->processes->carry,
  //          cpu->processes->last_live, cpu->processes->pc,
  //          cpu->processes->instruction_time);
  // cpu->processes = cpu->first->next;
  // cpu->processes->instruction_time += 1;
  if (f_verbose >= 3)
    printf("DBG: clock(%zu) pid(%d) carry(%d) last_live(%d) pc(%d) "
           "ins_time(%d) INS_FORK end\n",
           cpu->clock, cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc,
           cpu->processes->instruction_time);
  // next(cpu);
  return 1;
}

// lld is the same as 'ld', but without the (% IDX_MOD). Modifies
// carry. 'lld 34,r3' loads the REG_SIZE bytes from address (PC + (34)) in
// register r3.
int instruction_lld(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LLD start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int r1;
  int v1 = read_val_load(cpu, get_param(par, 1));
  if (f_verbose >= 3)
    printf("DBG: v1(0x%08x)(%d) INS_LDI read_val_load\n", v1, v1);
  r1 = read_mem_byte(cpu->program, cpu->processes->pc);
  write_reg(cpu, r1, ntohl(v1));
  mod_carry(cpu, read_reg(cpu, r1));
  cpu->processes->pc += 1;
  if (f_verbose == 1) {
    printf("P%5d | lld %d r%d\n", cpu->processes->pid, v1, r1);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// lldi is the same as 'ldi', but without the (% IDX_MOD). Modifies
// carry. 'lldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3)), adding 4
// to this sum S. Read REG_SIZE bytes at address (PC + (S)), which are copied to
// 'r1'
int instruction_lldi(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LLDI start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  int p1, p2, r1, r2, r3;
  int op = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  p1 = get_param(par, 1);
  p2 = get_param(par, 2);
  r1 = read_mem_byte(cpu->program, cpu->processes->pc);
  int v1 = read_val_idx(cpu, p1, op);
  r2 = read_mem_byte(cpu->program, cpu->processes->pc);
  int v2 = read_val_idx(cpu, p2, op);
  int idx = v1 + v2 + pc;
  int v3 = read_mem_long(cpu->program, idx); // TODO: AAAAAAA?!?
  r3 = read_mem_byte(cpu->program, cpu->processes->pc);
  write_reg(cpu, r3, v3);
  mod_carry(cpu, read_reg(cpu, r3));
  cpu->processes->pc += 1;
  if (f_verbose == 1) {
    printf("P%5d | lldi", cpu->processes->pid);
    if (p1 == REG_CODE)
      printf(" r%d", r1);
    else
      printf(" %d", v1);
    if (p2 == REG_CODE)
      printf(" r%d", r2);
    else
      printf(" %d", v2);
    printf(" r%d\n", r3);
    printf("      | -> load from %d + %d = %d (with pc and mod %d)\n", v1, v2,
           v1 + v2, idx);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// lfork is the same as 'fork', but without the (% IDX_MOD).
// Modifies carry.
int instruction_lfork(struct s_cpu *cpu) {
  int pc = cpu->processes->pc;

  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_LFORK start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->processes->pc += 1;
  short idx = read_mem_word(cpu->program, cpu->processes->pc);
  cpu->processes->pc += 2;
  cpu->spawn_process(cpu, (pc + idx) % MEM_SIZE, *cpu->processes->registers);
  if (f_verbose == 1) {
    printf("P%5d | lfork %d (%d)\n", cpu->processes->pid, idx,
           idx + cpu->processes->pc - 2);
    // print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}

// aff takes a register and writes the stored value modulo 256 to
// stdout. 'ld %52,r3  aff r3' displays '*' on stdout.
int instruction_aff(struct s_cpu *cpu) {

  int pc = cpu->processes->pc;
  if (f_verbose >= 3)
    printf("DBG: pid(%d) carry(%d) last_live(%d) pc(%d) INS_AFF start\n",
           cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc);
  cpu->processes->pc += 1;
  // uint8_t par = cpu->program[cpu->processes->pc];
  cpu->processes->pc += 1;
  int val = read_reg(cpu, read_mem_byte(cpu->program, cpu->processes->pc));
  // ft_putchar(val & 0xff);
  // ft_putchar('\n');
  cpu->processes->pc += 1;
  if (f_verbose == 1) {
    printf("Aff: %c\n", val & 0xff);
    print_adv(cpu, cpu->processes->pc - pc, pc);
  }
  next(cpu);
  return 1;
}
