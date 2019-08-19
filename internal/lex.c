#include "asm.h"

/*
** Handles name and comment strings
*/

static char *get_str(char *s, unsigned *col) {
  unsigned end;
  int ii;
  unsigned start = ft_strspn(s, " \t");

  *col = start + 1;
  if (start[s] != '"')
    return (NULL);
  ++start;

  end = ft_strcspn(s + start, "\"");
  *col += end;
  if ((start + end)[s] != '"')
    return (NULL);
  ii = start + end + 1;
  while (s[ii] == ' ' || s[ii] == '\t')
    ++ii;
  *col = ii;
  if (s[ii])
    return (NULL);

  return (ft_strndup(s + start, end));
}

/*
** Handles complex tokens
*/

t_tok complex_token(char **s, char *str, t_tok tok) {
  unsigned ii;
  size_t len;
  size_t oplen;
  unsigned col;

  ii = -1;

  // name/comment
  while (++ii < sizeof(g_name_tab) / sizeof(*g_name_tab)) {
    len = g_name_len_tab[ii];
    if (ft_hashn(*s, len) == g_name_hash_tab[ii]) {
      tok = ((t_tok){.type = (bot_name + ii),
                     .col = *s - str,
                     .str = get_str(*s + len, &col)});
      **s = '\0';
      if (!tok.str) {
        tok = TOK_ERR(*s - str + len + col, ERR_BAD_NAME);
        return (tok);
      }
      break;
    }
  }

  // operation
  ii = -1;
  while (tok.type == err && ++ii < NUM_OPS) {
    len = ft_strcspn(*s, "\t \n");
    oplen = g_op_len_tab[ii];
    if (ft_hashn(*s, MAX(len, oplen)) == g_op_hash_tab[ii]) {
      tok = (t_tok){.type = instruction,
                    .col = *s - str,
                    .opcode = g_op_tab[ii + 1].opcode};
      *s += len;
      break;
    }
  }

  // label
  if (tok.type == err && ii > 0) {
    ii = ft_strspn(*s, LABEL_CHARS);
    if (ii == 0) {
      tok = TOK_ERR(*s - str, ERR_BAD_CHAR_IN_LABEL);
    } else {
      tok.type = label_def;
      tok.str = ft_strndup(*s, ii);
      *s += ii;
    }
  }
  return (tok);
}

/*
** Returns next possible token
*/

t_tok getnexttoken(char **s, char *str) {
  t_tok tok;
  int ii = 0;

  tok = TOK_ERR(*s - str, ERR_UNKNOWN_CMD);
  if ((ii = ft_strspn(*s, LABEL_CHARS)) &&
      ((*s != str && (*s)[-1] == LABEL_CHAR) || ((*s)[ii] == LABEL_CHAR)))
    return (complex_token(s, str, tok));
  switch (**s) {
  case ' ':
  case '\t':
    tok = ((t_tok){.type = space, .col = *s - str});
    break;
  case '-':
  case '0' ... '9':
    tok = ((t_tok){
        .type = num, .col = *s - str, .num = ft_atosize_tbase(*s, 10)});
    if (**s == '-')
      ++*s;
    if (**s < '0' || **s > '9')
      return (TOK_ERR(*s - str, ERR_BAD_NUM));
    *s += ft_strspn(*s, "0123456789");
    return (tok);
    break;
  case COMMENT_CHAR_ALT:
  case COMMENT_CHAR:
    tok = ((t_tok){.type = comment_char, .col = *s - str});
    **s = '\0';
    return (tok);
    break;
  case LABEL_CHAR:
    tok = ((t_tok){.type = label_char, .col = *s - str});
    break;
  case DIRECT_CHAR:
    tok = ((t_tok){.type = direct_char, .col = *s - str});
    break;
  case SEPARATOR_CHAR:
    tok = ((t_tok){.type = separator, .col = *s - str});
    break;
  case 'r':
    tok = ((t_tok){.type = registry, .col = *s - str, .num = 0});
    break;
  }
  if (tok.type == err)
    return (complex_token(s, str, tok));
  ++*s;
  return (tok);
}

/*
** Appends to list.  Must be used on NULL list to not break between different
** lists
*/

t_list *addtolst(t_list *lst, void *content, size_t size) {
  static t_list *ptr = NULL;

  if (!lst) {
    lst = ft_lstnew(content, size);
    lst->next = NULL;
    ptr = lst;
    return (lst);
  }
  ptr->next = ft_lstnew(content, size);
  ptr = ptr->next;
  return (lst);
}

/*
** Reads from fd, returns list of tokens
*/

t_list *lex_file(int fd) {
  unsigned linenum;
  char *str;
  char *sptr;
  t_tok token;
  t_list *lst;

  str = NULL;
  lst = NULL;
  if (fd < 0) {
    asprintf(&g_errstr, "Fatal error.  Unable to read from file");
    return (NULL);
  }
  linenum = 1;
  while ((get_next_line(fd, &str))) {
    if (!(str)) {
      asprintf(&g_errstr, "Fatal error.  Unable to read from file");
      return (NULL);
    }
    sptr = str;
    while (*str) {
      token = getnexttoken(&sptr, str);
      if (token.type == err) {
        asprintf(&g_errstr, token.str, linenum, token.col, str);
        ft_lstdel(&lst, (void (*)(void *, size_t))free_);
        free(str);
        return (NULL);
      }
      if (token.type != comment_char)
        lst = addtolst(lst, &token, sizeof(t_tok));
      if (!*sptr)
        break;
    }
    lst = addtolst(lst, &(t_tok){.type = newline, .col = sptr - str},
                   sizeof(t_tok));
    ++linenum;
    free(str);
  }
  lst = addtolst(lst, &(t_tok){.type = eof}, sizeof(t_tok));
  return (lst);
}
