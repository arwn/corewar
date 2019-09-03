/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   resolve_labels.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/22 15:43:43 by acarlson          #+#    #+#             */
/*   Updated: 2019/07/29 16:36:57 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "asm.h"

void resolve_labels(t_list *cmds, t_dict *htbl)
{
	unsigned	position;
	t_list		*lst;
	t_cmd		*c;
	int			ii;
	char		*tmp;
	size_t		n;

	position = 0;
	for (lst = cmds; lst; lst = lst->next)
	{
		c = C(lst);
		for (ii = 0; ii < g_op_tab[c->opcode].numargs; ++ii)
		{
			if (c->argtypes[ii] & T_LAB)
			{
				tmp = c->args[ii].str;
				n = dict_search(htbl, tmp);
				if (n == (size_t)-1)
				{
					asprintf(&g_errstr, ERR_UNKNOWN_LABEL, c->linenum, c->cols[ii], tmp);
					return;
				}
				else
					c->args[ii].num = (n - position);
				c->argtypes[ii] &= ~T_LAB;
			}
		}
		position += c->num_bytes;
	}
}
