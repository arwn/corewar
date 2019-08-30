/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   instructions_17.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 12:12:32 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 12:12:34 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "instructions.h"

#define OUT(...) printf(__VA_ARGS__)

/*
** 'aff' takes a register and writes the stored value modulo 256 to
** stdout. 'ld %52,r3  aff r3' displays '*' on stdout.
*/

int	instruction_aff(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	uint8_t		arg1;

	pcb = read_mem_1(cpu->program, proc->pc + 1);
	if (check_pcb(pcb, e_aff) != 0)
	{
		arg1 = read_mem_1(cpu->program, proc->pc + 2);
		if (valid_reg(arg1) != 0)
		{
			arg1 = read_reg(proc, arg1);
			if (g_enable_aff != 0)
				OUT("Aff: %c\n", arg1);
		}
	}
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_aff));
	return (proc->pc + 2 + size_from_pcb(pcb, e_aff));
}

/*
** 'nop' is a single cycle explicit no operation
*/

int	instruction_nop(struct s_cpu *cpu, struct s_process *proc)
{
	int ret;

	ret = proc->pc + 1;
	if (g_verbose & OPT_INSTR)
		OUT("P% 5d | nop\n", proc->pid);
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, ret);
	return (ret);
}

/*
** 'kill' sets the calling process' last_live to 0, reads DIR_SIZE bytes
*/

int	instruction_kill(struct s_cpu *cpu, struct s_process *proc)
{
	int tokill;
	int player;

	proc->kill = true;
	tokill = read_mem_4(cpu->program, proc->pc + 1);
	player = ~tokill;
	if (player >= 0 && player < MAX_PLAYERS)
		cpu->players[player].kill = true;
	if (g_verbose & OPT_INSTR)
		OUT("P% 5d | kill %d\n", proc->pid, tokill);
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 5);
	return (proc->pc + 5);
}
