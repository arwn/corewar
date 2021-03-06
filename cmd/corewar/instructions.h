/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   instructions.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <callen@student.42.us.org>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/18 04:09:40 by callen            #+#    #+#             */
/*   Updated: 2019/08/18 04:09:41 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INSTRUCTIONS_H
# define INSTRUCTIONS_H

# include "cpu.h"
# include "op.h"
# include "util.h"

int		instruction_live(struct s_cpu *cpu, struct s_process *proc);
int		instruction_ld(struct s_cpu *cpu, struct s_process *proc);
int		instruction_st(struct s_cpu *cpu, struct s_process *proc);
int		instruction_add(struct s_cpu *cpu, struct s_process *proc);
int		instruction_sub(struct s_cpu *cpu, struct s_process *proc);

int		instruction_and(struct s_cpu *cpu, struct s_process *proc);
int		instruction_or(struct s_cpu *cpu, struct s_process *proc);
int		instruction_xor(struct s_cpu *cpu, struct s_process *proc);
int		instruction_zjmp(struct s_cpu *cpu, struct s_process *proc);
int		instruction_ldi(struct s_cpu *cpu, struct s_process *proc);

int		instruction_sti(struct s_cpu *cpu, struct s_process *proc);
int		instruction_fork(struct s_cpu *cpu, struct s_process *proc);
int		instruction_lld(struct s_cpu *cpu, struct s_process *proc);
int		instruction_lldi(struct s_cpu *cpu, struct s_process *proc);
int		instruction_lfork(struct s_cpu *cpu, struct s_process *proc);

int		instruction_aff(struct s_cpu *cpu, struct s_process *proc);
int		instruction_nop(struct s_cpu *cpu, struct s_process *proc);
int		instruction_kill(struct s_cpu *cpu, struct s_process *proc);

int		valid_reg(int reg);
int		type_from_pcb(t_arg_type pcb, int arg);
int		size_from_pt(int type, int opcode);
int		size_from_pcb(t_arg_type pcb, int opcode);
int		check_pcb(t_arg_type pcb, int opcode);

void	write_reg(struct s_process *p, int r, int v);
int		read_reg(struct s_process *p, int r);
int		read_indirect(struct s_cpu *c, struct s_process *p, short o);
int		read_typearoni(struct s_cpu *c, struct s_process *p, int t, int o);
int		read_arg_pls(struct s_cpu *c, struct s_process *p, int t, int o);

int		read_op_args(struct s_cpu *cpu, struct s_process *proc);
void	print_adv(struct s_cpu *cpu, struct s_process *proc, int new);
void	next_cpu_op(struct s_cpu *cpu, struct s_process *proc);
void	write_mem_ins(struct s_process *proc, uint8_t *mem, int idx, int val);

typedef int	(*t_inst)(struct s_cpu *, struct s_process *);

static

const

t_inst

g_inst_tab[NUM_OPS + 1] = {
	(t_inst)0,
	[e_live] = instruction_live,
	[e_ld] = instruction_ld,
	[e_st] = instruction_st,
	[e_add] = instruction_add,
	[e_sub] = instruction_sub,
	[e_and] = instruction_and,
	[e_or] = instruction_or,
	[e_xor] = instruction_xor,
	[e_zjmp] = instruction_zjmp,
	[e_ldi] = instruction_ldi,
	[e_sti] = instruction_sti,
	[e_fork] = instruction_fork,
	[e_lld] = instruction_lld,
	[e_lldi] = instruction_lldi,
	[e_lfork] = instruction_lfork,
	[e_aff] = instruction_aff,
	[e_nop] = instruction_nop,
	[e_kill] = instruction_kill,
};

#endif
