/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cpu_triple.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 19:32:07 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 19:32:07 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cpu.h"
#include "instructions.h"

/*
** initial_process links the initial process in a program to itself.
*/

void
	initial_process(struct s_process *done, struct s_cpu *cpu)
{
	done->next = NULL;
	done->prev = NULL;
	cpu->processes = done;
}

/*
** prepend_process prepends a process to the beginning of the process list to be
** evaluated next cycle. we prepend because the newest process should go after
** the oldest and before the newest.
*/

void
	prepend_process(struct s_process *done, struct s_cpu *cpu)
{
	done->next = cpu->processes;
	done->prev = NULL;
	cpu->processes->prev = done;
	cpu->processes = done;
}

/*
** valid_header_p validates a header
*/

bool
	valid_header_p(header_t header)
{
	if (header.magic != COREWAR_EXEC_MAGIC
		&& header.magic != COREWAR_EXTENDED_EXEC_MAGIC)
		return (false);
	if (header.prog_size > CHAMP_MAX_SIZE)
		return (false);
	return (true);
}
