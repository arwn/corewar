#include "colors.h"
#include "cpu.h"
#include "nuklear.h"
#include <math.h>

struct nk_color to_rgb(float h, float s, float v) {
  while (h < 0)
    h += 360.;
  while (h > 360)
    h -= 360.;
  float C = v * s;
  float X = C * (1 - fabs(fmod(h / 60, 2) - 1));
  float m = v - C;
  float r, g, b;
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
  struct nk_color color = {(r + m) * 255, (g + m) * 255, (b + m) * 255, 255};
  return (color);
}

struct s_hsv to_hsv(int r, int g, int b) {
  float rp = r / 255.;
  float gp = g / 255.;
  float bp = b / 255.;
  float Cmax = MAX(rp, MAX(gp, bp));
  float Cmin = MIN(rp, MIN(gp, bp));
  float delta = Cmax - Cmin;
  float h, s, v;
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
