#include "asm.h"

int larger_than_bufsize(int *err, unsigned ii) {
	*err = 1;
	ft_dprintf(STDERR_FILENO, WARNING_PROG_TOO_BIG_DISASM"\n");
	return (ii);
}

/*
** Convert encoding byte to array of types (T_INT, T_DIR, T_REG)
*/

void find_arg_types(unsigned char *argtypes, unsigned char enc) {
	for (unsigned ii = 0; ii < 4; ++ii) {
		unsigned char nn = (enc >> ((6 - (ii * 2)))) & 0b11;
		argtypes[ii] = g_encoding_cmd[nn];
	}
}

/*
** Print next full instruction in progbuf to linebuf
** Sets *err if there is an error
*/

unsigned next_instruction(char *linebuf, size_t bufsize, size_t *bufidx, uint8_t *progbuf, size_t progbuf_size, int *err) {
	unsigned char	argtypes[MAX_ARGS_NUMBER] = {0};
	unsigned ii = 0;
	unsigned char	opcode = progbuf[ii++];

	if (opcode < 1 || opcode > sizeof(g_op_tab) / sizeof(*g_op_tab)) { // YTHO?
		*err = 1;
		asprintf(&g_errstr, ERR_UNKNOWN_OPCODE, progbuf[ii-1]);
		return (ii);
	}

	if (g_op_tab[opcode].param_encode) {
		find_arg_types(argtypes, progbuf[ii++]);
	}
	else {
		for (unsigned jj = 0; jj < MAX_ARGS_NUMBER; ++jj)
			argtypes[jj] = g_op_tab[opcode].argtypes[jj];
	}

	if (*bufidx >= bufsize)
		return (larger_than_bufsize(err, ii));

	ft_strcat(linebuf + *bufidx, g_op_tab[opcode].name);
	*bufidx += g_op_len_tab[opcode-1];
	for (int jj = 0; *bufidx < bufsize && jj < g_op_tab[opcode].numargs && ii < progbuf_size; ++jj) {
		ft_strcat(linebuf + *bufidx, jj ? "," : " ");
		*bufidx += 1;

		if (*bufidx >= bufsize)
			return (larger_than_bufsize(err, ii));

		int num_bytes = CMD_NUM_BYTES(argtypes[jj], opcode);

		printf("%zu\n", *bufidx);
		uint64_t nn = argtypes[jj];	// T_INT || T_DIR || T_REG
		if (!nn || nn > T_IND || !g_cmd_encoding[nn]) {
			*err = 1;
			asprintf(&g_errstr, ERR_UNKNOWN_TYPE, nn);
			return (ii);
		}

		ft_strcat(linebuf + *bufidx, g_cmd_str[(int)argtypes[jj]]);
		*bufidx += ft_strlen(g_cmd_str[(int)argtypes[jj]]);
		if (*bufidx >= bufsize)
			return (larger_than_bufsize(err, ii));

		nn = 0;

		switch (num_bytes) {
		case 1:
			nn = (int8_t)read_mem_1(progbuf, ii);
			break ;
		case 2:
			nn = (int16_t)read_mem_2(progbuf, ii);
			break ;
		case 4:
			nn = (int32_t)read_mem_4(progbuf, ii);
			break ;
		case 8:
			nn = (int64_t)read_mem_8(progbuf, ii);
			break ;
		}
		ii += num_bytes;
		char *s = ft_ssize_ttoabase(nn, 10);
		ft_strcat(linebuf + *bufidx, s);
		*bufidx += ft_strlen(s);
		free(s);
	}
	ft_strcat(linebuf + *bufidx, "\n");
	*bufidx += 1;
	return (ii);
}

/*
** Reads from fd, prints asm representation to buf not exceeding bufsize
*/

int	parse_bin(char *buf, size_t bufsize, int fd, size_t *size) {
	header_t	header;

	ft_bzero(&header, sizeof(header));

	size_t headersize = read(fd, &header, sizeof(header));

	if (headersize != sizeof(header)) {
		asprintf(&g_errstr, ERR_BAD_HEADER);
		return (1);
	}

	header.magic = ntohl(header.magic);
	header.prog_size = ntohl(header.prog_size);

	if (header.magic != COREWAR_EXEC_MAGIC) {
		asprintf(&g_errstr, ERR_BAD_HEADER);
		return (1);
	}

	ft_strcat(buf, NAME_CMD_STRING" \"");
	ft_strcat(buf, header.prog_name);
	ft_strcat(buf, "\"\n"COMMENT_CMD_STRING" \"");
	ft_strcat(buf, header.comment);
	ft_strcat(buf, "\"\n");

	uint8_t *progbuf = malloc(header.prog_size + 1);
	if (!progbuf) {
		asprintf(&g_errstr, ERR_MALLOC_FAIL, __FILE__, __LINE__);
		*size = 0;
		return (1);
	}
	size_t progbuf_size = read(fd, progbuf, header.prog_size);

	size_t bufidx = ft_strlen(buf);
	int err = 0;
	unsigned ii = 0;
	for (ii = 0; ii < progbuf_size; ) {
		ii += next_instruction(buf, bufsize - 10, &bufidx, progbuf + ii, progbuf_size - ii, &err);
		if (err) {
			free(progbuf);
			progbuf = NULL;
			*size = 0;
			return (1);
		}
	}
	*size = bufidx;
	if (ii < header.prog_size)
		ft_dprintf(STDERR_FILENO, WARNING_FILE_TRUNC"\n");
	free(progbuf);
	progbuf = NULL;
	return (0);
}
