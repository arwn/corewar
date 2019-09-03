/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   colors.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: callen <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/08/29 22:25:55 by callen            #+#    #+#             */
/*   Updated: 2019/08/29 22:25:56 by callen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "colors.h"
#include "cpu.h"
#include "nuklear.h"
#include <math.h>

#undef C
#define C arr[0]
#define X arr[1]
#define M arr[2]
#define R arr[3]
#define G arr[4]
#define B arr[5]

struct nk_color
	to_rgb(float h, float s, float v)
{
	float arr[6];

	while (h < 0)
		h += 360.;
	while (h > 360)
		h -= 360.;
	C = v * s;
	X = C * (1 - fabs(fmod(h / 60, 2) - 1));
	M = v - C;
	switch (((int)h) / 60) {
	case 0:
		R = C;
		G = X;
		B = 0;
		break;
	case 1:
		R = X;
		G = C;
		B = 0;
		break;
	case 2:
		R = 0;
		G = C;
		B = X;
		break;
	case 3:
		R = 0;
		G = X;
		B = C;
		break;
	case 4:
		R = X;
		G = 0;
		B = C;
		break;
	case 5:
		R = C;
		G = 0;
		B = X;
		break;
	}
	return ((struct nk_color){(R + M) * 255, (G + M) * 255, (B + M) * 255, 255});
}

struct s_hsv
	to_hsv(int r, int g, int b)
{
	float rp;
	float gp;
	float bp;
	float c_max;
	float c_min;
	float delta;
	float h;
	float s;
	float v;

	rp = r / 255.;
	gp = g / 255.;
	bp = b / 255.;
	c_max = MAX(rp, MAX(gp, bp));
	c_min = MIN(rp, MIN(gp, bp));
	delta = c_max - c_min;
	v = c_max;
	if (c_max == 0)
		s = 0;
	else
		s = delta / c_max;
	if (delta == 0)
		h = 0;
	else if (c_max == rp)
		h = 60 * (((gp - bp) / delta));
	else if (c_max == gp)
		h = 60 * (((bp - rp) / delta) + 2);
	else if (c_max == bp)
		h = 60 * (((rp - gp) / delta) + 4);
	else
		h = 0;
	return ((struct s_hsv){.h = h, .s = s, .v = v});
}
