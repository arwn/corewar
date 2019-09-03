/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   instr_op_args.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/26 18:02:03 by callen            #+#    #+#             */
/*   Updated: 2019/08/27 16:32:31 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "instructions.h"

const char	g_verboden_reg[NUM_OPS][MAX_ARGS_NUMBER] = {
	[e_ld] = {1, 0},
	[e_st] = {0, 1, 0},
	[e_add] = {0, 0, 1, 0},
	[e_sub] = {0, 0, 1, 0},
	[e_and] = {0, 0, 1, 0},
	[e_or] = {0, 0, 1, 0},
	[e_xor] = {0, 0, 1, 0},
	[e_ldi] = {0, 0, 1, 0},
	[e_sti] = {1, 0},
	[e_lld] = {0, 1, 0},
	[e_lldi] = {0, 0, 1, 0},
};

/*
** h
*/

int		read_op_args(struct s_cpu *cpu, struct s_process *proc)
{
	int			ii;
	int			newpc;
	int			type;
	t_arg_type	pcb;

	ii = 0;
	newpc = proc->pc + 1;
	pcb = read_mem_1(cpu->program, newpc);
	if (check_pcb(pcb, proc->opcode))
		return (-1);
	newpc += ENC_SIZE;
	while (ii < g_op_tab[proc->opcode].numargs)
	{
		type = type_from_pcb(pcb, ii);
		if (g_verboden_reg[proc->opcode][ii])
			proc->args[ii] = read_arg_pls(cpu, proc, type, newpc);
		else
			proc->args[ii] = read_typearoni(cpu, proc, type, newpc);
		newpc += size_from_pt(type, proc->opcode);
		ii++;
	}
	return (newpc);
}

/*
** prints the movement of the program counter for PROC
*/

void	print_adv(struct s_cpu *cpu, struct s_process *proc, int new)
{
	int ii;
	int len;

	ii = 0;
	len = new - proc->pc;
	printf("ADV %d (0x%04x -> 0x%04x) ", len, proc->pc, new);
	while (ii < len)
	{
		if (ii == 0)
			printf("%02x", read_mem_1(cpu->program, proc->pc + ii));
		else
			printf(" %02x", read_mem_1(cpu->program, proc->pc + ii));
		ii++;
	}
	printf("\n");
}

/*
** next_instruction updates the execution time for PROC.
*/

void	next_cpu_op(struct s_cpu *cpu, struct s_process *proc)
{
	uint8_t op;

	op = (char)read_mem_1(cpu->program, proc->pc);
	if (op < 1 || NUM_OPS < op)
		proc->pc = mod_idx(proc->pc + 1);
	else
	{
		proc->opcode = op;
		proc->instruction_time = g_op_tab[op].cycles_to_exec;
	}
}

#define M00(I,P) do{if(!g_mem_colors[(I)].writes||g_mem_colors[(I)].player!=(P))
#define M01(I,P) M00(I,P)g_mem_colors[(I)].writes=49;if(!g_mem_colors[(I)].
#define M02(I,P) M01(I,P)player||g_mem_colors[(I)].player!=(P))
#define M03(I,P) M02(I,P)g_mem_colors[(I)].player=(P);}while(0)
#define MEM_COLOR(IDX, PL) M03(IDX, PL)

/*
** Write four bytes of VAL into core memory MEM at offset IDX
*/

void	write_mem_ins(struct s_process *proc, uint8_t *mem, int idx, int val)
{
	const int idx1 = (idx);
	const int idx2 = (idx + 1) % MEM_SIZE;
	const int idx3 = (idx + 2) % MEM_SIZE;
	const int idx4 = (idx + 3) % MEM_SIZE;

	if (g_color || g_gui)
	{
		MEM_COLOR(idx1, proc->player);
		MEM_COLOR(idx2, proc->player);
		MEM_COLOR(idx3, proc->player);
		MEM_COLOR(idx4, proc->player);
	}
	mem[idx1] = (val >> 24) & 0xff;
	mem[idx2] = (val >> 16) & 0xff;
	mem[idx3] = (val >> 8) & 0xff;
	mem[idx4] = val & 0xff;
}
