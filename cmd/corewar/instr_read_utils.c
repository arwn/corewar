#include "cpu.h"
#include "op.h"
#include "util.h"
#include "instructions.h"

/*
** Write VAL into register REG for the process PROC
*/
void write_reg(struct s_process *proc, int reg, int val)
{
	if (valid_reg(reg))
		proc->registers[reg - 1] = val;
}

/*
** Read register REG for the process PROC
*/
int read_reg(struct s_process *proc, int reg)
{
	int ret;

	ret = 0;
	if (valid_reg(reg))
		ret = proc->registers[reg - 1];
	return (ret);
}

/*
** Modify the carry flag for the current process based on VAL
*/
void mod_carry(struct s_process *proc, int val) {
	proc->carry = val;
}

/*
** reads an indirect value from core memory
*/
int read_indirect(struct s_cpu *cpu, struct s_process *proc, short offset)
{
	int idx;
	int ofs;
	int ret;

	ofs = offset % IDX_MOD;
	idx = proc->pc + ofs;
	ret = (int)read_mem_4(cpu->program, idx);
	return ret;
}

/*
** readaroni that typearoni
*/
int read_typearoni(struct s_cpu *cpu, struct s_process *proc, int type, int ofs)
{
	int ret;

	ret = 0;
	if (type == T_REG)
	{
		ret = read_mem_1(cpu->program, ofs);
		if (valid_reg(ret) != 0)
			ret = read_reg(proc, ret);
	}
	else if (type == T_DIR)
	{
		if (g_op_tab[proc->opcode].direct_size)
			ret = (short)read_mem_2(cpu->program, ofs);
		else
			ret = read_mem_4(cpu->program, ofs);
	}
	else if (type == T_IND)
		ret = read_indirect(cpu, proc, read_mem_2(cpu->program, ofs));
	return (ret);
}

/*
** giv noombir plos
*/
int read_arg_pls(struct s_cpu *cpu, struct s_process *proc, int type, int ofs)
{
	int ret;

	ret = 0;
	if (type == T_REG)
		ret = read_mem_1(cpu->program, ofs);
	else if (type == T_DIR)
	{
		if (g_op_tab[proc->opcode].direct_size)
			ret = (short)read_mem_2(cpu->program, ofs);
		else
			ret = read_mem_4(cpu->program, ofs);
	}
	else if (type == T_IND)
		ret = read_mem_2(cpu->program, ofs);
	return (ret);
}
