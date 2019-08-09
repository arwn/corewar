/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/11 12:08:39 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/09 10:49:17 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "asm.h"

struct s_option *g_asm_opts[] = {
	&(struct s_option){'d', "disassemble", "Disassemble .cor file", 1 << 0},
	&(struct s_option){'h', "help", "Display help message", 1 << 1},
	&(struct s_option){'q', "quiet", "Run with no output", 1 << 2},
	&(struct s_option){'n', "nowrite", "Do not write to output file", 1 << 3},
	NULL,
};

int main(int argc, char **argv) {
	int opts;
	int ii;
	int r;
	char *s;
	int fd;
	size_t size;

	ii = 1;
	opts = opt_getopts(g_asm_opts, argc, argv, &ii);
	if (opts & opt_getoptcode(g_asm_opts, 'q', NULL)) {
		fclose(stdout);
		fclose(stderr);
	}
	if (opts & opt_getoptcode(g_asm_opts, 'h', NULL)) {
		opt_printusage(g_asm_opts, argv);
		return (0);
	} else if (argc - ii <= 0) {
		opt_printusage(g_asm_opts, argv);
		return (1);
	}
	r = 0;
	while (ii < argc) {
		size = 0;
		fd = open(argv[ii], O_RDONLY);
		if (fd < 0)
			ft_dprintf(STDERR_FILENO, "Unable to read from file %s\n", argv[ii]);
		else {
			ft_printf("-- %sssembling %s\n",
					  opts & opt_getoptcode(g_asm_opts, 'd', NULL)
					  ? "Disa" : "A", argv[ii]);
			s = g_tab[!!(opts & opt_getoptcode(g_asm_opts, 'd', NULL))](fd, &size);
			if (g_errstr) {
				ft_dprintf(STDERR_FILENO, "%s\n", g_errstr);
				free(g_errstr);
				g_errstr = NULL;
			}
			if (s && !(opts & opt_getoptcode(g_asm_opts, 'n', NULL)))
				r |= write_to_file(opts, argv[ii], s, size);
			else
				r |= !!s;
			close(fd);
		}

		++ii;
	}
	return (r);
}
