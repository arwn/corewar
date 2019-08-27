#include "cpu.h"
#include "op.h"
#include "util.h"
#include <stdio.h>

// clang-format off
// must be included AFTER cpu.h
#include "instructions.h"
// clang-format on

#define OUT(...) printf(__VA_ARGS__)
// 'aff' takes a register and writes the stored value modulo 256 to
// stdout. 'ld %52,r3  aff r3' displays '*' on stdout.
int instruction_aff(struct s_cpu *cpu, struct s_process *proc) {
  t_arg_type pcb;
  uint8_t arg1;

  pcb = read_mem_1(cpu->program, proc->pc + 1);
  if (check_pcb(pcb, e_aff) != 0) {
    arg1 = read_mem_1(cpu->program, proc->pc + 2);
    if (valid_reg(arg1) != 0) {
      arg1 = read_reg(proc, arg1);
      if (f_enable_aff != 0)
        OUT("Aff: %c\n", arg1);
    }
  }
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_aff));
  return (proc->pc + 2 + size_from_pcb(pcb, e_aff));
}

// 'nop' is a single cycle explicit no operation
int instruction_nop(struct s_cpu *cpu, struct s_process *proc)
{
  int ret = proc->pc + 1;
  if (f_verbose & OPT_INSTR)
    OUT("P% 5d | nop\n", proc->pid);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, ret);
  return (ret);
}

// 'kill' sets the calling process' last_live to 0, reads DIR_SIZE bytes
int instruction_kill(struct s_cpu *cpu, struct s_process *proc)
{
  int tokill;
  int player;

  proc->last_live = 0;
  tokill = read_mem_4(cpu->program, proc->pc + 1);
  player = ~tokill;
  if (player >= 0 && player < MAX_PLAYERS) {
    cpu->players[player].last_live = 0;
  }
  if (f_verbose & OPT_INSTR)
    OUT("P% 5d | kill %d\n", proc->pid, tokill);
  if (f_verbose & OPT_PCMOVE)
    print_adv(cpu, proc, proc->pc + 5);
  return (proc->pc + 5);
}
