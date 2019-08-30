/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   instructions_15.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/27 16:23:49 by callen            #+#    #+#             */
/*   Updated: 2019/08/27 16:23:52 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "instructions.h"

#define OUT(...) printf(__VA_ARGS__)

#define ERRORBAD (perror("Fatal error"), -1)

/*
** Allocate and initialize the new process with the state of the parent process
*/

static void	fork_process(struct s_cpu *cpu, struct s_process *proc, int idx)
{
	struct s_process	*new;
	int					ii;
	int					pl;

	if ((new = malloc(sizeof(*new))) == NULL)
		exit(ERRORBAD);
	ft_bzero(new, sizeof(*new));
	new->pc = mod_idx(idx);
	pl = ~proc->player;
	if (pl >= 0 && pl < MAX_PLAYERS)
		cpu->players[pl].active_processes += 1;
	new->carry = proc->carry;
	new->player = proc->player;
	new->last_live = proc->last_live;
	new->pid = cpu->pid_next + 1;
	ii = -1;
	while (++ii < REG_NUMBER)
		new->registers[ii] = proc->registers[ii];
	new->next = cpu->processes;
	new->next->prev = new;
	cpu->processes = new;
	cpu->active += 1;
	cpu->pid_next += 1;
}

/*
** 'fork' always takes an index and creates a new program which is
** executed from address (PC + ('idx' % IDX_MOD)). 'fork %34' spawns a new
** process at (PC + (34 % IDX_MOD)).
*/

int			instruction_fork(struct s_cpu *cpu, struct s_process *proc)
{
	int		idx;
	short	arg1;

	arg1 = read_mem_2(cpu->program, proc->pc + 1);
	idx = arg1 % IDX_MOD;
	if (g_verbose & OPT_INSTR)
		OUT("P% 5d | fork %d (%d)\n", proc->pid, arg1, proc->pc + idx);
	fork_process(cpu, proc, proc->pc + idx);
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 3);
	return (proc->pc + 3);
}

/*
** 'lld' is the same as 'ld', but without the (% IDX_MOD). Modifies
** carry. 'lld 34,r3' loads the REG_SIZE bytes from address (PC + (34)) in
** register r3.
*/

#define LLDPRINT OUT("P% 5d | lld %d r%d\n", proc->pid, arg1, arg2)
#define PRINT_INSTR (g_verbose & OPT_INSTR) ? LLDPRINT : 0

int			instruction_lld(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	uint8_t		arg2;
	int			type;
	int			arg1;

	pcb = read_mem_1(cpu->program, proc->pc + 1);
	if (check_pcb(pcb, e_lld) != 0)
	{
		if ((type = type_from_pcb(pcb, 0)) == T_DIR)
			arg1 = (int)read_mem_4(cpu->program, proc->pc + 2);
		else
			arg1 = (short)read_mem_2(cpu->program, proc->pc
				+ read_mem_2(cpu->program, proc->pc + 2));
		arg2 = read_mem_1(cpu->program, proc->pc
			+ 2 + size_from_pt(type, e_lld));
		if (valid_reg(arg2) != 0)
		{
			PRINT_INSTR;
			proc->carry = arg1 == 0;
			write_reg(proc, arg2, arg1);
		}
	}
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_lld));
	return (proc->pc + 2 + size_from_pcb(pcb, e_lld));
}

/*
** 'lldi' is the same as 'ldi', but without the (% IDX_MOD). Modifies
** carry. 'lldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3)), adding 4
** to this sum S. Read REG_SIZE bytes at address (PC + (S)), which are copied to
** 'r1'.
*/

#define A1 arg12[0]
#define A2 arg12[1]
#define LLDIPS1 "P% 5d | lldi %d %d r%d\n"
#define LLDIPS2 "       | -> load from %d + %d = %d (with pc %d)\n"
#define LLDIPA1 proc->pid, A1, A2, arg3
#define LLDIPA2 A1, A2, A1+A2,proc->pc+A1+A2
#define LLDIPRINT (g_verbose&OPT_INSTR)?OUT(LLDIPS1 LLDIPS2,LLDIPA1,LLDIPA2):0
#define TYFP(N) (type = type_from_pcb(pcb, (N)))
#define RPCB (pcb = read_mem_1(cpu->program, proc->pc + 1))
#define VALARG3 (valid_reg((arg3 = read_mem_1(cpu->program, ofs))) == 0)
#define SHOULD_BREAK (VALARG3 || proc->reg_err)

int			instruction_lldi(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	uint8_t		arg3;
	int			ofs;
	int			type;
	int			arg12[2];

	pcb = read_mem_1(cpu->program, proc->pc + 1);
	while (check_pcb(pcb, e_lldi) != 0)
	{
		ofs = proc->pc + 2;
		A1 = read_typearoni(cpu, proc, TYFP(0), ofs);
		ofs += size_from_pt(type, e_lldi);
		A2 = read_typearoni(cpu, proc, TYFP(1), ofs);
		ofs += size_from_pt(type, e_lldi);
		if (SHOULD_BREAK)
			break ;
		LLDIPRINT;
		proc->carry = (read_mem_4(cpu->program, proc->pc + A1 + A2) == 0);
		write_reg(proc, arg3, read_mem_4(cpu->program, proc->pc + A1 + A2));
		break ;
	}
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_lldi));
	return (proc->pc + 2 + size_from_pcb(pcb, e_lldi));
}

/*
** 'lfork' is the same as 'fork', but without the (% IDX_MOD).
** Modifies carry.
*/

int			instruction_lfork(struct s_cpu *cpu, struct s_process *proc)
{
	short new_offset;

	new_offset = (short)read_mem_2(cpu->program, proc->pc + 1);
	if (g_verbose & OPT_INSTR)
		OUT("P% 5d | lfork %d (%d)\n", proc->pid, new_offset,
			new_offset + proc->pc);
	fork_process(cpu, proc, proc->pc + new_offset);
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 3);
	return (proc->pc + 3);
}
