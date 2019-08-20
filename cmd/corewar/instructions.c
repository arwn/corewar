#include "cpu.h"
#include "op.h"
#include "util.h"
#include <stdio.h>

// clang-format off
// must be included AFTER cpu.h
#include "instructions.h"
// clang-format on

// Write N bytes of CPU->PROGRAM at offset PC
void dump_nbytes(struct s_cpu *cpu, int n, int pc, int opt) {
  if (opt)
    printf("%04x  ", pc);
  for (int i = 0; i < n; i++) {
    printf("%02hhx%c", cpu->program[pc + i], (i % n == n - 1) ? '\n' : ' ');
  }
}

// Get the type of the N'th argument from the pcb
int type_from_pcb(uint8_t pcb, int arg) {
  uint8_t param;
  int ret;

  param = (pcb >> (('\x03' - (char)arg) * '\x02' & 0x1fU)) & 3;
  if (param == DIR_CODE) {
    ret = T_DIR;
  } else {
    if (param == IND_CODE) {
      ret = T_IND;
    } else {
      if (param == REG_CODE) {
        ret = T_REG;
      } else {
        ret = 0;
      }
    }
  }
  return ret;
}

int size_from_pt(int type, int opcode) {
  int ret;

  if (type == T_REG) {
    ret = REG_SIZE;
  } else {
    if (type == T_IND) {
      ret = IND_SIZE;
    } else {
      if (type == T_DIR) {
        if (g_op_tab[opcode].direct_size == 0)
          ret = DIR_SIZE;
        else
          ret = SPECIAL_DIR_SIZE;
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
  int type;

  ii = 0;
  while (ii < g_op_tab[op].numargs) {
    type = type_from_pcb(pcb, ii);
    if (((type & g_op_tab[op].argtypes[ii]) != type) || type == 0) {
      return 0;
    }
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

// Write four bytes of VAL into core memory MEM at offset IDX
// TODO: cleanup
static void write_mem_ins(struct s_process *proc, uint8_t *mem, uint32_t idx,
                          uint32_t val) {
  const int idx1 = idx % MEM_SIZE;
  const int idx2 = (idx + 1) % MEM_SIZE;
  const int idx3 = (idx + 2) % MEM_SIZE;
  const int idx4 = (idx + 3) % MEM_SIZE;

  if (f_color || f_gui) {
    if (g_mem_colors[idx1].writes == 0 ||
        g_mem_colors[idx1].player != proc->player)
      g_mem_colors[idx1].writes = 49;
    if (!g_mem_colors[idx1].player || g_mem_colors[idx1].player != proc->player)
      g_mem_colors[idx1].player = proc->player;

    if (g_mem_colors[idx2].writes == 0 ||
        g_mem_colors[idx2].player != proc->player)
      g_mem_colors[idx2].writes = 49;
    if (!g_mem_colors[idx2].player || g_mem_colors[idx2].player != proc->player)
      g_mem_colors[idx2].player = proc->player;

    if (g_mem_colors[idx3].writes == 0 ||
        g_mem_colors[idx3].player != proc->player)
      g_mem_colors[idx3].writes = 49;
    if (!g_mem_colors[idx3].player || g_mem_colors[idx3].player != proc->player)
      g_mem_colors[idx3].player = proc->player;

    if (g_mem_colors[idx4].writes == 0 ||
        g_mem_colors[idx4].player != proc->player)
      g_mem_colors[idx4].writes = 49;
    if (!g_mem_colors[idx4].player || g_mem_colors[idx4].player != proc->player)
      g_mem_colors[idx4].player = proc->player;
  }

  mem[idx1] = (val >> 24) & 0xff;
  mem[idx2] = (val >> 16) & 0xff;
  mem[idx3] = (val >> 8) & 0xff;
  mem[idx4] = val & 0xff;
}

// Write VAL into register REG for the process PROC
static void write_reg(struct s_process *proc, uint32_t reg, uint32_t val) {
  if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
    printf("DBG: write_reg reg(%08x) val(%08x) lav(%08x)\n", reg, val,
           ntohl(val));
  if (reg > 0 && reg <= REG_NUMBER)
    proc->registers[reg - 1] = val;
}

// Read register REG for the process PROC
static int read_reg(struct s_process *proc, uint32_t reg) {
  if (reg > 0 && reg <= REG_NUMBER) {
    int ret = proc->registers[reg - 1];
    if ((f_verbose & OPT_INTLDBG) && (f_verbose & OPT_INTLDBG))
      printf("DBG: read_reg reg(%08x) ger(%08x)\n", ret, ntohl(ret));
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
int validate_register(int reg) { return reg > 0 && reg <= NUM_OPS; }

// ld takes 2 parameters, 2nd must be a register that isn't the
// 'program counter'. It loads the value of the first parameter in the register,
// and modifies the 'carry'. 'ld 34,r3' loads the REG_SIZE bytes from address
// (PC + (34 % IDX_MOD)) in register r3.
int instruction_ld(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int val;
  int reg;
  int type;
  int size_type;
  int valid_reg;
  int test;
  int size;

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
    if (type == T_DIR) {
      val = (int)read_mem_4(cpu->program, proc->pc + 2);
      if (f_verbose & OPT_INTLDBG)
        printf("DBG: val(%08x) T_DIR INS_LD\n", val);
    } else {
      val = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      if (f_verbose & OPT_INTLDBG)
        printf("DBG: val(%08x) T_IND INS_LD\n", val);
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
  return proc->pc + size + 2;
}

// st takes 2 parameters, storing (REG_SIZE bytes) of the value of
// the first argument (always a register) in the second. 'st r4,34' stores the
// value of 'r4' at the address (PC + (34 % IDX_MOD)) 'st r3,r8' copies the
// contents of 'r3' to 'r8'
// TODO: validate pcb
int instruction_st(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int check;
  int type;
  int valid_reg;
  int val;
  int reg;
  int ivar2;
  int uvar3;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  check = check_pcb(pcb, 3);
  if (check != 0) {
    reg = read_mem_1(cpu->program, proc->pc + 2);
    type = type_from_pcb(pcb, 1);
    if (type == T_IND) {
      val = (short)read_mem_2(cpu->program, proc->pc + 3);
    } else {
      val = read_mem_1(cpu->program, proc->pc + 3);
    }
    valid_reg = validate_register(reg);
    if (valid_reg != 0) {
      if ((type == T_REG) && (valid_reg = validate_register(val)) != 0) {
        if (f_verbose & OPT_INSTR)
          printf("P% 5d | st r%d %d\n", proc->pid, reg, val);
        reg = read_reg(proc, reg);
        write_reg(proc, val, reg);
      } else {
        if (type == T_IND) {
          if (f_verbose & OPT_INSTR)
            printf("P% 5d | st r%d %d\n", proc->pid, reg, val);
          ivar2 = proc->pc;
          uvar3 = read_reg(proc, reg);
          write_mem_ins(proc, cpu->program, (ivar2 + val % IDX_MOD), uvar3);
        }
      }
    }
  }
  reg = size_from_pcb(pcb, 3);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + reg + 2);
  return proc->pc + reg + 2;
}

// add takes 3 registers as parameters, adding the contents of the
// first and second, storing the result into the third. Modifies carry. 'add
// r2,r3,r5' adds the values of 'r2' and 'r3' and stores the result in 'r5'.
// TODO: validate pcb
int instruction_add(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  int val;
  int reg1;
  int reg2;
  int reg3;
  int ivar2;
  int uvar3;
  int uvar4;

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
  return proc->pc + val + 2;
}

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
  return proc->pc + val + 2;
}

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
    if (type == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 6);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        return type + 2 + uvar3;
      }
      uvar4 = read_reg(proc, uvar1);
      local_34 = uvar4;
    } else {
      if (type == T_IND) {
        local_34 =
            read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (int)read_mem_4(cpu->program, proc->pc + 2);
      }
    }
    type = type_from_pcb(pcb, 1);
    uvar4 = size_from_pt(type, 6);
    if (type == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 6);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        return type + 2 + uvar3;
      }
      uvar5 = read_reg(proc, uvar1);
      local_40 = uvar5;
    } else {
      if (type == T_IND) {
        local_40 = read_indirect(
            cpu, proc, read_mem_2(cpu->program, proc->pc + 2 + ivar2));
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
  return type + 2 + uvar3;
}

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
    if (type == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        return type + 2 + uvar3;
      }
      uvar4 = read_reg(proc, uvar1);
      local_34 = uvar4;
    } else {
      if (type == T_IND) {
        local_34 =
            read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (int)read_mem_4(cpu->program, proc->pc + 2);
      }
    }
    type = type_from_pcb(pcb, 1);
    uvar4 = size_from_pt(type, 7);
    if (type == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        return type + 2 + uvar3;
      }
      uvar5 = read_reg(proc, uvar1);
      local_40 = uvar5;
    } else {
      if (type == T_IND) {
        local_40 = read_indirect(
            cpu, proc, read_mem_2(cpu->program, proc->pc + 2 + ivar2));
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
  return type + 2 + uvar3;
}

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
    if (type == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        return type + 2 + uvar3;
      }
      uvar4 = read_reg(proc, uvar1);
      local_34 = uvar4;
    } else {
      if (type == T_IND) {
        local_34 =
            read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (int)read_mem_4(cpu->program, proc->pc + 2);
      }
    }
    type = type_from_pcb(pcb, 1);
    uvar4 = size_from_pt(type, 7);
    if (type == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 7);
        if (f_verbose & OPT_PCMOVE) {
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        return type + 2 + uvar3;
      }
      uvar5 = read_reg(proc, uvar1);
      local_40 = uvar5;
    } else {
      if (type == T_IND) {
        local_40 = read_indirect(
            cpu, proc, read_mem_2(cpu->program, proc->pc + 2 + ivar2));
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
  return type + 2 + uvar3;
}

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
  return local_c;
}

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
    if (type == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 10);
        if (f_verbose & OPT_PCMOVE)
          print_adv(cpu, proc, type + 2 + uvar3);
        return type + 2 + uvar3;
      }
      local_34 = read_reg(proc, uvar1);
    } else {
      if (type == T_IND) {
        local_34 =
            read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        local_34 = (short)read_mem_2(cpu->program, proc->pc + 2);
      }
    }
    uvar4 = type_from_pcb(pcb, 1);
    uvar5 = size_from_pt(uvar4, 10);
    if (uvar4 == T_REG) {
      uvar1 = read_mem_1(cpu->program, proc->pc + 2 + ivar2);
      type = validate_register(uvar1);
      if (type == 0) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 10);
        if (f_verbose & OPT_PCMOVE)
          print_adv(cpu, proc, type + 2 + uvar3);
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
      return type + 2 + uvar3;
    }
    int idx = proc->pc + (local_34 + local_40) % IDX_MOD;
    uvar3 = read_mem_4(cpu->program, idx);
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | ldi %d %d r%d\n", proc->pid, local_34, local_40, uvar1);
      printf("       | -> load from %d + %d = %d (with pc and mod %d)\n",
             local_34, local_40, local_34 + local_40, (idx));
    }
    write_reg(proc, uvar1, uvar3);
  }
  type = proc->pc;
  uvar3 = size_from_pcb(pcb, 10);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, type + 2 + uvar3);
  return type + 2 + uvar3;
}

// sti stores at an index offset. 'sti r2,%4,%5' copies REG_SIZE bytes
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
    if (type == T_REG) {
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
      if (type == T_IND) {
        local_3c =
            read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 3));
      } else {
        local_3c = (short)read_mem_2(cpu->program, proc->pc + 3);
      }
    }
    uvar3 = type_from_pcb(pcb, 2);
    if (uvar3 == T_REG) {
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
    write_mem_ins(proc, cpu->program, idx, uvar2);
  }
  reg = size_from_pcb(pcb, 0xb);
  if (f_verbose & OPT_PCMOVE) {
    print_adv(cpu, proc, proc->pc + 2 + reg);
  }
  local_c = proc->pc + 2 + reg;
LAB_100007631:
  return local_c;
}

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

// fork always takes an index and creates a new program which is
// executed from address (PC + ('idx' % IDX_MOD)). 'fork %34' spawns a new
// process at (PC + (34 % IDX_MOD)). helltrain cycles (1105,1935,2745,3555,4365)
// TOOD: handle process calling fork correctly: cycle 2745,
//       pid(3).instruction_time is decremented twice
int instruction_fork(struct s_cpu *cpu, struct s_process *proc) {
  int idx;
  short uvar2;

  uvar2 = read_mem_2(cpu->program, proc->pc + 1);
  idx = uvar2 % IDX_MOD;
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | fork %d (%d)\n", proc->pid, uvar2, proc->pc + idx);
  fork_process(cpu, proc, proc->pc + idx);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 3);
  return proc->pc + 3;
}

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
    if (uvar4 == T_DIR) {
      local_2c = (int)read_mem_4(cpu->program, proc->pc + 2);
    } else {
      local_2c = (short)read_mem_2(
          cpu->program, proc->pc + read_mem_2(cpu->program, proc->pc + 2));
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
  return (proc->pc + 2 + uvar3);
}

// lldi is the same as 'ldi', but without the (% IDX_MOD). Modifies
// carry. 'lldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3)), adding 4
// to this sum S. Read REG_SIZE bytes at address (PC + (S)), which are copied to
// 'r1'. TODO: fix broken
int instruction_lldi(struct s_cpu *cpu, struct s_process *proc) {
  uint8_t pcb;
  uint8_t reg;
  int size1;
  int type;
  int uvar3;
  int uvar4;
  int type2;
  int val1;
  int val2;
  int ret;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  uvar4 = check_pcb(pcb, 0xe);
  if (uvar4 != 0) {
    uvar4 = type_from_pcb(pcb, 0);
    type = uvar4;
    uvar4 = size_from_pt(type, 0xe);
    size1 = uvar4;
    if (type == T_REG) {
      reg = read_mem_1(cpu->program, proc->pc + 2);
      type = validate_register(reg);
      if (type == 0) {
        if (f_verbose & OPT_PCMOVE) {
          type = proc->pc;
          uvar3 = size_from_pcb(pcb, 0xe);
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        type = proc->pc;
        ret = size_from_pcb(pcb, 0xe);
        ret = type + 2 + ret;
        return ret;
      }
      val1 = read_reg(proc, reg);
    } else {
      if (type == T_IND) {
        val1 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 2));
      } else {
        val1 = (short)read_mem_2(cpu->program, proc->pc + 2);
      }
    }
    uvar4 = type_from_pcb(pcb, 1);
    type2 = size_from_pt(uvar4, 0xe);
    if (uvar4 == T_REG) {
      reg = read_mem_1(cpu->program, proc->pc + 2 + size1);
      type = validate_register(reg);
      if (type == 0) {
        if (f_verbose & OPT_PCMOVE) {
          type = proc->pc;
          uvar3 = size_from_pcb(pcb, 0xe);
          print_adv(cpu, proc, type + 2 + uvar3);
        }
        type = proc->pc;
        ret = size_from_pcb(pcb, 0xe);
        ret = type + 2 + ret;
        return ret;
      }
      val2 = read_reg(proc, reg);
    } else {
      val2 = (short)read_mem_2(cpu->program, proc->pc + 2 + size1);
    }
    reg = read_mem_1(cpu->program, proc->pc + 2 + size1 + type2);
    type = validate_register(reg);
    if (type == 0) {
      if (f_verbose & OPT_PCMOVE) {
        type = proc->pc;
        uvar3 = size_from_pcb(pcb, 0xe);
        print_adv(cpu, proc, type + 2 + uvar3);
      }
      type = proc->pc;
      ret = size_from_pcb(pcb, 0xe);
      ret = type + 2 + ret;
      return ret;
    }
    uvar3 = read_mem_4(cpu->program, proc->pc + val1 + val2);
    if (f_verbose & OPT_INSTR) {
      printf("P% 5d | lldi %d %d r%d\n", proc->pid, val1, val2, reg);
    }
    if (f_verbose & OPT_INSTR) {
      printf("       | -> load from %d + %d = %d (with pc %d)\n", val1, val2,
             val1 + val2, proc->pc + val1 + val2);
    }
    mod_carry(proc, (uvar3 == 0));
    write_reg(proc, reg, uvar3);
  }
  if (f_verbose & OPT_PCMOVE) {
    type = proc->pc;
    uvar3 = size_from_pcb(pcb, 0xe);
    print_adv(cpu, proc, type + 2 + uvar3);
  }
  type = proc->pc;
  ret = size_from_pcb(pcb, 0xe);
  ret = type + 2 + ret;
  return ret;
}

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
  return (proc->pc + 3);
}

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
    if ((ivar2 != 0) && ((uvar1 = read_reg(proc, uvar1)), f_enable_aff != 0)) {
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
  return (ivar2 + 2 + uvar3);
}

// explicit no operation
int instruction_nop(struct s_cpu *cpu, struct s_process *proc) {
  int ret = proc->pc + 1;
  if (f_verbose & OPT_INSTR)
    printf("P% 5d | nop\n", proc->pid);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, ret);
  return (ret);
}

// inverse of live
int instruction_kill(struct s_cpu *cpu, struct s_process *proc) {
  int tokill;

  tokill = read_mem_4(cpu->program, proc->pc + 1);
  proc->last_live = 0;
  int player = ~tokill;
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
