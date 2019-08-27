#include "cpu.h"
#include "op.h"
#include "util.h"
#include <stdio.h>

// clang-format off
// must be included AFTER cpu.h
#include "instructions.h"
// clang-format on

#define OUT(...) printf(__VA_ARGS__)

// INSTRUCTION_BITWISE generates an instruction function withe name and operator
#define INSTRUCTION_BITWISE(name, operator)                                    \
  int instruction_##name(struct s_cpu *cpu, struct s_process *proc) {          \
    t_arg_type pcb;                                                            \
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
      if (valid_reg(arg3) != 0) {                                              \
        if (f_verbose & OPT_INSTR)                                             \
          OUT("P% 5d | %s %d %d r%d\n", proc->pid, #name, arg1, arg2, arg3);   \
        proc->carry = ((arg1 operator arg2) == 0);                             \
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
int instruction_zjmp(struct s_cpu *cpu, struct s_process *proc)
{
  short arg1;
  int ret;

  arg1 = (short)read_mem_2(cpu->program, proc->pc + 1);
  ret = proc->pc + 3;
  if (proc->carry == 0) {
    if (f_verbose & OPT_INSTR) {
      OUT("P% 5d | zjmp %d FAILED\n", proc->pid, arg1);
    }
    if (f_verbose & OPT_PCMOVE)
      print_adv(cpu, proc, ret);
  } else {
    if (f_verbose & OPT_INSTR)
      OUT("P% 5d | zjmp %d OK\n", proc->pid, arg1);
    ret = proc->pc + arg1 % IDX_MOD;
  }
  return ret;
}

// ldi modifies carry. 'idx' and 'add' are indexes, and 'reg' is a
// register. 'ldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3 %
// IDX_MOD)), adding 4 to this sum S. Read REG_SIZE bytes at address (PC + (S %
// IDX_MOD)), which are copied to 'r1'.
int instruction_ldi(struct s_cpu *cpu, struct s_process *proc)
{
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
      if (valid_reg(arg1) == 0)
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
      if (valid_reg(arg2) == 0)
        break;
      arg2 = read_reg(proc, arg2);
    } else
      arg2 = (short)read_mem_2(cpu->program, offset);
    offset += size_from_pt(type, e_ldi);
    arg3 = read_mem_1(cpu->program, offset);
    if (valid_reg(arg3) == 0)
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
int instruction_sti(struct s_cpu *cpu, struct s_process *proc)
{
  t_arg_type pcb;
  int type;
  int arg1;
  int arg2;
  int arg3;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  while (check_pcb(pcb, e_sti) != 0) {
    arg1 = read_mem_1(cpu->program, proc->pc + 2);
    if (valid_reg(arg1) == 0)
      break;
    type = type_from_pcb(pcb, 1);
    if (type == T_REG) {
      arg2 = read_mem_1(cpu->program, proc->pc + 3);
      if (valid_reg(arg2) == 0)
        break;
      arg2 = read_reg(proc, arg2);
    } else if (type == T_IND)
      arg2 = read_indirect(cpu, proc, read_mem_2(cpu->program, proc->pc + 3));
    else
      arg2 = (short)read_mem_2(cpu->program, proc->pc + 3);
    if (type_from_pcb(pcb, 2) == T_REG) {
      arg3 = read_mem_1(cpu->program, proc->pc + 3 + size_from_pt(type, e_sti));
      if (valid_reg(arg3) == 0)
        break;
      arg3 = read_reg(proc, arg3);
    } else
      arg3 = (short)read_mem_2(cpu->program, proc->pc + 3 + size_from_pt(type, e_sti));
    if (f_verbose & OPT_INSTR) {
      OUT("P% 5d | sti r%d %d %d\n", proc->pid, arg1, arg2, arg3);
      OUT("       | -> store to %d + %d = %d (with pc and mod %d)\n", arg2,
          arg3, (arg2 + arg3), proc->pc + (arg2 + arg3) % IDX_MOD);
    }
    write_mem_ins(proc, cpu->program, mod_idx(proc->pc + (arg2 + arg3) % IDX_MOD), read_reg(proc, arg1));
    break;
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_sti));
  return (proc->pc + 2 + size_from_pcb(pcb, e_sti));
}
