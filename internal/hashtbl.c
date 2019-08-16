/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hashtbl.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acarlson <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/07/17 12:27:08 by acarlson          #+#    #+#             */
/*   Updated: 2019/08/16 15:28:17 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "hashtbl.h"

struct s_dict	*dictInit(int capacity) {
	struct s_dict *dict;
	struct s_item **items;

	if (!(dict = malloc(sizeof(struct s_dict))))
		return (NULL);
	if (!(items = malloc(sizeof(struct s_item*) * capacity)))
	{
		free(dict);
		return (NULL);
	}
	ft_bzero(items, sizeof(struct s_items*) * capacity);
	dict->items = items;
	dict->capacity = capacity;
	return (dict);
}

/*
** returns 1 if ok
**         0 if malloc fail
**         -1 if duplicate
*/

int				dictInsert(struct s_dict *dict, char *str, size_t value)
{
	if (!dict || !str)
		return (0);

	size_t	key = ft_hash(str);
	size_t	idx = key % dict->capacity;
	struct s_item *ii;

	struct s_item *item;
	if (!(item = malloc(sizeof(struct s_item))))
		return (0);
	ii = dict->items[idx];
	while (ii)
	{
		if (!ft_strcmp(str, ii->str))
		{
			free(item);
			return (1);
		}
		ii = ii->next;
	}
	item->str = ft_strdup(str);
	item->value = value;
	item->next = dict->items[idx];
	dict->items[idx] = item;
	return (1);
}

size_t			dictSearch(struct s_dict *dict, char *str)
{
	size_t idx;
	struct s_item *ii;
	size_t	key;

	key = ft_hash(str);
	idx = key % dict->capacity;
	if (!dict)
		return (-1);
	if (idx < dict->capacity)
	{
		ii = dict->items[idx];
		while (ii)
		{
			if (!ft_strcmp(str, ii->str))
				return (ii->value);
			ii = ii->next;
		}
	}
	return (-1);
}

void			killDict(struct s_dict *dict)
{
	unsigned		ii;
	struct s_item	*item;

	ii = 0;
	while (ii < dict->capacity)
	{
		item = dict->items[ii];
		while (item) {
			struct s_item *tmp = item;
			item = item->next;
			free(tmp->str);
			free(tmp);
		}
		++ii;
	}
	free(dict->items);
	free(dict);
}
