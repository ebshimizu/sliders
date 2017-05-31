/*
util.h - Various helper classes and functions
author: Evan Shimizu
*/

#pragma once

#include <algorithm>
#include <vector>
#include <map>

using namespace std;

namespace Comp {
  static map<int, string> intervalNames = { {0, "reds"} , {1, "yellows" }, {2, "greens" }, {3, "cyans"}, {4, "blues" }, {5, "magentas" } };

  // utility functions for converting between color spaces
  template<typename T>
  static class Utils {
  public:
    struct RGBColorT {
      T _r;
      T _g;
      T _b;
    };

    struct RGBAColorT {
      T _r;
      T _g;
      T _b;
      T _a;
    };

    struct HSLColorT {
      T _h;
      T _s;
      T _l;
    };

    struct HSYColorT {
      T _h;
      T _s;
      T _y;
    };

    struct CMYKColorT {
      T _c;
      T _m;
      T _y;
      T _k;
    };

    HSLColorT RGBToHSL(T r, T g, T b);
    HSLColorT RGBToHSL(RGBColorT& c);

    RGBColorT HSLToRGB(T h, T s, T l);
    RGBColorT HSLToRGB(HSLColorT& c);
    RGBColorT HSYToRGB(T h, T s, T y);
    RGBColorT HSYToRGB(HSYColorT& c);
    RGBColorT CMYKToRGB(T c, T m, T y, T k);
    RGBColorT CMYKToRGB(CMYKColorT& c);
    RGBColorT LabToRGB(T L, T a, T b); // Lab isn't really used internally right now, so this is mostly one way

    HSYColorT RGBToHSY(T r, T g, T b);
    HSYColorT RGBToHSY(RGBColorT& c);

    CMYKColorT RGBToCMYK(T r, T g, T b);
    CMYKColorT RGBToCMYK(RGBColorT& c);

    float rgbCompand(T x);
    float clamp(T val, T mn, T mx);

    class PointT {
    public:
      Point(T x, T y) : _x(x), _y(y) {}

      T _x;
      T _y;
    };

    // a curve here is implemented as a cubic hermite spline
    class CurveT {
    public:
      CurveT() {}
      CurveT(vector<PointT> pts);

      vector<PointT> _pts;
      vector<T> _m;

      void computeTangents();
      T eval(T x);
    };

    // very simple linear interpolation of gradients.
    // does not handle the complicated parts of gradient creation in PS
    // (like the midpoint)
    class GradientT {
    public:
      GradientT() {}
      GradientT(vector<T> x, vector<RGBColorT> colors) : _x(x), _colors(colors) {}

      vector<T> _x;
      vector<RGBColorT> _colors;

      RGBColorT eval(T x);
    };
  };

  //typedef Utils<float>::RGBColorT RGBColor;
  //typedef Utils<float>::RGBAColorT RGBAColor;
  //typedef Utils<float>::CMYKColorT CMYKColor;
  //typedef Utils<float>::HSLColorT HSLColor;
  //typedef Utils<float>::HSYColorT HSYColor;
  //typedef Utils<float>::PointT Point;
  //typedef Utils<float>::GradientT Gradient;
  //typedef Utils<float>::CurveT Curve;
}

#include "util.cpp"