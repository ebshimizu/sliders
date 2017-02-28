/*
util.h - Various helper classes and functions
author: Evan Shimizu
*/

#pragma once

#include <algorithm>
#include <vector>

using namespace std;

namespace Comp {
  // utility functions for converting between color spaces
  struct RGBColor {
    float _r;
    float _g;
    float _b;
  };

  struct HSLColor {
    float _h;
    float _s;
    float _l;
  };

  struct HSYColor {
    float _h;
    float _s;
    float _y;
  };

  struct CMYKColor {
    float _c;
    float _m;
    float _y;
    float _k;
  };

  HSLColor RGBToHSL(float r, float g, float b);
  HSLColor RGBToHSL(RGBColor& c);

  RGBColor HSLToRGB(float h, float s, float l);
  RGBColor HSLToRGB(HSLColor& c);
  RGBColor HSYToRGB(float h, float s, float y);
  RGBColor HSYToRGB(HSYColor& c);
  RGBColor CMYKToRGB(float c, float m, float y, float k);
  RGBColor CMYKToRGB(CMYKColor& c);

  HSYColor RGBToHSY(float r, float g, float b);
  HSYColor RGBToHSY(RGBColor& c);

  CMYKColor RGBToCMYK(float r, float g, float b);
  CMYKColor RGBToCMYK(RGBColor& c);

  float clamp(float val, float mn, float mx);

  class Point {
  public:
    Point(float x, float y) : _x(x), _y(y) {}

    float _x;
    float _y;
  };

  // a curve here is implemented as a cubic hermite spline
  class Curve {
  public:
    Curve() {}
    Curve(vector<Point> pts);

    vector<Point> _pts;
    vector<float> _m;

    void computeTangents();
    float eval(float x);
  };

  // very simple linear interpolation of gradients.
  // does not handle the complicated parts of gradient creation in PS
  // (like the midpoint)
  class Gradient {
  public:
    Gradient() {}
    Gradient(vector<float> x, vector<RGBColor> colors) : _x(x), _colors(colors) {}

    vector<float> _x;
    vector<RGBColor> _colors;

    RGBColor eval(float x);
  };
}