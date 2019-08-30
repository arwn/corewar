/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   asm.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/11 14:08:29 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/19 14:19:18 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ASM_H
# define ASM_H

# include "hashtbl.h"
# include "libasm.h"
# include "libft.h"
# include "op.h"
# include "util.h"
# include <fcntl.h>
# include <stdio.h>
# include <unistd.h>

# define TOK_ERR(_col, s) ((t_tok){.type = err, .col = (_col), .u.str = s})

# define CMD_NUM_BYTES(type, opcode) \
	(((type) & ~T_LAB) == T_DIR && g_op_tab[(opcode)].direct_size \
			 ? SPECIAL_DIR_SIZE \
			 : g_cmd_sizes[(type) & ~T_LAB])

# define DISASM_BUF_SIZE (2048)
# define TOK_END ((t_tok){.type = end, .col = 0, .num = 0})

/*
// space        - separators
// bot_name     - name/comment
// label_def    - labels
// instruction  - number things
*/

enum	e_toktype {
	err,
	eof,
	space,
	newline,
	separator,
	bot_name,
	bot_comment,
	comment_char,
	label_def,
	label_char,
	direct_char,
	instruction,
	registry,
	num,
};

union	u_tok {
	char			*str;
	long			num;
	int				opcode;
	enum e_toktype	_type;
};

struct	s_tok {
	enum e_toktype	type;
	unsigned		col;
	union u_tok		u;
};

union	arg_un {
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

struct s_arg {
	int				type;
	union arg_un	u;
};

# define C(l) ((t_cmd *)((l)->content))
# define T(l) ((t_tok *)(l)->content)

# define TOK_TO_ERR(tok) \
	({ \
		if ((tok)->type == label_def || (tok)->type == bot_name || (tok)->type == bot_comment) \
			free((tok)->u.str); \
		if ((tok)->type != err) \
			(tok)->u._type = (tok)->type; \
		(tok)->type = err; \
		})

# ifdef COLORS
#  define WARNING_MOD TE(BOL) FG(LMGN)
#  define ERR_MOD TE(BOL) FG(LRED)
# else
#  ifdef NC
#   undef NC
#  endif
#  define NC ""
#  define WARNING_MOD ""
#  define ERR_MOD ""
# endif

# define ERR_ST ERR_MOD "ERROR" NC " LINE %u:%u\n"
# define ERR_SECOND_BIT "[%s]"
# define ERR_TMPL(s) (ERR_ST s ERR_SECOND_BIT)

# define ERR_UNKNOWN_CMD ERR_TMPL("Unknown command")

# define ERR_BAD_NAME ERR_ST "Bad name or comment"
# define ERR_BAD_CHAR_IN_LABEL ERR_TMPL("Incomprehensible string: ")
# define ERR_BAD_NUM ERR_TMPL("Poorly structured number: ")
# define ERR_EXPECTED_HEAD ERR_TMPL("Expected comment or name.  Got: ")
# define ERR_UNEXPECTED_SYMBOL ERR_TMPL("Unexpected symbol: ")
# define ERR_UNEXPECTED_ARG ERR_TMPL("Unexpected argument to command \"%s\": ")
# define ERR_BAD_NUM_OF_ARGS ERR_ST "Bad number of arguments to command \"%s\""

# define ERR_UNKNOWN_LABEL ERR_ST "Unrecognized label \"%s\""
# define WARNING_PROG_TOO_BIG WARNING_MOD "WARNING" NC " LINE %u\nProgram too large. Truncating"

/*
** disassembler errors
*/

# define WARNING_PROG_TOO_BIG_DISASM WARNING_MOD "WARNING" NC " Binary too large. Truncating"
# define WARNING_FILE_TRUNC WARNING_MOD "WARNING" NC " File appears to be truncated"
# define ERR_BAD_HEADER ERR_MOD "ERROR" NC " File has invalid header"
# define ERR_UNKNOWN_TYPE ERR_MOD "ERROR" NC " Unknown argument type %#04llx"
# define ERR_FILE_TOO_BIG ERR_MOD "ERROR" NC " File too large"
# define ERR_UNKNOWN_OPCODE ERR_MOD "ERROR" NC " Unknown opcode %#04hhx"

# define ERR_MALLOC_FAIL ERR_MOD "ERROR" NC " Malloc failure FILE %s LINE %d"

typedef struct s_tok	t_tok;
typedef struct s_cmd	t_cmd;
typedef struct s_arg	t_arg;

t_list			*lex_file(int fd);
t_list			*parse(t_list *tokens, size_t bufsize, header_t *header,
t_dict *dict);
void			resolve_labels(t_list *cmds, t_dict *htbl);
void			print_args(char *tab, size_t bufsize, t_list *cmd, size_t *size,
header_t *header);

t_list			*addtolst(t_list *lst, void *content, size_t size);
int				write_to_file(int opts, char *filename, char *str, size_t size);
char			*assemble(int fd, size_t *size);

char			g_errarr[100];

struct s_option	*g_asm_opts[6];
char 			g_tok_to_str_safe[16][17];
char 			g_tok_to_str[16][17];
char 			g_cmd_sizes[9];
char 			g_cmd_encoding[9];
char 			*(*g_tab[2])(int, size_t *);
char 			g_arg_to_str[9][10];
char 			g_encoding_cmd[9];
char 			*g_cmd_str[9];

char			*g_name_tab[2];
size_t			g_name_len_tab[2];
size_t			g_name_hash_tab[2];
size_t			g_op_len_tab[NUM_OPS];
size_t			g_op_hash_tab[NUM_OPS];

char			*g_errstr;
char			g_is_extended;
char			g_force_disasm;

void			write_mem(uint8_t *buf, uint32_t idx, uint64_t val,
uint32_t size);
int				parse_bin(char *buf, size_t bufsize, int fd, size_t *size);

/*
// run before main
*/

void			fill_tbls(void) __attribute__((constructor));

#endif
