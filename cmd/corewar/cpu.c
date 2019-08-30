/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cpu.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 19:31:51 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 19:31:52 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cpu.h"
#include "instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT(...) printf(__VA_ARGS__)

static int
	step(struct s_cpu *cpu)
{
	int ret;
	int ii;

	cpu->clock += 1;
	if (g_verbose & OPT_CYCLES)
		OUT("It is now cycle %d\n", cpu->clock);
	ret = run_processes(cpu);
	if (cpu->cycle_to_die <= cpu->clock - cpu->prev_check)
		check_alive(cpu);
	if (g_color || g_gui)
	{
		ii = 0;
		while (ii < MEM_SIZE)
		{
			if (g_mem_colors[ii].writes != 0)
				g_mem_colors[ii].writes -= 1;
			ii++;
		}
	}
	return (ret);
}

/*
** spawn_process makes a new process. sometimes spawn_process allocates more
** space for a few new processes.
*/

static void
	spawn_process(struct s_cpu *cpu, int pc, int player)
{
	struct s_process *done;

	done = malloc(sizeof(*done));
	if (done == NULL)
	{
		perror("Fatal error");
		exit(-1);
	}
	bzero(done, sizeof(*done));
	done->carry = 0;
	pc = mod_idx(pc);
	if (player >= -4 && player <= -1)
		done->player = player;
	done->pc = pc;
	done->pid = cpu->active + 1;
	*done->registers = player;
	if (cpu->processes == 0)
		initial_process(done, cpu);
	else
		prepend_process(done, cpu);
	cpu->active += 1;
	cpu->pid_next += 1;
}

/*
** delete_process deletes the current process and sets the current process to
** the previous process.
*/

static void
	delete_process(struct s_cpu *cpu, struct s_process **proc)
{
	struct s_process *expired;

	expired = *proc;
	if (g_verbose & OPT_DEATHS)
		OUT("Process %d hasn\'t lived for %d cycles (CTD %d)\n",
		expired->pid, cpu->clock - expired->last_live, cpu->cycle_to_die);
	if (expired->prev)
		expired->prev->next = expired->next;
	else
		cpu->processes = expired->next;
	if (expired->next)
		expired->next->prev = expired->prev;
	free(expired);
	cpu->active -= 1;
	if (cpu->active < 1)
		cpu->processes = 0;
}

/*
** load loads a PROGRAM of length LENGTH into memory address ADDRESS.
*/

static void
	load(struct s_cpu *cpu, char *program, uint32_t length, uint32_t address)
{
	uint32_t ii;

	ii = 0;
	while (ii < length)
	{
		cpu->program[ii + address] = (uint8_t)program[ii];
		ii++;
	}
}

/*
** new_cpu makes a new cpu.
*/

struct s_cpu
	new_cpu(void)
{
	static struct s_cpu done;

	done.winner = -1;
	done.cycle_to_die = CYCLE_TO_DIE;
	done.spawn_process = spawn_process;
	done.kill_process = delete_process;
	done.step = step;
	done.load = load;
	return (done);
}
