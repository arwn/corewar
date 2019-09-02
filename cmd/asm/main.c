/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/11 12:08:39 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/19 14:19:28 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "asm.h"

struct s_option *g_asm_opts[] = {
	&(struct s_option){'d', "disassemble", "Disassemble .cor file", 1 << 0},
	&(struct s_option){'h', "help", "Display help message", 1 << 1},
	&(struct s_option){'q', "quiet", "Run with no output", 1 << 2},
	&(struct s_option){'n', "nowrite", "Do not write to output file", 1 << 3},
	&(struct s_option){'f', "force", "Force disassembly output if error",
		1 << 4},
	NULL,
};

char	g_force_disasm = 0;

void	opteroni(int opts, int argc, char **argv, int ii)
{
	if (opts & opt_getoptcode(g_asm_opts, 'q', NULL))
	{
		fclose(stdout);
		fclose(stderr);
	}
	if (opts & opt_getoptcode(g_asm_opts, 'h', NULL))
	{
		opt_printusage(g_asm_opts, argv);
		exit(0);
	}
	else if (argc - ii <= 0)
	{
		opt_printusage(g_asm_opts, argv);
		exit(1);
	}
	if (opts & opt_getoptcode(g_asm_opts, 'f', NULL))
		g_force_disasm |= 1;
}

#define CHK(C) (OPTS & opt_getoptcode(g_asm_opts, (C), NULL))

#define OPTS (arr[0])
#define II (arr[1])
#define R (arr[2])
#define FD (arr[3])
#define WRTF write_to_file
#define STDL ft_strdel

int		main(int argc, char **argv)
{
	int		arr[4];
	char	*s;
	size_t	size;

	II = 1;
	OPTS = opt_getopts(g_asm_opts, argc, argv, &II);
	DO_ALL(1, opteroni(OPTS, argc, argv, II), R = 0);
	while (II < argc)
	{
		size = 0;
		FD = open(argv[II], O_RDONLY);
		if (FD < 0)
			ft_dprintf(2, "Unable to read file %s\n", argv[II]);
		else
		{
			ft_printf("-- %sssembling %s\n", CHK('d') ? "Disa" : "A", argv[II]);
			s = g_tab[!!CHK('d')](FD, &size);
			if (g_errstr)
				DO_ALL(ft_dprintf(2, "%s\n", g_errstr), STDL(&g_errstr), R = 1);
			R |= (s && !CHK('n')) ? WRTF(OPTS, argv[II], s, size) : !s;
			close(FD);
		}
		++II;
	}
	return (R);
}
