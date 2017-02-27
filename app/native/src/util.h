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

  HSLColor RGBToHSL(float r, float g, float b);
  HSLColor RGBToHSL(RGBColor& c);

  RGBColor HSLToRGB(float h, float s, float l);
  RGBColor HSLToRGB(HSLColor& c);
  RGBColor HSYToRGB(float h, float s, float y);
  RGBColor HSYToRGB(HSYColor& c);

  HSYColor RGBToHSY(float r, float g, float b);
  HSYColor RGBToHSY(RGBColor& c);

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
}