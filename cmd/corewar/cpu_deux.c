/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cpu_deux.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 19:31:59 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 19:32:00 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cpu.h"
#include "instructions.h"
#include <stdio.h>

#define OUT(...) printf(__VA_ARGS__)

/*
** execute op or decrement instruction_time
*/

int
	run_op(struct s_cpu *cpu, struct s_process *proc)
{
	int new_pc;

	proc->instruction_time -= 1;
	if (proc->instruction_time == 0)
	{
		new_pc = g_inst_tab[proc->opcode](cpu, proc);
		if (new_pc >= MEM_SIZE)
			new_pc %= MEM_SIZE;
		g_instruction_calls[proc->opcode] += 1;
		if (new_pc < 0)
			new_pc += MEM_SIZE;
		proc->pc = new_pc;
		proc->opcode = 0;
		return (1);
	}
	return (0);
}

int
	remove_proc(struct s_cpu *cpu, struct s_process **proc, int cond)
{
	if (!cond)
		return (0);
	if ((*proc)->next)
	{
		*proc = (*proc)->next;
		cpu->kill_process(cpu, &(*proc)->prev);
	}
	else
	{
		cpu->kill_process(cpu, proc);
		return (1);
	}
	return (0);
}

/*
** run processes, bada-bing bada-boom
*/

int
	run_processes(struct s_cpu *cpu)
{
	struct s_process	*proc;
	int					ret;

	ret = 0;
	proc = cpu->processes;
	while (proc != 0)
	{
		if (proc->opcode == 0)
			next_cpu_op(cpu, proc);
		if (proc->opcode != 0)
			ret = run_op(cpu, proc);
		if (remove_proc(cpu, &proc, (proc->kill
			|| cpu->players[~proc->player].kill)))
			break ;
		else
			proc = proc->next;
	}
	return (ret);
}

int
	assbutt
	(
				cpu,
				proc
)
	struct s_cpu *cpu;
	struct s_process **proc;
	{
	if ((cpu->clock - (*proc)->last_live) < cpu->cycle_to_die)
	{
		if (!(*proc) || !(*proc)->next)
			return (1);
		(*proc) = (*proc)->next;
	}
	else
	{
		if ((*proc)->next)
		{
			(*proc) = (*proc)->next;
			cpu->kill_process(cpu, &(*proc)->prev);
		}
		else
		{
			cpu->kill_process(cpu, proc);
			return (1);
		}
	}
	return (0);
}

/*
** Check for living processes
*/

void
	check_alive(struct s_cpu *cpu)
{
	struct s_process *proc;

	proc = cpu->processes;
	while (proc != NULL && cpu->processes != NULL)
		if (assbutt(cpu, &proc))
			break ;
	cpu->prev_check = cpu->clock;
	cpu->num_checks += 1;
	if (cpu->nbr_lives >= NBR_LIVE || cpu->num_checks == MAX_CHECKS)
	{
		cpu->cycle_to_die -= CYCLE_DELTA;
		cpu->num_checks = 0;
		if (g_verbose & OPT_CYCLES)
			OUT("Cycle to die is now %d\n", cpu->cycle_to_die);
	}
	cpu->nbr_lives = 0;
}
