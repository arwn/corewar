/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   asm.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/11 14:08:29 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/10 11:09:38 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ASM_H
# define ASM_H

# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include "op.h"
# include "libft.h"
# include "hashtbl.h"
# include "libasm.h"
# include "util.h"

# define TOK_ERR(_col, s) ((t_tok){.type=err, .col=(_col), .str=s})

#define CMD_NUM_BYTES(type, opcode)									\
	(((type) & ~T_LAB) == T_DIR && g_op_tab[(opcode)-1].direct_size	\
	 ? SPECIAL_DIR_SIZE : g_cmd_sizes[(type) & ~T_LAB])

# define DISASM_BUF_SIZE (2048)
# define TOK_END ((t_tok){.type=end, .col=0, .num=0})

enum e_toktype {
	err,
	eof,
	space, // separators
	newline,
	separator,
	bot_name, // name/comment
	bot_comment,
	comment_char,
	label_def, // labels
	label_char,
	direct_char,
	instruction, // number things
	registry,
	num,
};

struct	s_tok {
	enum e_toktype	type;
	unsigned		col;
	union {
		char		*str;
		long		num;
		int			opcode;
		enum e_toktype	_type;
	};
};

union arg_un {
	char	*str;
	long	num;
};

struct	s_cmd {
	int				opcode;
	int				num_bytes;
	unsigned		linenum;
	unsigned		encoding;
	unsigned		cols[MAX_ARGS_NUMBER];
	long			argtypes[MAX_ARGS_NUMBER];
	union arg_un	args[MAX_ARGS_NUMBER];
};

struct	s_arg {
	int				type;
	union {
		long		num;
		char		*str;
	};
};

#define C(l) ((t_cmd*)((l)->content))
#define T(l) ((t_tok*)(l)->content)

# define TOK_TO_ERR(tok) ({											\
			if ((tok)->type == label_def || (tok)->type == bot_name	\
				|| (tok)->type == bot_comment)						\
				free((tok)->str);									\
			if ((tok)->type != err)									\
				(tok)->_type = (tok)->type;							\
			(tok)->type = err;										\
		})

# define WARNING_MOD TE(BOL) FG(LMGN)
# define ERR_MOD TE(BOL) FG(LRED)

# define ERR_FIRST_BIT ERR_MOD"ERROR"NC" LINE %u:%u\n"
# define ERR_SECOND_BIT "[%s]"
# define ERR_TEMPLATE(s) (ERR_FIRST_BIT s ERR_SECOND_BIT)

# define ERR_UNKNOWN_CMD ERR_TEMPLATE("Unknown command")

# define ERR_BAD_NAME ERR_FIRST_BIT"Bad name or comment"
# define ERR_BAD_CHAR_IN_LABEL ERR_TEMPLATE("Incomprehensible string: ")
# define ERR_BAD_NUM ERR_TEMPLATE("Poorly structured number: ")
# define ERR_EXPECTED_HEAD ERR_TEMPLATE("Expected comment or name.  Got: ")
# define ERR_UNEXPECTED_SYMBOL ERR_TEMPLATE("Unexpected symbol: ")
# define ERR_UNEXPECTED_ARG ERR_TEMPLATE("Unexpected argument to command \"%s\": ")
# define ERR_BAD_NUM_OF_ARGS ERR_FIRST_BIT"Bad number of arguments to command \"%s\""

// # define WARNING_UNKNOWN_LABEL WARNING_MOD"WARNING"NC" LINE %u:%u\nUnrecognized label "ERR_SECOND_BIT
// # define ERR_UNKNOWN_LABEL ERR_TEMPLATE("Unrecognized label ")
# define ERR_UNKNOWN_LABEL ERR_FIRST_BIT"Unrecognized label \"%s\""
# define WARNING_PROG_TOO_BIG WARNING_MOD"WARNING"NC" LINE %u\nProgram too large.  Truncating"

// disassembler errors
# define WARNING_PROG_TOO_BIG_DISASM WARNING_MOD"WARNING"NC" Binary too large.  Truncating"
# define WARNING_FILE_TRUNC WARNING_MOD"WARNING"NC" File appears to be truncated"
# define ERR_BAD_HEADER ERR_MOD"ERROR"NC" File has invalid header"
# define ERR_UNKNOWN_TYPE ERR_MOD"ERROR"NC" Unknown argument type %#04llx"
# define ERR_FILE_TOO_BIG ERR_MOD"ERROR"NC" File too large"
# define ERR_UNKNOWN_OPCODE ERR_MOD"ERROR"NC" Unknown opcode %#04hhx"

# define ERR_MALLOC_FAIL ERR_MOD"ERROR"NC" Malloc failure FILE %s LINE %d"

typedef struct s_tok	t_tok;
typedef struct s_cmd	t_cmd;
typedef struct s_arg	t_arg;

t_list		*lex_file(int fd);
t_list		*parse(t_list *tokens, size_t bufsize,
				   header_t *header, t_dict *dict);
void		resolve_labels(t_list *cmds, t_dict *htbl);
void		print_args(char *tab, size_t bufsize, t_list *cmd, size_t *size, header_t *header);

t_list		*addtolst(t_list *lst, void *content, size_t size);
int			write_to_file(int opts, char *filename, char *str, size_t size);
char		*assemble(int fd, size_t *size);

char	g_errarr[100];

struct s_option *g_asm_opts[5];
char		g_tok_to_str_safe[16][17];
char		g_tok_to_str[16][17];
char		g_cmd_sizes[9];
char		g_cmd_encoding[9];
char*(*g_tab[2])(int,size_t*);
char		g_arg_to_str[9][10];
char		g_encoding_cmd[9];
char		*g_cmd_str[9];

char *g_name_tab[2];
size_t g_name_len_tab[2];
size_t g_name_hash_tab[2];
size_t g_op_len_tab[NUM_OPS];
size_t g_op_hash_tab[NUM_OPS];

char		*g_errstr;

void write_mem(uint8_t *buf, uint32_t idx, uint64_t val, uint32_t size);
int	parse_bin(char *buf, size_t bufsize, int fd, size_t *size);

// run before main
void fill_tbls(void) __attribute__((constructor));

#endif
