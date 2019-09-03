/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   util.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 19:56:00 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 19:56:01 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTIL_H
# define UTIL_H

# include <stdint.h>
# include <unistd.h>
# include "op.h"

/*
** for use with lstdel. Deletes elements of list
*/

void		free_(void *a, size_t b);

int			mod_idx(int idx);
uint8_t		read_mem_1(uint8_t *program, uint32_t idx);
uint16_t	read_mem_2(uint8_t *program, uint32_t idx);
uint32_t	read_mem_4(uint8_t *program, uint32_t idx);

#endif
