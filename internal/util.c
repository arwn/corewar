/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   util.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/30 10:26:24 by callen            #+#    #+#             */
/*   Updated: 2019/08/30 10:26:25 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "asm.h"
#include <stdlib.h>

void
	free_(void *a, size_t b)
{
	t_tok *t;

	if (b == sizeof(t_tok))
	{
		t = (t_tok *)(a);
		if ((t->type == label_def || t->type == bot_name
			|| t->type == bot_comment) && t->u.str)
			free(t->u.str);
	}
	else if (b == sizeof(t_cmd))
	{
	}
	free(a);
}

/*
** Prevent out of bounds accesses for cpu->program
*/

int
	mod_idx(int idx)
{
	int ret;

	ret = idx % MEM_SIZE;
	if (ret < 0)
		ret += MEM_SIZE;
	return (ret);
}

/*
** Read a single byte from buffer PROGRAM at IDX, swapping endianness
*/

uint8_t
	read_mem_1(uint8_t *program, uint32_t idx)
{
	return (program[idx % MEM_SIZE]);
}

/*
** Read two bytes from buffer PROGRAM at IDX, swapping endianness
*/

uint16_t
	read_mem_2(uint8_t *program, uint32_t idx)
{
	return ((uint16_t)program[idx % MEM_SIZE] << 8) |
	((uint16_t)program[(idx + 1) % MEM_SIZE]);
}

/*
** Read four bytes from buffer PROGRAM at IDX, swapping endianness
*/

uint32_t
	read_mem_4(uint8_t *program, uint32_t idx)
{
	return ((uint32_t)program[idx % MEM_SIZE] << 24) |
	((uint32_t)program[(idx + 1) % MEM_SIZE] << 16) |
	((uint32_t)program[(idx + 2) % MEM_SIZE] << 8) |
	((uint32_t)program[(idx + 3) % MEM_SIZE]);
}
