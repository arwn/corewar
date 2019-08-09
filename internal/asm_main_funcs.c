/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   other.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/24 15:34:16 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/08 13:37:58 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "asm.h"

// max of size needed for assembler, size needed for disassembler
static char	g_bufarr[MAX(CHAMP_MAX_SIZE + sizeof(header_t), DISASM_BUF_SIZE)];

char	*assemble(int fd, size_t *size)
{
	if (g_errstr)
		free(g_errstr);
	g_errstr = NULL;

	header_t	header;
	t_list		*tokens;
	t_list		*cmds;
	t_dict		*htbl;

	ft_bzero(&header, sizeof(header_t));
	ft_bzero(g_bufarr, sizeof(g_bufarr));
	tokens = lex_file(fd);
	if (!tokens)
		return (NULL);

	htbl = dictInit(MEM_SIZE * 4);
	cmds = parse(tokens, sizeof(g_bufarr) - sizeof(header), &header, htbl);

	if (!cmds)
	{
		ft_lstdel(&tokens, free_);
		ft_lstdel(&cmds, free_);
		killDict(htbl);
		return (NULL);
	}

	resolve_labels(cmds->next, htbl);
	if (g_errstr)
	{
		ft_lstdel(&tokens, free_);
		ft_lstdel(&cmds, free_);
		killDict(htbl);
		return (NULL);
	}

	print_args(g_bufarr, sizeof(g_bufarr), cmds->next, size, &header);

	killDict(htbl);
	ft_lstdel(&tokens, (void(*)(void*,size_t))free_);
	ft_lstdel(&cmds, (void(*)(void*,size_t))free_);
	return (g_bufarr);
}

char	*disassemble(int fd, size_t *size)
{
	if (g_errstr)
		free(g_errstr);
	g_errstr = NULL;

	ft_bzero(g_bufarr, sizeof(g_bufarr));
	if (parse_bin(g_bufarr, sizeof(g_bufarr), fd, size))
		return (NULL);
	return (g_bufarr);
}

char*(*g_tab[])(int,size_t*) = {
	assemble,
	disassemble,
};

// determines and writes filled buffer to output file
int		write_to_file(int opts, char *filename, char *buf, size_t bufsize)
{
	char	ofilename[1024];
	char	*s;
	char	*end;
	int		fd;

	ft_bzero(ofilename, sizeof(ofilename));
	end = ft_strrchr(filename, '.');
	// makes sure there is enough space for '.cor' or '.s'
	s = ft_strncpy(ofilename, filename,
				   MIN((sizeof(ofilename) / sizeof(*ofilename)) - 5,
					   ((size_t)end - (size_t)filename)));
	ft_strcat(s, (!(opts & opt_getoptcode(g_asm_opts, 'd', NULL))) ? ".cor" : ".s");


	fd = open(ofilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		ft_dprintf(STDERR_FILENO, "Error opening file %s\n", ofilename);
		return (1);
	}
	ft_printf("-- Writing to ofile %s\n", ofilename);
	write(fd, buf, bufsize);
	close(fd);
	return (0);
}
