#include "cpu.h"
#include "instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// initial_process links the initial process in a program to itself.
static void initial_process(struct s_process *done, struct s_cpu *cpu) {
  done->next = NULL; // done;
  done->prev = NULL; // done;
  cpu->processes = done;
  // if (cpu->program && cpu->program[0] >= 0x01 && cpu->program[0] <= 0x10)
  //   done->instruction_time = g_op_tab[cpu->program[0]-1].cycles_to_exec;
  // else
}

// prepend_process prepends a process to the beginning of the process list to be
// evaluated next cycle. we prepend because the newest process should go after
// the oldest and before the newest.
static void prepend_process(struct s_process *done, struct s_cpu *cpu) {
  done->next = cpu->processes;
  done->prev = NULL;
  // cpu->first->prev = done;
  // struct s_process *last = cpu->first->prev;
  // last->next = done;
  // done->next = cpu->first;
  // done->prev = last;
  // cpu->first->prev = done;
  cpu->processes = done;
}

#define CASE_INSTRUCTION(type)                                                 \
  case e_##type:                                                               \
    proc->pc = instruction_##type(cpu, proc);                                  \
    instruction_calls[e_##type - 1] += 1;                                      \
    break;

// bugs: the first instruction will be ran instantly because the
// instruction_time starts at 0 pleas send patches.
// static int execute_instruction(struct s_cpu *cpu, struct s_process *proc) {
//   int done = 0;

//   if (proc->instruction_time != 0) {
//     proc->prev_time = proc->instruction_time;
//     proc->instruction_time -= 1;
//   } else {
//     switch (proc->opcode) {
//       CASE_INSTRUCTION(live);
//       CASE_INSTRUCTION(ld);
//       CASE_INSTRUCTION(st);
//       CASE_INSTRUCTION(add);
//       CASE_INSTRUCTION(sub);
//       CASE_INSTRUCTION(and);
//       CASE_INSTRUCTION(or);
//       CASE_INSTRUCTION(xor);
//       CASE_INSTRUCTION(zjmp);
//       CASE_INSTRUCTION(ldi);
//       CASE_INSTRUCTION(sti);
//       CASE_INSTRUCTION(fork);
//       CASE_INSTRUCTION(lld);
//       CASE_INSTRUCTION(lldi);
//       CASE_INSTRUCTION(lfork);
//       CASE_INSTRUCTION(aff);
//     default:
//       if (f_verbose & OPT_INTLDBG)
//         puts("NOP: instruction not implemented yet");
//       done = 1; // invalid instructions are still instructions
//       break;
//     }
//     if (proc->opcode == 0)
//       next_instruction(cpu, proc);
//   }
//   if (f_verbose & OPT_INTLDBG)
//     printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
//            "pc(%4d) ins_time(%4d) prev_time(%4d) player(%d) EXEC_INS end
//            --\n", cpu->clock, proc->pid, proc->carry, proc->last_live,
//            proc->pc, proc->instruction_time, proc->prev_time, proc->player);
//   return done;
// }
extern void ft_nop(void);
// valid_header_p validates a header
bool valid_header_p(header_t $header) {
  if ($header.magic != COREWAR_EXEC_MAGIC)
    return false;
  if ($header.prog_size > CHAMP_MAX_SIZE)
    return false;
  return true;
}

// execute op or decrement instruction_time
int run_op(struct s_cpu *cpu, struct s_process *proc) {
  int new_pc;

  proc->instruction_time -= 1;
  if (proc->instruction_time == 0) {
    if (f_verbose & OPT_INTLDBG)
      printf("DBG: proc->opcode(%02x) mem(%02x)\n", proc->opcode, cpu->program[proc->pc]);
    new_pc = inst_tab[proc->opcode](cpu, proc);
    if (cpu->program[2792] == 0xcd) ft_nop();
    if (new_pc >= MEM_SIZE)
      new_pc %= MEM_SIZE;
    instruction_calls[proc->opcode] += 1;
    if (new_pc < 0)
      new_pc += MEM_SIZE;
    proc->pc = new_pc;
    proc->opcode = 0;
    return 1;
  }
  return 0;
}

// run processes
int run_processes(struct s_cpu *cpu) {
  struct s_process *proc;
  int ret = 0;

  proc = cpu->processes;
  while (proc != 0) {
    if (proc->opcode == 0)
      next_cpu_op(cpu, proc);
    if (proc->opcode != 0)
      ret = run_op(cpu, proc);
    proc = proc->next;
  }
  return ret;
}
// Check for living processes
static void check_alive(struct s_cpu *cpu) {
  struct s_process *proc;

  proc = cpu->processes;
  while (proc != NULL) {
    // printf("DBG: stupid(%d)\n", cpu->clock - proc->last_live);
    if ((cpu->clock - proc->last_live) < cpu->cycle_to_die) {
      if (!proc || !proc->next)
        break;
      proc = proc->next;
    } else {
      if (cpu->active == 0)
        cpu->winner = 1;
      cpu->kill_process(cpu, &proc);
    }
  }
  cpu->prev_check = cpu->clock;
  cpu->num_checks += 1;
  if (NBR_LIVE < cpu->nbr_lives || cpu->num_checks == MAX_CHECKS) {
    cpu->cycle_to_die -= CYCLE_DELTA;
    if (f_verbose & OPT_CYCLES)
      printf("Cycle to die is now %d\n", cpu->cycle_to_die);
    cpu->num_checks = 0;
  }
  cpu->nbr_lives = 0;
}

static int step(struct s_cpu *cpu) {
  if (cpu == NULL) {
    fprintf(stderr, "Fatal Error: cpu is NULL in step()\n");
    exit(-2);
  }
  cpu->clock += 1;
  if (f_verbose & OPT_CYCLES)
    printf("It is now cycle %d\n", cpu->clock);
  // printf("DBG: nbr_lives(%d) prev_check(%d) cycledie(%d)\n", cpu->nbr_lives, cpu->prev_check, cpu->cycle_to_die);
  int ret = run_processes(cpu);
  if (cpu->cycle_to_die <= cpu->clock - cpu->prev_check)
    check_alive(cpu);
  // int done = 0;
  // struct s_process *proc;
  // proc = cpu->processes;
  // while (proc != 0) {
  //   if (proc->opcode == 0)
  //     next(cpu, proc);
  //   done = execute_instruction(cpu, proc);
  //   if (f_verbose & OPT_INTLDBG)
  //     printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
  //            "pc(%4d) ins_time(%4d) prev_time(%4d) player(%d) CPU_STEP
  //            loop\n", cpu->clock, proc->pid, proc->carry, proc->last_live,
  //            proc->pc, proc->instruction_time, proc->prev_time,
  //            proc->player);
  //   proc = proc->next;
  // }
  return ret;
}

// spawn_process makes a new process. sometimes spawn_process allocates more
// space for a few new processes.
static void spawn_process(struct s_cpu *cpu, int pc, int player) {
  struct s_process *done = malloc(sizeof(*done));

  if (done == NULL) {
    perror("Fatal error");
    exit(-1);
  }
  bzero(done, sizeof(*done));
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: clock(%5d) pc(%4d) player(%d) SPAWN_PROC start\n", cpu->clock,
           pc, player);
  done->carry = 0;
  if (pc >= MEM_SIZE)
    pc %= MEM_SIZE;
  if (pc < 0)
    pc += MEM_SIZE;
  if (player >= -4 && player <= -1) {
    done->player = player;
  }
  done->pc = pc;
  done->pid = cpu->active + 1;
  done->prev_time = 0;
  done->opcode = 0;
  done->instruction_time = 0;
  done->last_live = 0;
  for (int i = 0; i < REG_NUMBER; i++) {
    done->registers[i] = 0;
  }
  *done->registers = player;
  if (cpu->processes == 0) {
    initial_process(done, cpu);
  } else {
    prepend_process(done, cpu);
  }
  cpu->active += 1;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: clock(%5d) pid(%4d) carry(%d) last_live(%5d) pc(%4d) "
           "ins_time(%4d) prev_time(%4d) player(%d) SPAWN_PROC end\n",
           cpu->clock, cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc,
           cpu->processes->instruction_time, cpu->processes->prev_time,
           cpu->processes->player);
}

// delete_process deletes the current process and sets the current process to
// the previous process.
static void delete_process(struct s_cpu *cpu, struct s_process **proc) {
  // if (f_verbose & OPT_INTLDBG) {
  //   printf("DBG: pid(%d) num_checks(%d) nbr_lives(%d) prev_check(%d) "
  //          "DELET_PROC start\n",
  //          cpu->processes ? cpu->processes->pid : -1, cpu->num_checks,
  //          cpu->nbr_lives, cpu->prev_check);
  // }
  // if (!cpu) {
  //   if (f_verbose & OPT_INTLDBG)
  //     fprintf(stderr,
  //             "Fatal error: cpu or cpu->processes NULL in delete_process\n");
  //   exit(-1);
  //   return;
  // }
  // if (cpu->winner == 1)
    // return;
  if(proc == 0 || *proc == 0) {
    fprintf(stderr, "ERROR: proc is null in delete_process\n");
    return;
  }
  struct s_process *current = *proc;
  if (f_verbose & OPT_DEATHS)
    printf("Process %d hasn\'t lived for %d cycles (CTD %d)\n", current->pid, cpu->clock - current->last_live, cpu->cycle_to_die);
  *proc = (*proc)->next;
  if (current->next != NULL)
    current->next->prev = current->prev;
  if (current->prev != NULL)
    current->prev->next = current->next;
  current->next = 0;
  current->prev = 0;
  free(current);
  cpu->active -= 1;
}

// load loads a PROGRAM of length LENGTH into memory address ADDRESS.
static void load(struct s_cpu *cpu, char *program, uint32_t length,
                 uint32_t address) {
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: length(%d) address(%d) LOAD start\n", length, address);
  for (uint32_t i = 0; i < length; i++) {
    cpu->program[i + address] = (uint8_t)program[i];
  }
  cpu->program_length = MEM_SIZE; // length;
}

// new_cpu makes a new cpu.
struct s_cpu new_cpu(void) {
  static struct s_cpu done;
  done.active = 0;
  // done.first = 0; // TODO: remove
  done.processes = 0;
  done.clock = 0;
  memset(done.players, 0, sizeof(done.players));
  memset(done.program, 0, sizeof(done.program));
  done.program_length = 0;
  done.winner = 0;
  done.cycle_to_die = CYCLE_TO_DIE;
  done.prev_check = 0;
  done.num_checks = 0;
  done.nbr_lives = 0;
  memset(done.lastlive, 0, sizeof(int) * 4);
  done.spawn_process = spawn_process;
  done.kill_process = delete_process;
  done.step = step;
  done.load = load;

  return done;
}
