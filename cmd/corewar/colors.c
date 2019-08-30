#include "colors.h"
#include "cpu.h"
#include "nuklear.h"
#include <math.h>

struct nk_color
  to_rgb(float h, float s, float v)
{
  float C;
  float X;
  float m;
  float r;
  float g;
  float b;

  while (h < 0)
    h += 360.;
  while (h > 360)
    h -= 360.;
  C = v * s;
  X = C * (1 - fabs(fmod(h / 60, 2) - 1));
  m = v - C;
  switch (((int)h) / 60) {
  case 0:
    r = C;
    g = X;
    b = 0;
    break;
  case 1:
    r = X;
    g = C;
    b = 0;
    break;
  case 2:
    r = 0;
    g = C;
    b = X;
    break;
  case 3:
    r = 0;
    g = X;
    b = C;
    break;
  case 4:
    r = X;
    g = 0;
    b = C;
    break;
  case 5:
    r = C;
    g = 0;
    b = X;
    break;
  }
  return ((struct nk_color){(r + m) * 255, (g + m) * 255, (b + m) * 255, 255});
}

struct s_hsv
  to_hsv(int r, int g, int b)
{
  float rp;
  float gp;
  float bp;
  float Cmax;
  float Cmin;
  float delta;
  float h;
  float s;
  float v;

  rp = r / 255.;
  gp = g / 255.;
  bp = b / 255.;
  Cmax = MAX(rp, MAX(gp, bp));
  Cmin = MIN(rp, MIN(gp, bp));
  delta = Cmax - Cmin;
  v = Cmax;
  if (Cmax == 0)
    s = 0;
  else
    s = delta / Cmax;
  if (delta == 0)
    h = 0;
  else if (Cmax == rp)
    h = 60 * (((gp - bp) / delta));
  else if (Cmax == gp)
    h = 60 * (((bp - rp) / delta) + 2);
  else if (Cmax == bp)
    h = 60 * (((rp - gp) / delta) + 4);
  else
    h = 0;
  return ((struct s_hsv){.h = h, .s = s, .v = v});
}
