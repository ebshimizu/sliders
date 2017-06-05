/*
util.h - Various helper classes and functions
author: Evan Shimizu
*/

#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <cmath>
#include <iostream>
#include <vector>
#include <functional>
#include <cassert>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>

using namespace std;

#include "../../../expression_tree/expressionContext.h"

namespace Comp {
  static map<int, string> intervalNames = { {0, "reds"} , {1, "yellows" }, {2, "greens" }, {3, "cyans"}, {4, "blues" }, {5, "magentas" } };

  template <typename T>
  inline T clamp(T val, T mn, T mx) {
    return (val > mx) ? mx : ((val < mn) ? mn : val);
  }

  template <>
  inline ExpStep clamp(ExpStep val, ExpStep mn, ExpStep mx) {
    vector<ExpStep> res = val.context->callFunc("clamp", val, mn, mx);
    return res[0];
  }

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

    template <typename T>
    T eval(T x);
  };

  // utility functions for converting between color spaces
  template<typename T>
  class Utils {
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

    static HSLColorT RGBToHSL(T r, T g, T b);
    static HSLColorT RGBToHSL(RGBColorT& c);

    static RGBColorT HSLToRGB(T h, T s, T l);
    static RGBColorT HSLToRGB(HSLColorT& c);
    static RGBColorT HSYToRGB(T h, T s, T y);
    static RGBColorT HSYToRGB(HSYColorT& c);
    static RGBColorT CMYKToRGB(T c, T m, T y, T k);
    static RGBColorT CMYKToRGB(CMYKColorT& c);
    static RGBColorT LabToRGB(T L, T a, T b); // Lab isn't really used internally right now, so this is mostly one way

    static HSYColorT RGBToHSY(T r, T g, T b);
    static HSYColorT RGBToHSY(RGBColorT& c);

    static CMYKColorT RGBToCMYK(T r, T g, T b);
    static CMYKColorT RGBToCMYK(RGBColorT& c);

    static T rgbCompand(T x);
  };

  typedef Utils<float>::RGBColorT RGBColor;

  // very simple linear interpolation of gradients.
  // does not handle the complicated parts of gradient creation in PS
  // (like the midpoint)
  class Gradient {
  public:
    Gradient() {}
    Gradient(vector<float> x, vector<RGBColor> colors) : _x(x), _colors(colors) {}

    vector<float> _x;
    vector<RGBColor> _colors;

    template<typename T>
    inline typename Utils<T>::RGBColorT eval(T x);
  };

  template<typename T>
  inline typename Utils<T>::HSLColorT Utils<T>::RGBToHSL(T r, T g, T b)
  {
    HSLColorT hsl;

    T cmax = max(r, max(g, b));
    T cmin = min(r, min(g, b));

    hsl._l = (cmax + cmin) / 2;

    if (cmax == cmin) {
      hsl._h = 0;
      hsl._s = 0;
    }
    else {
      T d = cmax - cmin;

      hsl._s = (hsl._l == 1) ? 0 : d / (1 - abs(2 * hsl._l - 1));

      if (cmax == r) {
        hsl._h = (T)fmod((g - b) / d, 6);
      }
      else if (cmax == g) {
        hsl._h = (b - r) / d + 2;
      }
      else if (cmax == b) {
        hsl._h = (r - g) / d + 4;
      }

      hsl._h *= 60;
    }

    return hsl;
  }

  template<>
  inline typename Utils<ExpStep>::HSLColorT Utils<ExpStep>::RGBToHSL(ExpStep r, ExpStep g, ExpStep b) {
    vector<ExpStep> res = r.context->callFunc("RGBToHSL", r, g, b);

    Utils<ExpStep>::HSLColorT c;
    c._h = res[0];
    c._s = res[1];
    c._l = res[2];

    return c;
  }

  template<typename T>
  inline typename Utils<T>::HSLColorT Utils<T>::RGBToHSL(RGBColorT & c)
  {
    return RGBToHSL(c._r, c._g, c._b);
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Utils<T>::HSLToRGB(T h, T s, T l)
  {
    RGBColorT rgb;

    // h needs to be positive
    while (h < 0) {
      h += 360;
    }

    // but not greater than 360 so
    h = (T)fmod(h, 360);
    s = (s > 1) ? 1 : (s < 0) ? 0 : s;
    l = (l > 1) ? 1 : (l < 0) ? 0 : l;

    T c = (1 - abs(2 * l - 1)) * s;
    T hp = h / 60;
    T x = c * (T)(1 - abs(fmod(hp, 2) - 1));

    if (0 <= hp && hp < 1) {
      rgb._r = c;
      rgb._g = x;
      rgb._b = 0;
    }
    else if (1 <= hp && hp < 2) {
      rgb._r = x;
      rgb._g = c;
      rgb._b = 0;
    }
    else if (2 <= hp && hp < 3) {
      rgb._r = 0;
      rgb._g = c;
      rgb._b = x;
    }
    else if (3 <= hp && hp < 4) {
      rgb._r = 0;
      rgb._g = x;
      rgb._b = c;
    }
    else if (4 <= hp && hp < 5) {
      rgb._r = x;
      rgb._g = 0;
      rgb._b = c;
    }
    else if (5 <= hp && hp < 6) {
      rgb._r = c;
      rgb._g = 0;
      rgb._b = x;
    }

    float m = l - 0.5f * c;

    rgb._r = rgb._r + m;
    rgb._g = rgb._g + m;
    rgb._b = rgb._b + m;

    return rgb;
  }

  template<>
  inline typename Utils<ExpStep>::RGBColorT Utils<ExpStep>::HSLToRGB(ExpStep h, ExpStep s, ExpStep l) {
    vector<ExpStep> res = h.context->callFunc("HSLToRGB", h, s, l);

    Utils<ExpStep>::RGBColorT c;
    c._r = res[0];
    c._g = res[1];
    c._b = res[2];

    return c;
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Utils<T>::HSLToRGB(HSLColorT& c)
  {
    return HSLToRGB(c._h, c._s, c._l);
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Utils<T>::HSYToRGB(T h, T s, T y)
  {
    RGBColorT rgb;

    // h needs to be positive
    while (h < 0) {
      h += 360;
    }

    h = (T)fmod(h, 360);
    s = (s > 1) ? 1 : (s < 0) ? 0 : s;
    y = (y > 1) ? 1 : (y < 0) ? 0 : y;

    T c = s;
    T hp = h / 60;
    T x = c * (T)(1 - abs(fmod(hp, 2) - 1));

    if (0 <= hp && hp < 1) {
      rgb._r = c;
      rgb._g = x;
      rgb._b = 0;
    }
    else if (1 <= hp && hp < 2) {
      rgb._r = x;
      rgb._g = c;
      rgb._b = 0;
    }
    else if (2 <= hp && hp < 3) {
      rgb._r = 0;
      rgb._g = c;
      rgb._b = x;
    }
    else if (3 <= hp && hp < 4) {
      rgb._r = 0;
      rgb._g = x;
      rgb._b = c;
    }
    else if (4 <= hp && hp < 5) {
      rgb._r = x;
      rgb._g = 0;
      rgb._b = c;
    }
    else if (5 <= hp && hp < 6) {
      rgb._r = c;
      rgb._g = 0;
      rgb._b = x;
    }

    float m = y - (.3f * rgb._r + .59f * rgb._g + 0.11f * rgb._b);

    rgb._r += m;
    rgb._g += m;
    rgb._b += m;

    return rgb;
  }

  template<>
  inline typename Utils<ExpStep>::RGBColorT Utils<ExpStep>::HSYToRGB(ExpStep h, ExpStep s, ExpStep y) {
    vector<ExpStep> res = h.context->callFunc("HSYToRGB", h, s, y);

    Utils<ExpStep>::RGBColorT c;
    c._r = res[0];
    c._g = res[1];
    c._b = res[2];

    return c;
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Utils<T>::HSYToRGB(HSYColorT & c)
  {
    return HSYToRGB(c._h, c._s, c._y);
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Utils<T>::CMYKToRGB(T c, T m, T y, T k)
  {
    RGBColorT rgb;

    rgb._r = (1 - c) * (1 - k);
    rgb._g = (1 - m) * (1 - k);
    rgb._b = (1 - y) * (1 - k);

    return rgb;
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Utils<T>::CMYKToRGB(CMYKColorT & c)
  {
    return CMYKToRGB(c._c, c._m, c._y, c._k);
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Utils<T>::LabToRGB(T L, T a, T b)
  {
    // convert to XYZ first, D50 is the Photoshop ref white
    // D50 = 96.4212, 100.0, 82.5188
    T e = 0.008856;
    T k = 903.3;

    T fy = (L + 16) / 116;
    T fx = a / 500 + fy;
    T fz = fy - b / 200;

    T fx3 = pow(fx, 3);
    T fz3 = pow(fz, 3);

    T xr = (fx3 > e) ? fx3 : (116 * fx - 16) / k;
    T yr = (L > k * e) ? pow((L + 16) / 116, 3) : L / k;
    T zr = (fz3 > e) ? fz3 : (116 * fz - 16) / k;

    T X = (xr * 96.4242) / 100.0f;
    T Y = (yr * 100.0) / 100.0f;
    T Z = (zr * 82.5188) / 100.0f;

    RGBColorT rgb;

    rgb._r = clamp<T>(rgbCompand(X *  3.2406 + Y * -1.5372 + Z * -0.4986), 0, 1);
    rgb._g = clamp<T>(rgbCompand(X * -0.9689 + Y *  1.8758 + Z *  0.0415), 0, 1);
    rgb._b = clamp<T>(rgbCompand(X *  0.0557 + Y * -0.2040 + Z *  1.0570), 0, 1);

    return rgb;
  }

  template<>
  inline typename Utils<ExpStep>::RGBColorT Utils<ExpStep>::LabToRGB(ExpStep L, ExpStep a, ExpStep b) {
    vector<ExpStep> res = L.context->callFunc("LabToRGB", L, a, b);

    Utils<ExpStep>::RGBColorT c;
    c._r = res[0];
    c._g = res[1];
    c._b = res[2];

    return c;
  }

  template<typename T>
  inline typename Utils<T>::HSYColorT Utils<T>::RGBToHSY(T r, T g, T b)
  {
    // basically this is the same as hsl except l is computed differently.
    // because nothing depends on L for this computation, we just abuse the
    // HSL conversion function and overwrite L
    HSLColorT c = RGBToHSL(r, g, b);
    HSYColorT c2;
    c2._h = c._h;
    c2._s = max(r, max(g, b)) - min(r, min(g, b));
    c2._y = 0.30f * r + 0.59f * g + 0.11f * b;

    return c2;
  }

  template <>
  inline typename Utils<ExpStep>::HSYColorT Utils<ExpStep>::RGBToHSY(ExpStep r, ExpStep g, ExpStep b)
  {
    vector<ExpStep> res = r.context->callFunc("RGBToHSY", r, g, b);

    Utils<ExpStep>::HSYColorT c;
    c._h = res[0];
    c._s = res[1];
    c._y = res[2];

    return c;
  }

  template<typename T>
  inline typename Utils<T>::HSYColorT Utils<T>::RGBToHSY(RGBColorT& c)
  {
    return RGBToHSY(c._r, c._g, c._b);
  }

  template<typename T>
  inline typename Utils<T>::CMYKColorT Utils<T>::RGBToCMYK(T r, T g, T b)
  {
    CMYKColorT c;
    c._k = 1 - max(r, max(g, b));

    if (c._k == 1) {
      c._m = 0;
      c._y = 0;
      c._c = 0;
      return c;
    }

    c._c = (1 - r - c._k) / (1 - c._k);
    c._m = (1 - g - c._k) / (1 - c._k);
    c._y = (1 - b - c._k) / (1 - c._k);

    return c;
  }

  template<>
  inline typename Utils<ExpStep>::CMYKColorT Utils<ExpStep>::RGBToCMYK(ExpStep r, ExpStep g, ExpStep b) {
    vector<ExpStep> res = r.context->callFunc("RGBToCMYK", r, g, b);

    Utils<ExpStep>::CMYKColorT c;
    c._c = res[0];
    c._m = res[1];
    c._y = res[2];
    c._k = res[4];

    return c;
  }

  template<typename T>
  inline typename Utils<T>::CMYKColorT Utils<T>::RGBToCMYK(RGBColorT& c)
  {
    return RGBToCMYK(c._r, c._g, c._b);
  }

  template<typename T>
  inline T Utils<T>::rgbCompand(T x)
  {
    if (x > 0.0031308)
      return 1.055 * pow(x, 1.0f / 2.4f) - 0.055;
    else
      return 12.92 * x;
  }

  template<>
  inline ExpStep Utils<ExpStep>::rgbCompand(ExpStep x) {
    vector<ExpStep> res = x.context->callFunc("rgbCompand", x);
    return res[0];
  }

  template<typename T>
  inline T Curve::eval(T x)
  {
    int k = -1;

    // sometimes there will be nothing at x = 0 so clamp to nearest
    if (x < _pts[0]._x) {
      return _pts[0]._y;
    }

    // determine interval
    for (int i = 0; i < _pts.size() - 1; i++) {
      if (x >= _pts[i]._x && x < _pts[i + 1]._x) {
        k = i;
        break;
      }
    }

    // if k wasn't ever assigned, clamp to last seen y
    if (k == -1) {
      return _pts[_pts.size() - 1]._y;
    }

    // compute t
    T t = (x - _pts[k]._x) / (_pts[k + 1]._x - _pts[k]._x);

    // compute interp basis
    T h00 = 2 * t * t * t - 3 * t * t + 1;
    T h10 = t * t * t - 2 * t * t + t;
    T h01 = -2 * t * t * t + 3 * t * t;
    T h11 = t * t * t - t * t;

    // compute p
    T p = h00 * _pts[k]._y + h10 * _m[k] * (_pts[k + 1]._x - _pts[k]._x) +
      h01 * _pts[k + 1]._y + h11 * _m[k + 1] * (_pts[k + 1]._x - _pts[k]._x);

    return p;
  }

  // specialization. Unsure how to handle uneven intervals without conditionals,
  // so this actually just returns x. (linear)
  template<>
  inline ExpStep Curve::eval(ExpStep x) {
    return x;
  }

  template<typename T>
  inline typename Utils<T>::RGBColorT Gradient::eval(T x)
  {
    if (_x.size() == 0)
      return Utils<T>::RGBColorT();

    int k = -1;

    // sometimes there will be nothing at x = 0 so clamp to nearest
    if (x < _x[0]) {
      RGBColor c = _colors[0];
      Utils<T>::RGBColorT ret;

      ret._r = c._r;
      ret._g = c._g;
      ret._b = c._b;
      return ret;
    }

    // determine interval
    for (int i = 0; i < _x.size() - 1; i++) {
      if (x >= _x[i] && x < _x[i + 1]) {
        k = i;
        break;
      }
    }

    // if k wasn't ever assigned, clamp to last seen y
    if (k == -1) {
      RGBColor c = _colors[_x.size() - 1];
      Utils<T>::RGBColorT ret;

      ret._r = c._r;
      ret._g = c._g;
      ret._b = c._b;
      return ret;
    }

    // linear interpolation
    T a = (x - _x[k]) / (_x[k + 1] - _x[k]);
    RGBColor c1 = _colors[k];
    RGBColor c2 = _colors[k + 1];

    Utils<T>::RGBColorT c;
    c._r = c1._r * (1 - a) + c2._r * a;
    c._g = c1._g * (1 - a) + c2._g * a;
    c._b = c1._b * (1 - a) + c2._b * a;

    return c;
  }

  //this also has a problem with uneven intervals, for now we'll interpolate between first
  // and last colors, as that should handle most gradients
  template<>
  inline typename Utils<ExpStep>::RGBColorT Gradient::eval<ExpStep>(ExpStep x) {
    // linear interpolation
    ExpStep a = (x - _x[0]) / (_x[_x.size() - 1] - _x[0]);
    RGBColor c1 = _colors[0];
    RGBColor c2 = _colors[_x.size() - 1];

    Utils<ExpStep>::RGBColorT c;
    c._r = c1._r * (1 - a) + c2._r * a;
    c._g = c1._g * (1 - a) + c2._g * a;
    c._b = c1._b * (1 - a) + c2._b * a;

    return c;
  }

  typedef Utils<float>::RGBAColorT RGBAColor;
  typedef Utils<float>::CMYKColorT CMYKColor;
  typedef Utils<float>::HSLColorT HSLColor;
  typedef Utils<float>::HSYColorT HSYColor;
}