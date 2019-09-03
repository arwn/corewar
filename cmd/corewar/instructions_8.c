/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   instructions_8.c                                   :+:      :+:    :+:   */
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
** Horrendous garbage macros to pass norme, since multiline macros are forbidden
*/

#define DOP(H) (H)==0?arg1&arg2:(H)==1?arg1|arg2:(H)==2?arg1^arg2:0
#define FMTSTR "P% 5d | %s %d %d r%d\n"
#define RETONI(NM) (pr->pc + 2 + size_from_pcb(pcb,e_##NM))
#define H01(NM) t_arg_type pcb;
#define H02(NM) H01(NM) int offset,type,arg1,arg2;
#define H03(NM) H02(NM) uint8_t arg3;
#define H04(NM) H03(NM) pcb = read_mem_1(cpu->program, pr->pc + 1);
#define H05(NM) H04(NM) if (check_pcb(pcb, e_##NM) != 0) {
#define H06(NM) H05(NM) type = type_from_pcb(pcb, 0);
#define H07(NM) H06(NM) offset = pr->pc + 2;
#define H08(NM) H07(NM) arg1 = read_typearoni(cpu, pr, type, offset);
#define H09(NM) H08(NM) offset += size_from_pt(type, e_##NM);
#define H10(NM) H09(NM) type = type_from_pcb(pcb, 1);
#define H11(NM) H10(NM) arg2 = read_typearoni(cpu, pr, type, offset);
#define H12(NM) H11(NM) offset += size_from_pt(type, e_##NM);
#define H13(NM) H12(NM) arg3 = read_mem_1(cpu->program, offset);
#define H14(NM) H13(NM) if (valid_reg(arg3) != 0) {
#define H15(NM) H14(NM) if (g_verbose & OPT_INSTR)
#define H16(NM) H15(NM) OUT(FMTSTR,pr->pid,#NM,arg1,arg2,arg3);
#define H17(NM,OP) H16(NM) pr->carry = ((OP) == 0);
#define H18(NM,OP) H17(NM,OP) write_reg(pr, arg3, (OP));}}
#define H19(NM,OP) H18(NM,OP) if (g_verbose & OPT_PCMOVE)
#define H20(NM,OP) H19(NM,OP) print_adv(cpu,pr,RETONI(NM));
#define H21(NM,OP) H20(NM,OP) return RETONI(NM)

/*
** INSTRUCTION_BITWISE generates an instruction function withe name and operator
*/

#define INSTRUCTION_BITWISE(name, operator) H21(name, operator)

/*
** perform a bitwise and on the first two parameters, storing the result into
** the third which is always a register. Modifies carry.'and r2,%0,r3' stores
** 'r2 & 0' into 'r3'.
*/

int
	instruction_and
	(
		cpu,
		pr
)
	struct s_cpu *cpu;
	struct s_process *pr;
	{
	INSTRUCTION_BITWISE(and, DOP(0));
}

/*
** 'or' is the same as and, except uses bitwise or
*/

int
	instruction_or
	(
	cpu,
			pr
)
	struct s_cpu *cpu;
	struct s_process *pr;
	{
	INSTRUCTION_BITWISE(or, DOP(1));
}

/*
** 'xor' is the same as and, except uses bitwise xor
*/

int
	instruction_xor
	(
cpu,
				pr
)
	struct s_cpu *cpu;
	struct s_process *pr;
	{
	INSTRUCTION_BITWISE(xor, DOP(2));
}

/*
** 'zjmp' always takes an index (IND_SIZE) and makes a jump at this
** index if carry is true, otherwise consuming cycles. 'zjmp %23' stores (PC +
** (23 % IDX_MOD)) into PC.
*/

int
	instruction_zjmp
	(
																			cpu,
																		proc
)
	struct s_cpu *cpu;
	struct s_process *proc;
	{
	short	arg1;
	int		ret;

	arg1 = (short)read_mem_2(cpu->program, proc->pc + 1);
	ret = proc->pc + 3;
	if (proc->carry == 0)
	{
		if (g_verbose & OPT_INSTR)
			OUT("P% 5d | zjmp %d FAILED\n", proc->pid, arg1);
		if (g_verbose & OPT_PCMOVE)
			print_adv(cpu, proc, ret);
	}
	else
	{
		if (g_verbose & OPT_INSTR)
			OUT("P% 5d | zjmp %d OK\n", proc->pid, arg1);
		ret = proc->pc + arg1 % IDX_MOD;
	}
	return (ret);
}
