#ifndef CPU_H
#define CPU_H

#include "asm.h"
#include "op.h"
#include <stdbool.h>
#include <stdlib.h>

enum instructions {
  e_live = 0x01,
  e_ld = 0x02,
  e_st = 0x03,
  e_add = 0x04,
  e_sub = 0x05,
  e_and = 0x06,
  e_or = 0x07,
  e_xor = 0x08,
  e_zjmp = 0x09,
  e_ldi = 0x0a,
  e_sti = 0x0b,
  e_fork = 0x0c,
  e_lld = 0x0d,
  e_lldi = 0x0e,
  e_lfork = 0x0f,
  e_aff = 0x10
};

#define OPT_SILENT  0x00000000
#define OPT_CYCLES  0x00000001
#define OPT_DEATHS  0x00000002
#define OPT_LIVES   0x00000004
#define OPT_INSTR   0x00000008
#define OPT_PCMOVE  0x00000010
#define OPT_DBGOUT  0x00000020
#define OPT_INTLDBG 0x00000040

struct s_process {
  struct s_process *next;
  struct s_process *prev;
  int instruction_time;
  int prev_time;
  int last_live;
  int opcode;
  int pid;
  int registers[REG_NUMBER];
  int pc;
  bool carry;
};

struct s_cpu {
  struct s_process *first;      //
  struct s_process *processes;  //
  uint8_t program[MEM_SIZE];    // vm core memory.
  int program_length;           // TODO: remove
  int active;                   // total number of active processes.
  size_t clock;                 //
  int lives[MAX_PLAYERS];       // perhaps remove
  int cycle_to_die;             // cycles until processes are removed
  int prev_check;               // previous cycle_to_die
  int num_checks;               // maximum number of checks
  int nbr_lives;                // maximum number of lives per cycle_to_die
  int winner;                   // winning player
  int lastlive[4];              // TODO: remove?

  // spawn_process keeps track of how many *processes are allocated and reallocs
  // if needed. arg1 is the context. arg2 is the program counter. arg3 is r1
  void (*spawn_process)(struct s_cpu *, int, int);
  void (*kill_process)(struct s_cpu *);
  int (*step)(struct s_cpu *);
  void (*load)(struct s_cpu *, char *, uint32_t, uint32_t);
};

/* VM Command Line Flags */
int f_color;          /* enable color in core dump */
int f_dump;           /* dump core memory after ARG cycles */
int f_leaks;          /* call 'pause()' at the end of main */
int f_dump_processes; /* dump processes */
int f_gui;            /* graphical visualizer of vm */
int f_verbose;        /* verbosity level */

extern int instruction_calls[18];

extern void next(struct s_cpu *cpu);

struct s_cpu new_cpu(void);

bool valid_header_p(header_t header);

#endif
