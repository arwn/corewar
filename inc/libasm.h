/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libasm.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/25 13:53:27 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/29 19:58:30 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIBASM_H
# define LIBASM_H

# include "libft.h"

char	*assemble(int fd, size_t *size);
char	*disassemble(int fd, size_t *size);
int		write_to_file(int opts, char *filename, char *buf, size_t bufsize);

#endif
