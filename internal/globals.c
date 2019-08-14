/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   globals.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/23 13:28:23 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/09 10:09:18 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "asm.h"

char		g_tok_to_str_safe[][17] = {
	[err] = "err",
	[eof] = "EOF",
	[space] = "space",
	[newline] = "\\n",
	[separator][0] = SEPARATOR_CHAR,
	[bot_name] = NAME_CMD_STRING,
	[bot_comment] = COMMENT_CMD_STRING,
	[label_char][0] = LABEL_CHAR,
	[direct_char][0] = DIRECT_CHAR,
	[direct_char][1] = DIRECT_CHAR,
	[comment_char][0] = COMMENT_CHAR,
	[label_def] = "label definition",
	[instruction] = "instruction",
	[registry] = "registry",
	[num] = "num",
};

char		g_tok_to_str[][17] = {
	[err] = "err",
	[eof] = "EOF",
	[space] = "space",
	[newline] = "\\n",
	[separator][0] = SEPARATOR_CHAR,
	[bot_name] = NAME_CMD_STRING,
	[bot_comment] = COMMENT_CMD_STRING,
	[label_char][0] = LABEL_CHAR,
	[direct_char][0] = DIRECT_CHAR,
	[comment_char][0] = COMMENT_CHAR,
	[label_def] = "label definition",
	[instruction] = "instruction",
	[registry] = "registry",
	[num] = "num",
};

char		g_cmd_sizes[] = {
	[T_DIR] = DIR_SIZE,
	[T_REG] = REG_SIZE,
	[T_IND] = IND_SIZE,
};

char		g_cmd_encoding[] = {
	[T_DIR] = DIR_CODE,
	[T_REG] = REG_CODE,
	[T_IND] = IND_CODE,
};

char		g_encoding_cmd[] = {
	[DIR_CODE] = T_DIR,
	[REG_CODE] = T_REG,
	[IND_CODE] = T_IND,
};

char		g_arg_to_str[][10] = {
	[T_DIR] = "direct",
	[T_REG] = "registry",
	[T_IND] = "indirect",
	[T_LAB] = "label",
};

char		*g_cmd_str[] = {
	[T_DIR] = "%",
	[T_REG] = "r",
	[T_IND] = "",
	[T_LAB] = ":",
};

char *g_name_tab[2] = {NAME_CMD_STRING, COMMENT_CMD_STRING};

void fill_tbls(void) {
	for (unsigned ii = 0; ii < sizeof(g_name_len_tab) / sizeof(*g_name_len_tab); ++ii)
		g_name_len_tab[ii] = ft_strlen(g_name_tab[ii]);

	for (unsigned ii = 0; ii < sizeof(g_name_hash_tab) / sizeof(*g_name_hash_tab); ++ii)
		g_name_hash_tab[ii] = ft_hash(g_name_tab[ii]);

	for (unsigned ii = 0; ii < sizeof(g_op_len_tab) / sizeof(*g_op_len_tab); ++ii)
		g_op_len_tab[ii] = ft_strlen(g_op_tab[ii+1].name); // TODO: actually change other g_*_tab tables to be like g_op_talb

	for (unsigned ii = 0; ii < sizeof(g_op_hash_tab) / sizeof(*g_op_hash_tab); ++ii)
		g_op_hash_tab[ii] = ft_hash(g_op_tab[ii+1].name);
}
