/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   instructions_10.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 18:14:35 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 18:14:36 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "instructions.h"

#define OUT(...) printf(__VA_ARGS__)

/*
** ldi modifies carry. 'idx' and 'add' are indexes, and 'reg' is a
** register. 'ldi 3,%4,r1' reads IND_SIZE bytes at address: (PC + (3 %
** IDX_MOD)), adding 4 to this sum S. Read REG_SIZE bytes at address (PC + (S %
** IDX_MOD)), which are copied to 'r1'.
*/

#define A1 arg12[0]
#define A2 arg12[1]
#define TYFP(N) (type = type_from_pcb(pcb, (N)))
#define LDIPS1 "P% 5d | ldi %d %d r%d\n"
#define LDIPS2 "       | -> load from %d + %d = %d (with pc and mod %d)\n"
#define LDIPA1 proc->pid, A1, A2, arg3
#define LDIPA2 A1, A2, A1 + A2, (proc->pc + (A1 + A2) % IDX_MOD)
#define LDIPRINT (g_verbose&OPT_INSTR)?OUT(LDIPS1 LDIPS2,LDIPA1,LDIPA2):0

int
	instruction_ldi(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	uint8_t		arg3;
	int			offset;
	int			type;
	int			arg12[2];

	pcb = read_mem_1(cpu->program, proc->pc + 1);
	while (check_pcb(pcb, e_ldi) != 0)
	{
		offset = proc->pc + 2;
		A1 = read_typearoni(cpu, proc, TYFP(0), offset);
		offset += size_from_pt(type, e_ldi);
		A2 = read_typearoni(cpu, proc, TYFP(1), offset);
		offset += size_from_pt(type, e_ldi);
		if (valid_reg((arg3 = read_mem_1(cpu->program, offset))) == 0)
			break ;
		LDIPRINT;
		write_reg(proc, arg3,
			read_mem_4(cpu->program, proc->pc + (A1 + A2) % IDX_MOD));
		break ;
	}
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_ldi));
	return (proc->pc + 2 + size_from_pcb(pcb, e_ldi));
}

/*
** 'sti' stores at an index offset. 'sti r2,%4,%5' copies REG_SIZE bytes
** of 'r2' at address (4 + 5) Parameters 2 and 3 are treated as indexes.
*/

#undef A2
#define A2 arg23[0]
#define A3 arg23[1]
#define STIPS1 "P% 5d | sti r%d %d %d\n"
#define STIPS2 "       | -> store to %d + %d = %d (with pc and mod %d)\n"
#define STIPA1 proc->pid, arg1, A2, A3
#define STIPA2 A2, A3, (A2 + A3), proc->pc + (A2 + A3) % IDX_MOD
#define STIPRINT (g_verbose&OPT_INSTR)?OUT(STIPS1 STIPS2,STIPA1,STIPA2):0

int	instruction_sti(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	int			type;
	int			ofs;
	int			arg1;
	int			arg23[2];

	while (check_pcb(pcb = read_mem_1(cpu->program, proc->pc + 1), e_sti) != 0)
	{
		if (valid_reg((arg1 = read_mem_1(cpu->program, proc->pc + 2))) == 0)
			break ;
		ofs = proc->pc + 3;
		A2 = read_typearoni(cpu, proc, TYFP(1), ofs);
		ofs += size_from_pt(type, e_sti);
		A3 = read_typearoni(cpu, proc, TYFP(2), ofs);
		if (proc->reg_err)
			break ;
		STIPRINT;
		write_mem_ins(proc, cpu->program,
			mod_idx(proc->pc + (A2 + A3) % IDX_MOD), read_reg(proc, arg1));
		break ;
	}
	proc->reg_err = 0;
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 2 + size_from_pcb(pcb, e_sti));
	return (proc->pc + 2 + size_from_pcb(pcb, e_sti));
}
