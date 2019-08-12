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
    done = instruction_##type(cpu, proc);                                      \
    instruction_calls[e_##type - 1] += done;                                   \
    break;

// bugs: the first instruction will be ran instantly because the
// instruction_time starts at 0 pleas send patches.
static int execute_instruction(struct s_cpu *cpu, struct s_process *proc) {
  int done = 0;

  if (proc->instruction_time) {
    proc->prev_time = proc->instruction_time;
    proc->instruction_time -= 1;
  } else {
    switch (proc->opcode) {
      CASE_INSTRUCTION(live);
      CASE_INSTRUCTION(ld);
      CASE_INSTRUCTION(st);
      CASE_INSTRUCTION(add);
      CASE_INSTRUCTION(sub);
      CASE_INSTRUCTION(and);
      CASE_INSTRUCTION(or);
      CASE_INSTRUCTION(xor);
      CASE_INSTRUCTION(zjmp);
      CASE_INSTRUCTION(ldi);
      CASE_INSTRUCTION(sti);
      CASE_INSTRUCTION(fork);
      CASE_INSTRUCTION(lld);
      CASE_INSTRUCTION(lldi);
      CASE_INSTRUCTION(lfork);
      CASE_INSTRUCTION(aff);
    default:
      if (f_verbose & OPT_INTLDBG)
        puts("NOP: instruction not implemented yet");
      if (proc->pc >= MEM_SIZE)
        proc->pc = 0;
      else
        proc->pc++;
      next(cpu, proc);
      done = 1; // invalid instructions are still instructions
      break;
    }
  }
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
           "pc(%4d) ins_time(%4d) EXEC_INS end --\n",
           cpu->clock, proc->pid, proc->carry,
           proc->last_live, proc->pc,
           proc->instruction_time);
  return done;
}

// valid_header_p validates a header
bool valid_header_p(header_t $header) {
  if ($header.magic != COREWAR_EXEC_MAGIC)
    return false;
  if ($header.prog_size > CHAMP_MAX_SIZE)
    return false;
  return true;
}

// Check for living processes
static void check_alive(struct s_cpu *cpu) {
  struct s_process *proc;

  proc = cpu->processes;
  while (proc != NULL) {
    if (cpu->clock - proc->last_live < (unsigned)cpu->cycle_to_die) {
      proc = proc->next;
    } else {
      if (cpu->active == 1)
        cpu->winner = -(*proc->registers);
      cpu->kill_process(cpu);
    }
  }
  cpu->prev_check = cpu->clock;
  cpu->num_checks++;
  if (NBR_LIVE < cpu->nbr_lives || cpu->num_checks == MAX_CHECKS) {
    cpu->cycle_to_die -= CYCLE_DELTA;
    if (f_verbose & OPT_CYCLES)
      printf("Cycle to die is now %d\n", cpu->cycle_to_die);
    cpu->num_checks = 0;
  }
  cpu->nbr_lives = 0;
}

static int step(struct s_cpu *cpu) {
  int ii;
  int done = 0;
  struct s_process *proc;

  if ((unsigned)cpu->cycle_to_die <= cpu->clock - cpu->prev_check)
    check_alive(cpu);
  cpu->clock += 1;
  if (f_verbose & OPT_CYCLES)
    printf("It is now cycle %zu\n", cpu->clock);
  proc = cpu->processes;
  ii = proc->pid;
  while (proc != 0) {
    done = execute_instruction(cpu, proc);
    if (f_verbose & OPT_INTLDBG)
      printf("DBG: clock(%5zu) pid(%4d) carry(%d) last_live(%5d) "
             "pc(%4d) ins_time(%4d) prev_time(%4d) CPU_STEP loop\n",
             cpu->clock, proc->pid, proc->carry,
             proc->last_live, proc->pc,
             proc->instruction_time, proc->prev_time);
    if (proc->pid > ii) {
      if (f_verbose & OPT_INTLDBG)
        printf("DBG: pid(%d) != ii(%d) CPU_STEP loop\n", proc->pid, ii);
    }
    --ii;
    proc = proc->next;
  }
  return done;
}

// spawn_process makes a new process. sometimes spawn_process allocates more
// space for a few new processes.
static void spawn_process(struct s_cpu *cpu, struct s_process *parent, int pc, int player) {
  struct s_process *done = malloc(sizeof(*done));
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: start SPAWN_PROC\n");
  done->carry = parent ? parent->carry : 0;
  if (pc >= MEM_SIZE)
    pc %= MEM_SIZE;
  if (pc < 0)
    pc += MEM_SIZE;
  done->pc = pc;
  done->pid = cpu->active + 1;
  done->prev_time = -1;
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pc(%4d) op(%02x) SPAWN_PROC\n", done->pc,
           cpu->program[done->pc]);
  done->opcode = cpu->program[done->pc];
  if (done->opcode >= 1 && done->opcode <= 16)
    done->instruction_time = g_op_tab[done->opcode - 1].cycles_to_exec - 1;
  else
    done->instruction_time = 1;
  done->last_live = parent ? parent->last_live : 0;
  if (f_verbose & OPT_INTLDBG)
    fprintf(stderr,
            "DBG: clock(%5zu) pid(%d) ins_time(%4d) op(0x%02x) SPAWN_PROC\n",
            cpu->clock, done->pid, done->instruction_time, done->opcode);
  for (int i = 0; i < REG_NUMBER; i++) {
    done->registers[i] = parent ? parent->registers[i] : 0;
  }
  if (parent == 0) {
    *done->registers = player;
    initial_process(done, cpu);
  } else {
    prepend_process(done, cpu);
  }
  cpu->active += 1;
  next(cpu, done);
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: clock(%5zu) pid(%5d) carry(%d) last_live(%5d) pc(%4d) "
           "ins_time(%4d) prv_time(%4d) SPAWN_PROC end\n",
           cpu->clock, cpu->processes->pid, cpu->processes->carry,
           cpu->processes->last_live, cpu->processes->pc,
           cpu->processes->instruction_time, cpu->processes->prev_time);
}

// delete_process deletes the current process and sets the current process to
// the previous process.
static void delete_process(struct s_cpu *cpu) {
  if (f_verbose & OPT_INTLDBG)
    printf("DBG: pid(%d) num_checks(%d) num_lives(%d) prev_check(%d) "
           "DELET_PROC start\n",
           cpu->processes ? cpu->processes->pid : -1, cpu->num_checks,
           cpu->nbr_lives, cpu->prev_check);
  if (!cpu || !cpu->processes) {
    if (f_verbose & OPT_INTLDBG)
      fprintf(stderr, "Fatal error: cpu or cpu->processes NULL in delete_process\n");
    exit(-1);
    return;
  }
  struct s_process *tmp = cpu->processes;
  cpu->processes = cpu->processes->next;
  if (tmp->next != NULL)
    tmp->next->prev = tmp->prev;
  if (tmp->prev != NULL)
    tmp->prev->next = tmp->next;
  free(tmp);
  // cpu->first = cpu->processes;
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
  struct s_cpu done;
  done.active = 0;
  done.first = 0;
  done.processes = 0;
  done.clock = 0;
  memset(done.players, 0, sizeof(done.players));
  memset(done.program, 0, sizeof(done.program));
  done.program_length = 0;
  done.winner = 0;
  done.cycle_to_die = CYCLE_TO_DIE;
  done.prev_check = 0;
  done.num_checks = 0;
  memset(done.lastlive, 0, sizeof(int) * 4);
  done.spawn_process = spawn_process;
  done.kill_process = delete_process;
  done.step = step;
  done.load = load;

  return done;
}
