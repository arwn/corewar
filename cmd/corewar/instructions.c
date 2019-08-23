#include "cpu.h"
#include "op.h"
#include "util.h"
#include <stdio.h>

// clang-format off
// must be included AFTER cpu.h
#include "instructions.h"
// clang-format on

// Get the type of the N'th argument from the pcb
static int type_from_pcb(uint8_t pcb, int arg) {
  switch ((pcb >> (('\x03' - (char)arg) * '\x02' & 0x1fU)) & 3) {
  case DIR_CODE:
    return T_DIR;
  case IND_CODE:
    return T_IND;
  case REG_CODE:
    return T_REG;
  }
  return 0;
}

// returns REG_SIZE, IND_SIZE, DIR_SIZE, or SPECIAL_DIR_SIZE
static int size_from_pt(int type, int opcode) {
  switch (type) {
  case T_REG:
    return REG_SIZE;
  case T_IND:
    return IND_SIZE;
  case T_DIR:
    if (g_op_tab[opcode].direct_size == 0)
      return DIR_SIZE;
    else
      return SPECIAL_DIR_SIZE;
  }
  return 0;
}

static int size_from_pcb(uint8_t pcb, int opcode) {
  int ret, tmp;
  int ii;

  ii = 0;
  ret = 0;
  while (ii < g_op_tab[opcode].numargs) {
    tmp = type_from_pcb(pcb, ii);
    tmp = size_from_pt(tmp, opcode);
    ret += tmp;
    ii += 1;
  }
  return ret;
}

static int check_pcb(uint8_t pcb, int op) {
  int ii;
  int type;

  ii = 0;
  while (ii < g_op_tab[op].numargs) {
    type = type_from_pcb(pcb, ii);
    if (((type & g_op_tab[op].argtypes[ii]) != type) || type == 0)
      return 0;
    ii += 1;
  }
  return 1;
}

// prints the movement of the program counter for PROC
static void print_adv(struct s_cpu *cpu, struct s_process *proc, int new) {
  int ii;
  int len;

  ii = 0;
  len = new - proc->pc;
  printf("ADV %d (0x%04x -> 0x%04x) ", len, proc->pc, new);
  while (ii < len) {
    printf((!ii ? "%02x" : " %02x"), read_mem_1(cpu->program, proc->pc + ii));
    ii += 1;
  }
  printf("\n");
}

// next_instruction updates the execution time for PROC.
void next_cpu_op(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t op;
  int newpc;

  op = (char)read_mem_1(cpu->program, proc->pc);
  if (op < 1 || NUM_OPS < op) {
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

/* Utility functions */
#define MEM_COLOR(IDX, PL)                                                     \
  do {                                                                         \
    if (g_mem_colors[(IDX)].writes == 0 || g_mem_colors[(IDX)].player != (PL)) \
      g_mem_colors[(IDX)].writes = 49;                                         \
    if (g_mem_colors[(IDX)].player == 0 || g_mem_colors[(IDX)].player != (PL)) \
      g_mem_colors[(IDX)].player = (PL);                                       \
  } while (0)

// Write four bytes of VAL into core memory MEM at offset IDX
static void write_mem_ins(struct s_process *proc, uint8_t *mem, uint32_t idx,
                          uint32_t val) {
  const int idx1 = idx % MEM_SIZE;
  const int idx2 = (idx + 1) % MEM_SIZE;
  const int idx3 = (idx + 2) % MEM_SIZE;
  const int idx4 = (idx + 3) % MEM_SIZE;

  if (f_color || f_gui) {
    MEM_COLOR(idx1, proc->player);
    MEM_COLOR(idx2, proc->player);
    MEM_COLOR(idx3, proc->player);
    MEM_COLOR(idx4, proc->player);
  }

  mem[idx1] = (val >> 24) & 0xff;
  mem[idx2] = (val >> 16) & 0xff;
  mem[idx3] = (val >> 8) & 0xff;
  mem[idx4] = val & 0xff;
}

// Write VAL into register REG for the process PROC
static void write_reg(struct s_process *proc, uint32_t reg, uint32_t val) {
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: write_reg reg(%08x) val(%08x)\n", reg, val);
  if (reg > 0 && reg <= REG_NUMBER)
    proc->registers[reg - 1] = val;
}

// Read register REG for the process PROC
static int read_reg(struct s_process *proc, uint32_t reg) {
  if (reg > 0 && reg <= REG_NUMBER) {
    int ret = proc->registers[reg - 1];
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_reg reg(%08x)\n", ret);
    return (ret);
  }
  return 0;
}

// Modify the carry flag for the current process based on VAL
static void mod_carry(struct s_process *proc, int val) {
  if (val) {
    proc->carry = 1;
  } else {
    proc->carry = 0;
  }
}

/* Instructions */

// Stores the current cycle into the lastlive array in CPU, if the NAME
// corresponds to an active player
int instruction_live(struct s_cpu *cpu, struct s_process *proc) {
  int player_id;
  int player_idx;

  player_id = (int)read_mem_4(cpu->program, proc->pc + 1);
  if (f_verbose & OPT_LIVES)
    printf("P% 5d | live %d\n", proc->pid, player_id);
  proc->last_live = cpu->clock;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: proc->last_live(%d) INS_LIVE\n", proc->last_live);
  cpu->nbr_lives += 1;
  player_idx = ~player_id;
  if (player_idx >= 0 && player_idx < MAX_PLAYERS) {
    if ((f_verbose & OPT_LIVES))
      printf("Player %d (%s) is said to be alive\n", player_idx + 1,
             cpu->players[player_idx].name);
    cpu->players[player_idx].last_live = cpu->clock;
    cpu->winner = player_idx;
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 5);
  return proc->pc + 5;
}

// reads an indirect value from core memory
int read_indirect(struct s_cpu *cpu, struct s_process *proc, short offset) {
  int idx;
  int ofs;
  int ret;

  ofs = offset % IDX_MOD;
  idx = proc->pc + ofs;
  ret = (int)read_mem_4(cpu->program, idx);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: offset(%04hx) ofs(%08x) idx(%08x) ret(%08x) READ_IND end\n",
           offset, ofs, idx, ret);
  return ret;
}

// check if a register number is valid: r1 - r16
#define VALID_REGISTER(X) ((X) > 0 && (X) <= REG_NUMBER)

// 'ld' takes 2 parameters, 2nd must be a register that isn't the
// 'program counter'. It loads the value of the first parameter in the register,
// and modifies the 'carry'. 'ld 34,r3' loads the REG_SIZE bytes from address
// (PC + (34 % IDX_MOD)) in register r3.
int instruction_ld(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int val;
  int reg;
  int type;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  if (check_pcb(pcb, e_ld) != 0) {
    type = type_from_pcb(pcb, 0);
    if (type == T_DIR)
      val = (int)read_mem_4(cpu->program, proc->pc + 2);
    else
      val = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
    reg = read_mem_1(cpu->program, proc->pc + 2 + size_from_pt(type, e_ld));
    if (VALID_REGISTER(reg) != 0) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | ld %d r%d\n", proc->pid, val, reg);
      mod_carry(proc, (val == 0));
      write_reg(proc, reg, val);
    }
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + size_from_pcb(pcb, e_ld) + 2);

  return proc->pc + size_from_pcb(pcb, e_ld) + 2;
}

// 'st' takes 2 parameters, storing (REG_SIZE bytes) of the value of
// the first argument (always a register) in the second. 'st r4,34' stores the
// value of 'r4' at the address (PC + (34 % IDX_MOD)) 'st r3,r8' copies the
// contents of 'r3' to 'r8'
int instruction_st(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int type;
  int val;
  int reg;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  if (check_pcb(pcb, e_st) != 0) {
    reg = read_mem_1(cpu->program, proc->pc + 2);
    type = type_from_pcb(pcb, 1);
    if (type == T_IND)
      val = (short)read_mem_2(cpu->program, proc->pc + 3);
    else
      val = read_mem_1(cpu->program, proc->pc + 3);
    if (VALID_REGISTER(reg) != 0) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | st r%d %d\n", proc->pid, reg, val);
      if ((type == T_REG) && VALID_REGISTER(val) != 0)
        write_reg(proc, val, read_reg(proc, reg));
      else if (type == T_IND)
        write_mem_ins(proc, cpu->program, (proc->pc + val % IDX_MOD),
                      read_reg(proc, reg));
    }
  }
  reg = size_from_pcb(pcb, e_st);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + reg + 2);
  return proc->pc + reg + 2;
}

// 'add' takes 3 registers as parameters, adding the contents of the
// first and second, storing the result into the third. Modifies carry. 'add
// r2,r3,r5' adds the values of 'r2' and 'r3' and stores the result in 'r5'.
int instruction_add(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int done;
  int reg1;
  int reg2;
  int reg3;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  if (check_pcb(pcb, e_add) != 0) {
    reg1 = read_mem_1(cpu->program, proc->pc + 2);
    reg2 = read_mem_1(cpu->program, proc->pc + 3);
    reg3 = read_mem_1(cpu->program, proc->pc + 4);
    if (VALID_REGISTER(reg1) && VALID_REGISTER(reg2) && VALID_REGISTER(reg3)) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | add r%d r%d r%d\n", proc->pid, reg1, reg2, reg3);
      done = read_reg(proc, reg1) + read_reg(proc, reg2);
      mod_carry(proc, (done == 0));
      write_reg(proc, reg3, done);
    }
  }
  done = size_from_pcb(pcb, e_add);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + done + 2);
  return proc->pc + done + 2;
}

// 'sub' is the same as instruction_add, except performs subtraction.
int instruction_sub(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int done;
  int reg1;
  int reg2;
  int reg3;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  if (check_pcb(pcb, e_sub) != 0) {
    reg1 = read_mem_1(cpu->program, proc->pc + 2);
    reg2 = read_mem_1(cpu->program, proc->pc + 3);
    reg3 = read_mem_1(cpu->program, proc->pc + 4);
    if (VALID_REGISTER(reg1) && VALID_REGISTER(reg2) && VALID_REGISTER(reg3)) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | sub r%d r%d r%d\n", proc->pid, reg1, reg2, reg3);
      done = read_reg(proc, reg1) - read_reg(proc, reg2);
      mod_carry(proc, (done == 0));
      write_reg(proc, reg3, done);
    }
  }
  done = size_from_pcb(pcb, e_sub);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + done + 2);
  return proc->pc + done + 2;
}

// readaroni that typearoni
int read_typearoni(struct s_cpu *cpu, struct s_process *proc, int type,
                   int offset) {
  int ret;

  ret = 0;
  if (type == T_REG) {
    ret = read_mem_1(cpu->program, offset);
    if (VALID_REGISTER(ret) != 0)
      ret = read_reg(proc, ret);
  } else if (type == T_DIR) {
    ret = read_mem_4(cpu->program, offset);
  } else if (type == T_IND) {
    ret = read_indirect(cpu, proc, read_mem_2(cpu->program, offset));
  }
  return ret;
}

// INSTRUCTION_BITWISE generates an instruction function withe name and operator
#define INSTRUCTION_BITWISE(name, operator)                                    \
  int instruction_##name(struct s_cpu *cpu, struct s_process *proc) {          \
    uint8_t pcb;                                                               \
    int offset;                                                                \
    int type;                                                                  \
    int arg1;                                                                  \
    int arg2;                                                                  \
    uint8_t arg3;                                                              \
    pcb = read_mem_1(cpu->program, proc->pc + 1);                              \
    if (check_pcb(pcb, e_##name) != 0) {                                       \
      type = type_from_pcb(pcb, 0);                                            \
      offset = proc->pc + 2;                                                   \
      arg1 = read_typearoni(cpu, proc, type, offset);                          \
      offset += size_from_pt(type, e_##name);                                  \
      type = type_from_pcb(pcb, 1);                                            \
      arg2 = read_typearoni(cpu, proc, type, offset);                          \
      offset += size_from_pt(type, e_##name);                                  \
      arg3 = read_mem_1(cpu->program, offset);                                 \
      if (VALID_REGISTER(arg3) != 0) {                                         \
        if (f_verbose & OPT_INSTR)                                             \
          printf("P% 5d | %s %d %d r%d\n", proc->pid, #name, arg1, arg2,       \
                 arg3);                                                        \
        mod_carry(proc, (arg1 operator arg2) == 0);                            \
        write_reg(proc, arg3, (arg1 operator arg2));                           \
      }                                                                        \
    }                                                                          \
    if (f_verbose & OPT_PCMOVE)                                                \
      print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_##name));       \
    return proc->pc + 2 + size_from_pcb(pcb, e_##name);                        \
  }

// perform a bitwise and on the first two parameters, storing the result into
// the third which is always a register. Modifies carry.'and r2,%0,r3' stores
// 'r2 & 0' into 'r3'.
INSTRUCTION_BITWISE(and, &);

// 'or' is the same as and, except uses bitwise or
INSTRUCTION_BITWISE(or, |);

// 'xor' is the same as and, except uses bitwise xor
INSTRUCTION_BITWISE(xor, ^);

// 'zjmp' always takes an index (IND_SIZE) and makes a jump at this
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
  return local_c;
}

// ldi modifies carry. 'idx' and 'add' are indexes, and 'reg' is a
// register. 'ldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3 %
// IDX_MOD)), adding 4 to this sum S. Read REG_SIZE bytes at address (PC + (S %
// IDX_MOD)), which are copied to 'r1'.
int instruction_ldi(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t arg3;
  int offset;
  int type;
  int arg1;
  int arg2;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  while (check_pcb(pcb, e_ldi) != 0) {
    offset = proc->pc + 2;
    type = type_from_pcb(pcb, 0);
    if (type == T_REG) {
      arg1 = read_mem_1(cpu->program, offset);
      if (VALID_REGISTER(arg1) == 0)
        break;
      arg1 = read_reg(proc, arg1);
    } else if (type == T_IND)
      arg1 = read_indirect(cpu, proc, read_mem_2(cpu->program, offset));
    else
      arg1 = (short)read_mem_2(cpu->program, offset);
    offset += size_from_pt(type, e_ldi);
    type = type_from_pcb(pcb, 1);
    if (type == T_REG) {
      arg2 = read_mem_1(cpu->program, offset);
      if (VALID_REGISTER(arg2) == 0)
        break;
      arg2 = read_reg(proc, arg2);
    } else
      arg2 = (short)read_mem_2(cpu->program, offset);
    offset += size_from_pt(type, e_ldi);
    arg3 = read_mem_1(cpu->program, offset);
    if (VALID_REGISTER(arg3) == 0)
      break;
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | ldi %d %d r%d\n", proc->pid, arg1, arg2, arg3);
      printf("       | -> load from %d + %d = %d (with pc and mod %d)\n", arg1,
             arg2, arg1 + arg2, (proc->pc + (arg1 + arg2) % IDX_MOD));
    }
    write_reg(proc, arg3,
              read_mem_4(cpu->program, proc->pc + (arg1 + arg2) % IDX_MOD));
    break;
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_ldi));
  return (proc->pc + 2 + size_from_pcb(pcb, e_ldi));
}

// 'sti' stores at an index offset. 'sti r2,%4,%5' copies REG_SIZE bytes
// of 'r2' at address (4 + 5) Parameters 2 and 3 are treated as indexes.
int instruction_sti(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int type;
  int reg;
  int arg2;
  int arg3;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  while (check_pcb(pcb, e_sti) != 0) {
    reg = read_mem_1(cpu->program, proc->pc + 2);
    if (VALID_REGISTER(reg) == 0)
      break;
    type = type_from_pcb(pcb, 1);
    if (type == T_REG) {
      arg2 = read_mem_1(cpu->program, proc->pc + 3);
      if (VALID_REGISTER(arg2) == 0)
        break;
      arg2 = read_reg(proc, arg2);
    } else if (type == T_IND)
      arg2 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 3));
    else
      arg2 = (short)read_mem_2(cpu->program, proc->pc + 3);
    if (type_from_pcb(pcb, 2) == T_REG) {
      arg3 = read_mem_1(cpu->program, proc->pc + 3 + size_from_pt(type, e_sti));
      if (VALID_REGISTER(arg3) == 0)
        break;
      arg3 = read_reg(proc, arg3);
    } else
      arg3 = (short)read_mem_2(cpu->program,
                               proc->pc + 3 + size_from_pt(type, e_sti));
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | sti r%d %d %d\n", proc->pid, reg, arg2, arg3);
      printf("       | -> store to %d + %d = %d (with pc and mod %d)\n", arg2,
             arg3, (arg2 + arg3), proc->pc + (arg2 + arg3) % IDX_MOD);
    }
    write_mem_ins(proc, cpu->program, proc->pc + (arg2 + arg3) % IDX_MOD,
                  read_reg(proc, reg));
    break;
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_sti));
  return (proc->pc + 2 + size_from_pcb(pcb, e_sti));
}

// Allocate and initialize the new process with the state of the parent process
static void fork_process(struct s_cpu *cpu, struct s_process *proc, int idx) {
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
  new->pc = idxmod;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: idx(%d) idxmod(%d) FORK_PROCESS\n", idx, idxmod);
  pl = ~proc->player;
  if (pl >= 0 && pl < MAX_PLAYERS)
    cpu->players[pl].active_processes += 1;
  new->carry = proc->carry;
  new->player = proc->player;
  new->opcode = 0;
  new->instruction_time = 0;
  new->last_live = proc->last_live;
  new->pid = cpu->pid_next + 1;
  ii = 0;
  while (ii < REG_NUMBER) {
    new->registers[ii] = proc->registers[ii];
    ++ii;
  }
  new->next = cpu->processes;
  new->next->prev = new;
  new->prev = NULL;
  cpu->processes = new;
  cpu->active += 1;
  cpu->pid_next += 1;
}

// 'fork' always takes an index and creates a new program which is
// executed from address (PC + ('idx' % IDX_MOD)). 'fork %34' spawns a new
// process at (PC + (34 % IDX_MOD)).
int instruction_fork(struct s_cpu *cpu, struct s_process *proc) {
  int idx;
  short arg1;

  arg1 = read_mem_2(cpu->program, proc->pc + 1);
  idx = arg1 % IDX_MOD;
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | fork %d (%d)\n", proc->pid, arg1, proc->pc + idx);
  fork_process(cpu, proc, proc->pc + idx);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 3);
  return (proc->pc + 3);
}

// 'lld' is the same as 'ld', but without the (% IDX_MOD). Modifies
// carry. 'lld 34,r3' loads the REG_SIZE bytes from address (PC + (34)) in
// register r3.
int instruction_lld(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t arg2;
  int type;
  int arg1;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  if (check_pcb(pcb, e_lld) != 0) {
    type = type_from_pcb(pcb, 0);
    if (type == T_DIR)
      arg1 = (int)read_mem_4(cpu->program, proc->pc + 2);
    else
      arg1 = (short)read_mem_2(
          cpu->program, proc->pc + read_mem_2(cpu->program, proc->pc + 2));
    arg2 = read_mem_1(cpu->program, proc->pc + 2 + size_from_pt(type, e_lld));
    if (VALID_REGISTER(arg2) != 0) {
      if (f_verbose & OPT_INSTR)
        printf("P% 5d | lld %d r%d\n", proc->pid, arg1, arg2);
      mod_carry(proc, (arg1 == 0));
      write_reg(proc, arg2, arg1);
    }
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_lld));
  return (proc->pc + 2 + size_from_pcb(pcb, e_lld));
}

// 'lldi' is the same as 'ldi', but without the (% IDX_MOD). Modifies
// carry. 'lldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3)), adding 4
// to this sum S. Read REG_SIZE bytes at address (PC + (S)), which are copied to
// 'r1'.
int instruction_lldi(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t reg;
  int offset;
  int type;
  int arg1;
  int arg2;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  while (check_pcb(pcb, e_lldi) != 0) {
    offset = proc->pc + 2;
    type = type_from_pcb(pcb, 0);
    if (type == T_REG) {
      reg = read_mem_1(cpu->program, offset);
      if (VALID_REGISTER(reg) == 0)
        break;
      arg1 = read_reg(proc, reg);
    } else if (type == T_IND) {
      arg1 = read_indirect(cpu, proc, read_mem_2(cpu->program, offset));
    } else {
      arg1 = (short)read_mem_2(cpu->program, offset);
    }
    offset += size_from_pt(type, e_lldi);
    type = type_from_pcb(pcb, 1);
    if (type == T_REG) {
      reg = read_mem_1(cpu->program, offset);
      if (VALID_REGISTER(reg) == 0)
        break;
      arg2 = read_reg(proc, reg);
    } else {
      arg2 = (short)read_mem_2(cpu->program, offset);
    }
    offset += size_from_pt(type, e_lldi);
    reg = read_mem_1(cpu->program, offset);
    if (VALID_REGISTER(reg) == 0)
      break;
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | lldi %d %d r%d\n", proc->pid, arg1, arg2, reg);
      printf("       | -> load from %d + %d = %d (with pc %d)\n", arg1, arg2,
             arg1 + arg2, proc->pc + arg1 + arg2);
    }
    mod_carry(proc, (read_mem_4(cpu->program, proc->pc + arg1 + arg2) == 0));
    write_reg(proc, reg, read_mem_4(cpu->program, proc->pc + arg1 + arg2));
    break;
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_lldi));
  return (proc->pc + 2 + size_from_pcb(pcb, e_lldi));
}

// 'lfork' is the same as 'fork', but without the (% IDX_MOD).
// Modifies carry.
int instruction_lfork(struct s_cpu *cpu, struct s_process *proc) {
  short new_offset;

  new_offset = (short)read_mem_2(cpu->program, proc->pc + 1);
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | lfork %d (%d)\n", proc->pid, new_offset,
           new_offset + proc->pc);
  fork_process(cpu, proc, proc->pc + new_offset);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 3);
  return (proc->pc + 3);
}

// 'aff' takes a register and writes the stored value modulo 256 to
// stdout. 'ld %52,r3  aff r3' displays '*' on stdout.
int instruction_aff(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t arg1;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  if (check_pcb(pcb, e_aff) != 0) {
    arg1 = read_mem_1(cpu->program, proc->pc + 2);
    if (VALID_REGISTER(arg1) != 0) {
      arg1 = read_reg(proc, arg1);
      if (f_enable_aff != 0)
        printf("Aff: %c\n", arg1);
    }
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_aff));
  return (proc->pc + 2 + size_from_pcb(pcb, e_aff));
}

// 'nop' is a single cycle explicit no operation
int instruction_nop(struct s_cpu *cpu, struct s_process *proc) {
  int ret = proc->pc + 1;
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | nop\n", proc->pid);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, ret);
  return (ret);
}

// 'kill' sets the calling process' last_live to 0, reads DIR_SIZE bytes
int instruction_kill(struct s_cpu *cpu, struct s_process *proc) {
  int tokill;
  int player;

  proc->last_live = 0;
  tokill = read_mem_4(cpu->program, proc->pc + 1);
  player = ~tokill;
  if (player >= 0 && player < MAX_PLAYERS) {
    cpu->players[player].last_live = 0;
  }
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | kill %d\n", proc->pid, tokill);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 5);
  return (proc->pc + 5);
}

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
    [e_nop] = instruction_nop,
    [e_kill] = instruction_kill,
};
