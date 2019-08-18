/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_args.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/23 11:25:22 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/08 13:21:30 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "asm.h"
#include <stdio.h>

size_t	write_to_buf(char *buf, size_t size, size_t n) {
	unsigned ii;
	for (ii = 0; ii < size; ++ii)
		buf[ii] = (n >> ((size - 1 - ii) * 8)) & 0xff;
	return (ii);
}

size_t	printCmd(char *buf, size_t ii, size_t bufsize, t_cmd *cmd) {
	buf[ii++] = cmd->opcode;
	if (g_op_tab[cmd->opcode].param_encode)
		ii += write_to_buf(buf + ii, ENC_SIZE, cmd->encoding);

	for (int jj = 0; ii < bufsize && jj < g_op_tab[cmd->opcode].numargs; ++jj) {
		unsigned n = cmd->args[jj].num;
		ii += write_to_buf(buf + ii, CMD_NUM_BYTES(cmd->argtypes[jj], cmd->opcode), n);
	}
	return (ii);
}

/*
** Prints header and list of commands to buff, not exceeding bufsize
*/

void	print_args(char *buf, size_t bufsize, t_list *cmd, size_t *size, header_t *header)
{
	size_t	ii;

	ii = 0;

	header->magic = htonl(header->magic);
	header->prog_size = htonl(header->prog_size);
	ft_memmove(buf, header, sizeof(header_t));
	ii += sizeof(header_t);

	for (t_list *ll = cmd; ll; ll = ll->next) {
		if (ii > (unsigned)(bufsize - C(ll)->num_bytes)) {
			ft_dprintf(STDERR_FILENO, WARNING_PROG_TOO_BIG"\n", C(ll)->linenum);
			break ;
		}
		ii = printCmd(buf, ii, bufsize, C(ll));
	}
	*size = ii;
}
