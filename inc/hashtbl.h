/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hashtbl.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/17 12:53:02 by acarlson          #+#    #+#             */
/*   Updated: 2019/07/20 17:39:56 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HASHTBL_H
# define HASHTBL_H

# include "libft.h"
# include <stdlib.h>

struct			s_item {
	char			*str;
	size_t			value;
	struct s_item	*next;
};

struct			s_dict {
	struct s_item	**items;
	unsigned		capacity;
};

typedef struct s_dict	t_dict;

struct s_dict	*dict_init(int capacity);
int				dict_insert(struct s_dict *dict, char *str, size_t value);
size_t			dict_search(struct s_dict *dict, char *str);
void			kill_dict(struct s_dict *dict);

#endif
