/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   instructions_5.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 12:31:10 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 12:31:11 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "instructions.h"

#define OUT(...) printf(__VA_ARGS__)

/*
** Stores the current cycle into the lastlive array in CPU, if the NAME
** corresponds to an active player
*/

int	instruction_live(struct s_cpu *cpu, struct s_process *proc)
{
	int arg1;
	int player;

	arg1 = read_mem_4(cpu->program, proc->pc + 1);
	if (g_verbose & OPT_LIVES)
		OUT("P% 5d | live %d\n", proc->pid, arg1);
	proc->last_live = cpu->clock;
	cpu->nbr_lives += 1;
	player = ~arg1;
	if (player >= 0 && player < MAX_PLAYERS && cpu->players[player].name)
	{
		if ((g_verbose & OPT_LIVES))
			OUT("Player %d (%s) is said to be alive\n", player + 1,
				cpu->players[player].name);
		cpu->players[player].last_live = cpu->clock;
		cpu->winner = player;
	}
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + 5);
	return (proc->pc + 5);
}

/*
** 'ld' takes 2 parameters, 2nd must be a register that isn't the
** 'program counter'. It loads the value of the first parameter in the register,
** and modifies the 'carry'. 'ld 34,r3' loads the REG_SIZE bytes from address
** (PC + (34 % IDX_MOD)) in register r3.
*/

int	instruction_ld(struct s_cpu *cpu, struct s_process *pr)
{
	t_arg_type	pcb;
	int			arg1;
	int			arg2;
	int			type;
	int			ret;

	if (check_pcb((pcb = read_mem_1(cpu->program, pr->pc + 1)), e_ld) != 0)
	{
		if ((type = type_from_pcb(pcb, 0)) == T_DIR)
			arg1 = read_mem_4(cpu->program, pr->pc + 2);
		else
			arg1 = read_indirect(cpu, pr, read_mem_2(cpu->program, pr->pc + 2));
		if (valid_reg((arg2 = read_mem_1(cpu->program,
			pr->pc + 2 + size_from_pt(type, e_ld)))) != 0)
		{
			if (g_verbose & OPT_INSTR)
				OUT("P% 5d | ld %d r%d\n", pr->pid, arg1, arg2);
			pr->carry = arg1 == 0;
			write_reg(pr, arg2, arg1);
		}
	}
	ret = pr->pc + 2 + size_from_pcb(pcb, e_ld);
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, pr, ret);
	return (ret);
}

/*
** 'st' takes 2 parameters, storing (REG_SIZE bytes) of the value of
** the first argument (always a register) in the second. 'st r4,34' stores the
** value of 'r4' at the address (PC + (34 % IDX_MOD)) 'st r3,r8' copies the
** contents of 'r3' to 'r8'
*/

int	instruction_st(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	int			type;
	int			arg2;
	int			arg1;

	if (check_pcb((pcb = read_mem_1(cpu->program, proc->pc + 1)), e_st) != 0)
	{
		if ((type = type_from_pcb(pcb, 1)) == T_IND)
			arg2 = (short)read_mem_2(cpu->program, proc->pc + 3);
		else
			arg2 = read_mem_1(cpu->program, proc->pc + 3);
		if (valid_reg((arg1 = read_mem_1(cpu->program, proc->pc + 2))) != 0)
		{
			if (g_verbose & OPT_INSTR)
				OUT("P% 5d | st r%d %d\n", proc->pid, arg1, arg2);
			if ((type == T_REG) && valid_reg(arg2) != 0)
				write_reg(proc, arg2, read_reg(proc, arg1));
			else if (type == T_IND)
				write_mem_ins(proc, cpu->program,
					mod_idx(proc->pc + arg2 % IDX_MOD), read_reg(proc, arg1));
		}
	}
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + size_from_pcb(pcb, e_st) + 2);
	return (proc->pc + size_from_pcb(pcb, e_st) + 2);
}

/*
** 'add' takes 3 registers as parameters, adding the contents of the
** first and second, storing the result into the third. Modifies carry. 'add
** r2,r3,r5' adds the values of 'r2' and 'r3' and stores the result in 'r5'.
*/

int	instruction_add(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	int			done;
	int			arg1;
	int			arg2;
	int			arg3;

	pcb = read_mem_1(cpu->program, proc->pc + 1);
	if (check_pcb(pcb, e_add) != 0)
	{
		arg1 = read_mem_1(cpu->program, proc->pc + 2);
		arg2 = read_mem_1(cpu->program, proc->pc + 3);
		arg3 = read_mem_1(cpu->program, proc->pc + 4);
		if (valid_reg(arg1) && valid_reg(arg2) && valid_reg(arg3))
		{
			if (g_verbose & OPT_INSTR)
				OUT("P% 5d | add r%d r%d r%d\n", proc->pid, arg1, arg2, arg3);
			done = read_reg(proc, arg1) + read_reg(proc, arg2);
			proc->carry = done == 0;
			write_reg(proc, arg3, done);
		}
	}
	done = size_from_pcb(pcb, e_add);
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + done + 2);
	return (proc->pc + done + 2);
}

/*
** 'sub' is the same as instruction_add, except performs subtraction.
*/

int	instruction_sub(struct s_cpu *cpu, struct s_process *proc)
{
	t_arg_type	pcb;
	int			done;
	int			reg1;
	int			reg2;
	int			reg3;

	pcb = read_mem_1(cpu->program, proc->pc + 1);
	if (check_pcb(pcb, e_sub) != 0)
	{
		reg1 = read_mem_1(cpu->program, proc->pc + 2);
		reg2 = read_mem_1(cpu->program, proc->pc + 3);
		reg3 = read_mem_1(cpu->program, proc->pc + 4);
		if (valid_reg(reg1) && valid_reg(reg2) && valid_reg(reg3))
		{
			if (g_verbose & OPT_INSTR)
				OUT("P% 5d | sub r%d r%d r%d\n", proc->pid, reg1, reg2, reg3);
			done = read_reg(proc, reg1) - read_reg(proc, reg2);
			proc->carry = done == 0;
			write_reg(proc, reg3, done);
		}
	}
	done = size_from_pcb(pcb, e_sub);
	if (g_verbose & OPT_PCMOVE)
		print_adv(cpu, proc, proc->pc + done + 2);
	return (proc->pc + done + 2);
}
