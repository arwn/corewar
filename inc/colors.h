#ifndef COLORS_H
# define COLORS_H

struct s_hsv {
  float h;
  float s;
  float v;
};

struct nk_color to_rgb(float h, float s, float v);
struct s_hsv to_hsv(int r, int g, int b);

#endif
