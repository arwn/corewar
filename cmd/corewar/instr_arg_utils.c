#include "cpu.h"
#include "op.h"
#include "util.h"
#include "instructions.h"

/*
** check if a register number is valid: r1 - r16
*/
int valid_reg(int reg)
{
  return (reg > 0 && reg <= REG_NUMBER);
}

/*
** Get the type of the N'th argument from the pcb
*/
int type_from_pcb(t_arg_type pcb, int arg)
{
	int type;

	type = (pcb >> ((3 - arg) * 2)) & 3;
	if (type == REG_CODE)
		return (T_REG);
	else if (type == DIR_CODE)
		return (T_DIR);
	else if (type == IND_CODE)
		return (T_IND);
	return (0);
}

/*
** returns REG_ARG_SIZE, IND_ARG_SIZE, or DIR_ARG_SIZE
*/
int size_from_pt(int type, int opcode)
{
	if (type == T_REG)
		return (REG_ARG_SIZE);
	else if (type == T_DIR)
		return (g_op_tab[opcode].direct_size ? IND_ARG_SIZE : DIR_ARG_SIZE);
	else if (type == T_IND)
		return (IND_ARG_SIZE);
	return (0);
}

int size_from_pcb(t_arg_type pcb, int opcode)
{
	int ret;
	int ii;

	ii = 0;
	ret = 0;
	while (ii < g_op_tab[opcode].numargs)
	{
		ret += size_from_pt(type_from_pcb(pcb, ii), opcode);
		ii++;
	}
	return (ret);
}

int check_pcb(t_arg_type pcb, int op)
{
	int ii;
	int type;

	ii = 0;
	while (ii < g_op_tab[op].numargs)
	{
		type = type_from_pcb(pcb, ii);
		if (((type & g_op_tab[op].argtypes[ii]) != type) || type == 0)
			return (0);
		ii++;
	}
	return (1);
}
