/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cpu.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 18:48:25 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 18:48:26 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CPU_H
# define CPU_H

# include "asm.h"
# include "libft.h"
# include "op.h"
# include <assert.h>
# include <stdbool.h>
# include <stdlib.h>

enum				e_instructions {
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
	e_aff = 0x10,
	e_nop = 0x11,
	e_kill = 0x12,
};

# define OPT_SILENT 0x00000000
# define OPT_LIVES 0x00000001
# define OPT_CYCLES 0x00000002
# define OPT_INSTR 0x00000004
# define OPT_DEATHS 0x00000008
# define OPT_PCMOVE 0x00000010
# define OPT_DBGOUT 0x00000020
# define OPT_INTLDBG 0x00000040

struct				s_process {
	struct s_process	*next;
	struct s_process	*prev;
	int					instruction_time;
	int					prev_time;
	int					last_live;
	int					opcode;
	int					player;
	int					pid;
	int					registers[REG_NUMBER];
	int					args[MAX_ARGS_NUMBER];
	int					pc;
	bool				carry;
	bool				reg_err;
	bool				kill;
};

/*
** write pc blank live
*/

struct				s_mem_colors {
	int player;
	int writes;
};

struct				s_player {
	int		player_number;
	int		active_processes;
	int		last_live;
	int		prog_size;
	char	*name;
	char	*comment;
	bool	kill;
};

/*
** processes                - list of active processes
** program                  - vm core memory.
** active;                  - total number of active processes.
** pid_next;                - next pid to spawn
** clock;                   - current cycle
** cycle_to_die;            - cycles until processes are removed
** prev_check;              - previous cycle_to_die
** num_checks;              - maximum number of live_checks
** nbr_lives;               - maximum number of lives per cycle_to_die
** winner;                  - winning player
** spawn_process keeps track of how many *processes are allocated and reallocs
** if needed. arg1 is the context. arg2 is the program counter. arg3 is r1
*/

struct				s_cpu {
	struct s_process	*processes;
	uint8_t				program[MEM_SIZE];
	int					active;
	int					pid_next;
	int					clock;
	int					cycle_to_die;
	int					prev_check;
	int					num_checks;
	int					nbr_lives;
	int					winner;
	struct s_player		players[MAX_PLAYERS];
	void				(*spawn_process)(struct s_cpu *, int, int);
	void				(*kill_process)(struct s_cpu *, struct s_process **);
	int					(*step)(struct s_cpu *);
	void				(*load)(struct s_cpu *, char *, uint32_t, uint32_t);
};

/*
** VM Command Line Flags
** g_color             - enable color in core dump
** g_dump              - dump core memory after ARG cycles
** g_leaks             - call 'pause()' at the end of main
** g_dump_processes    - dump processes
** g_gui               - graphical visualizer of vm
** g_verbose           - verbosity level
** g_enable_aff        - display AFF output
*/

int					g_color;
int					g_dump;
int					g_leaks;
int					g_dump_processes;
int					g_gui;
int					g_verbose;
int					g_enable_aff;

int					g_instruction_calls[NUM_OPS + 1];

struct s_mem_colors	*g_mem_colors;

void				next_cpu_op(struct s_cpu *cpu, struct s_process *proc);
int					assbutt(struct s_cpu *cpu, struct s_process **proc);
void				initial_process(struct s_process *done, struct s_cpu *cpu);
void				prepend_process(struct s_process *done, struct s_cpu *cpu);
int					run_op(struct s_cpu *cpu, struct s_process *proc);
int					remove_proc(struct s_cpu *cpu, struct s_process **proc,
int cond);
int					run_processes(struct s_cpu *cpu);
void				check_alive(struct s_cpu *cpu);
struct s_cpu		new_cpu(void);

bool				valid_header_p(header_t header);

#endif
