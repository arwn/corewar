#include "asm.h"

#define SPC (1)
#define SEP (2)

static t_list *handle_label(t_list *tok, t_arg *arg)
{
	tok = tok->next;
	if (T(tok)->type != label_def && T(tok)->type != instruction)
	{
		TOK_TO_ERR(T(tok));
		return (tok);
	}
	arg->type |= T_LAB;
	if (T(tok)->type == instruction)
		arg->str = g_op_tab[T(tok)->opcode - 1].name;
	else
		arg->str = T(tok)->str;
	return (tok->next);
}

static t_list *handle_num(t_list *tok, t_arg *arg)
{
	arg->num = T(tok)->num;
	return (tok->next);
}

/*
** Given some list of tokens, finds the next argument, stores it in arg, and
** returns token after found arg
*/

t_list		*getarg(t_list *tok, t_arg *arg, char *sep)
{
	(void)tok;
	(void)arg;

	switch (T(tok)->type)
	{
	case space:
		*sep |= SPC;
		return (tok->next);
	case registry:
		tok = tok->next;
		if (T(tok)->type != num)
			return (NULL);
		arg->type = T_REG;
		arg->num = T(tok)->num;
		return (tok->next);
		break ;
	case num:
		arg->type = T_IND;
		return (handle_num(tok, arg));
		break ;
	case direct_char:
		arg->type |= T_DIR;
		tok = tok->next;
		switch (T(tok)->type)
		{
		case num:
			return (handle_num(tok, arg));
			break ;
		case label_char:
			return (handle_label(tok, arg));
			break ;
		default:
			TOK_TO_ERR(T(tok));
			return (tok);
		}
		break ;
	case label_char:
		arg->type = T_IND;
		return (handle_label(tok, arg));
		break ;
	case separator:
		if (*sep & SEP)
		{
			TOK_TO_ERR(T(tok));
			return (tok);
		}
		*sep |= SEP;
		break ;
	default:
		TOK_TO_ERR(T(tok));
		return (tok);
	}
	// valid tokens
	if (T(tok)->type >= label_def || T(tok)->type == space || T(tok)->type == separator)
		return (tok->next);
	else if (T(tok)->type == newline)
		return (tok);
	return (NULL);
}

/*
** r-?\d+
** %4
** %:label
** -?\d+
** :label
*/

/*
** Attempts to rad one line of tokens, storing opcode and args in cmd
*/

t_list		*getargs(t_list *tok, t_cmd *cmd)
{
	t_list		*tmp;
	t_arg		arg;
	unsigned	ii;
	char		sep;
	unsigned	numargs = g_op_tab[cmd->opcode-1].numargs;

	tmp = tok;
	sep = 0;
	ii = 0;
	arg.type = 0;
	arg.num = 0;
	if (T(tok)->type == err)
		return (tok);
	while (tok && T(tok)->type != err && ((cmd->cols[ii] = T(tok)->col) || 1) && T(tok)->type != newline && (tmp = getarg(tok, &arg, &sep)))
	{
		if (!tmp)
			return (tmp);
		else if (T(tmp)->type == err)
			return (tmp);
		else if (!arg.type)
		{
			tok = tmp;
			continue ;
		}
		cmd->cols[ii] = T(tok)->col;
		if (!sep || ii >= numargs)
		{
			TOK_TO_ERR(T(tok));
			return (tok);
		}
		tok = tmp;
		cmd->argtypes[ii] = arg.type;
		if (arg.type & T_LAB)
			cmd->args[ii].str = arg.str;
		else
			cmd->args[ii].num = arg.num;
		cmd->num_bytes += CMD_NUM_BYTES(arg.type & ~T_LAB, cmd->opcode);
		arg.type = 0;
		arg.num = 0;
		++ii;
		sep = 0;
	}
	if (ii != numargs) {
		TOK_TO_ERR(T(tok));
		return (tok);
	}
	cmd->num_bytes += 1 + (g_op_tab[cmd->opcode - 1].param_encode ? ENC_SIZE : 0);
	return (tok);
}

/*
** handle each arg type individually
** dispatch to individual func depending on arg type
** return arg
** check all arg types against g_arg_tab thing
*/

t_list		*getcmd(t_list *tok, t_cmd *cmd, unsigned *position, t_dict *dict, unsigned *linenum, header_t *header)
{
	unsigned	argnum;
	int			ii;

	(void)g_errarr;
	(void)cmd;

	ft_bzero(g_errarr, sizeof(g_errarr));
	argnum = 0;

	// skip whitespace
	while (tok && (T(tok)->type == space))
		tok = tok->next;
	if (!tok)
		return (tok);

	switch (T(tok)->type)
	{
	case newline:
		*linenum = *linenum + 1;
		break ;
	case space:
		break ;
	case bot_name:
		ft_strncpy(header->prog_name, T(tok)->str, PROG_NAME_LENGTH);
		tok = tok->next;
		while (tok && (T(tok)->type == space))
			tok = tok->next;
		if (tok && T(tok)->type != newline)
		{
			ft_strcat(g_errarr, ERR_FIRST_BIT"Unexpected symbol [");
			ft_strcat(g_errarr, g_tok_to_str_safe[T(tok)->type]);
			ft_strcat(g_errarr, "]");
			TOK_TO_ERR(T(tok));
			return (tok);
		}
		*linenum = *linenum + 1;
		break ;
	case bot_comment:
		ft_strncpy(header->comment, T(tok)->str, COMMENT_LENGTH);
		tok = tok->next;
		while (tok && (T(tok)->type == space))
			tok = tok->next;
		if (tok && T(tok)->type != newline)
		{
			ft_strcat(g_errarr, ERR_FIRST_BIT"Unexpected symbol [");
			ft_strcat(g_errarr, g_tok_to_str_safe[T(tok)->type]);
			ft_strcat(g_errarr, "]");
			TOK_TO_ERR(T(tok));
			return (tok);
		}
		*linenum = *linenum + 1;
		break ;
	case label_def:
		ii = dictInsert(dict, T(tok)->str, *position);
		if (ii < 1)
		{
			ft_strcat(g_errarr, ERR_FIRST_BIT"Redefinition of label \"");
			ft_strcat(g_errarr, T(tok)->str);
			ft_strcat(g_errarr, "\"");
			TOK_TO_ERR(T(tok));
			return (tok);
		}
		tok = tok->next;
		if (tok && T(tok)->type == label_char)
			return (tok->next);
		ft_strcat(g_errarr, ERR_FIRST_BIT"Unexpected symbol [");
		ft_strcat(g_errarr, g_tok_to_str_safe[T(tok)->type]);
		ft_strcat(g_errarr, "]");
		TOK_TO_ERR(T(tok));
		return (tok);
		break ;
	case instruction:
		cmd->opcode = T(tok)->opcode;
		tok = tok->next;
		tok = getargs(tok, cmd);
		if (tok && T(tok)->type != newline)
		{
			ft_strcat(g_errarr, ERR_FIRST_BIT"Unexpected symbol [");
			ft_strcat(g_errarr, g_tok_to_str_safe[T(tok)->type == err ? T(tok)->_type : T(tok)->type]);
			ft_strcat(g_errarr, "]");
			TOK_TO_ERR(T(tok));
			return (tok);
		}
		*linenum = *linenum + 1;
		return (tok->next);
	default:
		if (T(tok)->type == label_def || T(tok)->type == bot_name || T(tok)->type == bot_comment)
		{
			free(T(tok)->str);
			T(tok)->str = NULL;
		}
		TOK_TO_ERR(T(tok));
		ft_strcat(g_errarr, ERR_UNEXPECTED_SYMBOL);
		return (tok);
	}
	tok = tok->next;
	return (tok);

	// skip whitespace
	while (tok && (T(tok)->type == space || T(tok)->type == newline))
		tok = tok->next;

	return (tok);
}

static int			chkcmd(t_cmd *cmd)
{
	t_op	*op = &g_op_tab[cmd->opcode - 1];

	for (int ii = 0; ii < op->numargs; ++ii) {
		if (!(op->argtypes[ii] & (cmd->argtypes[ii] & ~T_LAB)))
		{
			ft_strcat(g_errarr, ERR_UNEXPECTED_ARG);
			return (ii + 1);
		}
	}
	return (0);
}

/*
** take list of tokens, header, dict of labels, returns list of cmds
*/

t_list		*parse(t_list *tokens, size_t bufsize,
				   header_t *header, t_dict *dict)
{
	(void)tokens;
	(void)header;
	enum e_toktype	type;
	t_list			*tok;
	unsigned		linenum;
	t_list			*cmds;
	unsigned		position;
	t_list			*tmp;

	cmds = NULL;
	cmds = addtolst(cmds, NULL, 0);

	position = 0;
	type = space;
	linenum = 1;
	tmp = tokens;
	tok = tmp;

	t_cmd		cmd;

	header->magic = COREWAR_EXEC_MAGIC;
	while (tok && T(tok)->type != eof)
	{
		cmd = (t_cmd){-1,0,linenum,0,{0},{0,0,0,0},{{.str=0},{.str=0},{.str=0},{.str=0}}};
		tmp = tok;
		tok = getcmd(tok, &cmd, &position, dict, &linenum, header);
		if (tok && T(tok)->type == err)
		{
			asprintf(&g_errstr, g_errarr, linenum, T(tok)->col, g_tok_to_str[T(tok)->_type]);
			ft_lstdel(&cmds, free_);
			return (NULL);
		}
		if (cmd.opcode > -1)
		{
			int idx;
			if ((idx = chkcmd(&cmd)))
			{
				asprintf(&g_errstr, g_errarr, cmd.linenum, cmd.cols[idx-1], g_op_tab[cmd.opcode-1].name, g_arg_to_str[cmd.argtypes[idx-1] & ~T_LAB]);
				ft_lstdel(&cmds, free_);
				return (NULL);
			}
			{
				cmd.encoding = 0;
				for (int ii = 1; ii <= g_op_tab[cmd.opcode-1].numargs; ++ii) {
					cmd.encoding |= (g_cmd_encoding[cmd.argtypes[ii-1] & ~T_LAB]) << ((ENC_SIZE * 8) - (ii * 2));
				}
			}
			if (position + cmd.num_bytes > bufsize) {
				ft_dprintf(STDERR_FILENO, WARNING_PROG_TOO_BIG"\n", cmd.linenum);
				break ;
			}
			position += cmd.num_bytes;
			cmds = addtolst(cmds, &cmd, sizeof(cmd));
		}
	}
	header->prog_size = position;
	return (cmds);
}
