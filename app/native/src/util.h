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
#include "third_party/json/src/json.hpp"

using namespace std;

#include "../../../expression_tree/expressionContext.h"

namespace Comp {
  const string version = "0.1";

  enum ParamType {
    FREE_PARAM = 0,
    LAYER_PIXEL = 1
  };

  enum AdjustmentType {
    HSL = 0,              // expected params: hue [-180, 180], sat [-100, 100], light [-100, 100]
    LEVELS = 1,           // expected params: inMin, inMax, gamma, outMin, outMax (all optional, [0-255])
    CURVES = 2,           // Curves need more data, so the default map just contains a list of present channels
    EXPOSURE = 3,         // params: exposure [-20, 20], offset [-0.5, 0.5], gamma [0.01, 9.99]
    GRADIENT = 4,         // params: a gradient
    SELECTIVE_COLOR = 5,  // params: a lot 9 channels: RYGCBMLNK each with 4 params CMYK
    COLOR_BALANCE = 6,    // params: RGB shift for dark, mid, light tones (9 total)
    PHOTO_FILTER = 7,     // params: Lab color, density
    COLORIZE = 8,         // special case of a particular action output. Colors a layer based on specified color and alpha like the COLOR blend mode
    LIGHTER_COLORIZE = 9, // also a special case like colorize but this does the Lighter Color blend mode
    OVERWRITE_COLOR = 10, // also a special case. literally just replaces the layer with this solid color
    INVERT = 11,          // inverts the current composition, no parameters, key just needs to exist
    BRIGHTNESS = 12,      // also contrast. params: brightness, contrast, both [-1, 1]
    OPACITY = 1000        // special category for indicating layer opacity param, used in data transfer
  };

  static map<int, string> intervalNames = { {0, "reds"} , {1, "yellows" }, {2, "greens" }, {3, "cyans"}, {4, "blues" }, {5, "magentas" } };

  // statistics about the difference between two different contexts.
  class Stats {
  public:
    Stats(vector<double>& c1, vector<double>& c2, nlohmann::json& info);
    ~Stats();

    map<string, float> _paramChanges;
    map<string, float> _summary;    // unsure of type for this

    void computeStats(vector<double>& c1, vector<double>& c2, nlohmann::json& info);

    // returns a string version of the values stored in _paramChanges
    string detailedSummary();
  };

  template <typename T>
  inline T clamp(T val, T mn, T mx) {
    return (val > mx) ? mx : ((val < mn) ? mn : val);
  }

  template <>
  inline ExpStep clamp(ExpStep val, ExpStep mn, ExpStep mx) {
    vector<ExpStep> res = val.context->callFunc("clamp", val, mn, mx);
    return res[0];
  }

  template <typename T>
  inline T fmodt(T val, T mod) {
    T div = floor(val / mod);
    T rem = val - (mod * div);
    return rem;
  }

  template <typename T>
  class PointT {
  public:
    PointT(T x, T y) : _x(x), _y(y) {}
    PointT() : _x(0), _y(0) {}

    float _x;
    float _y;
  };

  typedef PointT<float> Point;
  typedef PointT<int> Pointi;

  // loads data from a json file into the proper ceres formats, then returns the json object
  nlohmann::json loadCeresData(string file, vector<double>& params, vector<vector<double> >& layerPixels,
    vector<vector<double> >& targets, vector<double>& weights);

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

  // class that stores a vector but allows access as a grid
  template<typename T>
  class Grid2D {
  public:
    Grid2D(int width, int height);
    ~Grid2D();

    T& operator()(int x, int y);

    void clear(T val);

    int width();
    int height();
    size_t size();

    string toString();
  private:
    vector<T> _data;

    int _width;
    int _height;
  };

  template<typename T>
  inline Grid2D<T>::Grid2D(int width, int height) : _width(width), _height(height)
  {
    _data.resize(width * height);
  }

  template<typename T>
  inline Grid2D<T>::~Grid2D()
  {
  }

  template<typename T>
  inline T & Grid2D<T>::operator()(int x, int y)
  {
    int flat = x + y * _width;

    if (flat > _data.size()) {
      throw exception("Index out of range in Grid2D");
    }

    return _data[x + y * _width];
  }

  template<typename T>
  inline void Grid2D<T>::clear(T val)
  {
    for (int i = 0; i < _data.size(); i++)
      _data[i] = val;
  }

  template<typename T>
  inline int Grid2D<T>::width()
  {
    return _width;
  }

  template<typename T>
  inline int Grid2D<T>::height()
  {
    return _height;
  }

  template<typename T>
  inline size_t Grid2D<T>::size()
  {
    return _data.size();
  }

  template<typename T>
  inline string Grid2D<T>::toString()
  {
    stringstream ss;
    ss << "Dumping Grid w: " << _width << " h: " << _height << "\n";

    for (int y = 0; y < _height; y++) {
      ss << "Row " << y << ":\t";
      for (int x = 0; x < _width; x++) {
        if (x != 0)
          ss << ", ";
        
        ss << this->operator()(x, y);
      }
      ss << "\n";
    }

    return ss.str();
  }

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

    struct LabColorT {
      T _L;
      T _a;
      T _b;
    };

    static T RGBL2Diff(RGBColorT& x, RGBColorT& y);
    static T RGBAL2Diff(RGBAColorT& x, RGBAColorT& y);

    static T LabL2Diff(RGBAColorT& x, RGBAColorT& y);
    static T LabL2Diff(LabColorT& x, LabColorT& y);

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

    static LabColorT RGBToLab(T r, T g, T b);
    static LabColorT RGBToLab(RGBColorT& c);
    static LabColorT RGBAToLab(RGBAColorT& c);

    static T rgbCompand(T x);
    static T inverseRGBCompand(T x);
  };

  typedef Utils<float>::RGBColorT RGBColor;
  typedef Utils<float>::LabColorT LabColor;

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
  inline T Utils<T>::RGBL2Diff(RGBColorT & x, RGBColorT & y)
  {
    return sqrt((x._r - y._r) * (x._r - y._r) + (x._g - y._g) * (x._g - y._g) + (x._b - y._b) * (x._b - y._b));
  }

  template<typename T>
  inline T Utils<T>::RGBAL2Diff(RGBAColorT & x, RGBAColorT & y)
  {
    // premult then normal L2
    float rd = (x._r * x._a) - (y._r * y._a);
    float gd = (x._g * x._a) - (y._g * y._a);
    float bd = (x._b * x._a) - (y._b * y._a);

    return sqrt(rd * rd + gd * gd + bd * bd);
  }

  template<typename T>
  inline T Utils<T>::LabL2Diff(RGBAColorT & x, RGBAColorT & y)
  {
    return Utils<T>::LabL2Diff(Utils<T>::RGBAToLab(x), Utils<T>::RGBAToLab(y));
  }

  template<typename T>
  inline T Utils<T>::LabL2Diff(LabColorT & x, LabColorT & y)
  {
    float ld = x._L - y._L;
    float ad = x._a - y._a;
    float bd = x._b - y._b;

    return sqrt(ld * ld + ad * ad + bd * bd);
  }

  template<typename T>
  inline typename Utils<T>::HSLColorT Utils<T>::RGBToHSL(T r, T g, T b)
  {
    HSLColorT hsl;

    T cmax = max(r, max(g, b));
    T cmin = min(r, min(g, b));

    hsl._l = (cmax + cmin) / (T)2;

    if (cmax == cmin) {
      hsl._h = (T)0;
      hsl._s = (T)0;
    }
    else {
      T d = cmax - cmin;

      hsl._s = (hsl._l == (T)1) ? (T)0 : d / ((T)1 - abs((T)2 * hsl._l - (T)1));

      if (cmax == r) {
        hsl._h = (T)fmodt((g - b) / d, (T)6);
      }
      else if (cmax == g) {
        hsl._h = (b - r) / d + (T)2;
      }
      else if (cmax == b) {
        hsl._h = (r - g) / d + (T)4;
      }

      hsl._h *= (T)60;
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
    while (h < (T)0) {
      h += (T)360;
    }

    // but not greater than 360 so
    h = (T)fmodt(h, (T)360);
    s = (s > (T)1) ? (T)1 : (s < (T)0) ? (T)0 : s;
    l = (l > (T)1) ? (T)1 : (l < (T)0) ? (T)0 : l;

    T c = ((T)1 - abs((T)2 * l - (T)1)) * s;
    T hp = h / (T)60;
    T x = c * (T)((T)1 - abs(fmodt(hp, (T)2) - (T)1));

    if ((T)0 <= hp && hp < (T)1) {
      rgb._r = c;
      rgb._g = x;
      rgb._b = (T)0;
    }
    else if ((T)1 <= hp && hp < (T)2) {
      rgb._r = x;
      rgb._g = c;
      rgb._b = (T)0;
    }
    else if ((T)2 <= hp && hp < (T)3) {
      rgb._r = (T)0;
      rgb._g = c;
      rgb._b = x;
    }
    else if ((T)3 <= hp && hp < (T)4) {
      rgb._r = (T)0;
      rgb._g = x;
      rgb._b = c;
    }
    else if ((T)4 <= hp && hp < (T)5) {
      rgb._r = x;
      rgb._g = (T)0;
      rgb._b = c;
    }
    else if ((T)5 <= hp && hp < (T)6) {
      rgb._r = c;
      rgb._g = (T)0;
      rgb._b = x;
    }

    T m = l - (T)0.5 * c;

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
    while (h < (T)0) {
      h += (T)360;
    }

    h = (T)fmodt(h, (T)360);
    s = (s >(T)1) ? (T)1 : (s < (T)0) ? (T)0 : s;
    y = (y >(T)1) ? (T)1 : (y < (T)0) ? (T)0 : y;

    T c = s;
    T hp = h / (T)60;
    T x = c * (T)((T)1 - abs(fmodt(hp, (T)2) - (T)1));

    if ((T)0 <= hp && hp < (T)1) {
      rgb._r = c;
      rgb._g = x;
      rgb._b = (T)0;
    }
    else if ((T)1 <= hp && hp < (T)2) {
      rgb._r = x;
      rgb._g = c;
      rgb._b = (T)0;
    }
    else if ((T)2 <= hp && hp < (T)3) {
      rgb._r = (T)0;
      rgb._g = c;
      rgb._b = x;
    }
    else if ((T)3 <= hp && hp < (T)4) {
      rgb._r = (T)0;
      rgb._g = x;
      rgb._b = c;
    }
    else if ((T)4 <= hp && hp < (T)5) {
      rgb._r = x;
      rgb._g = (T)0;
      rgb._b = c;
    }
    else if ((T)5 <= hp && hp < (T)6) {
      rgb._r = c;
      rgb._g = (T)0;
      rgb._b = x;
    }

    T m = y - ((T).3 * rgb._r + (T).59 * rgb._g + (T)0.11 * rgb._b);

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

    rgb._r = ((T)1 - c) * ((T)1 - k);
    rgb._g = ((T)1 - m) * ((T)1 - k);
    rgb._b = ((T)1 - y) * ((T)1 - k);

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
    c2._y = (T)0.30 * r + (T)0.59 * g + (T)0.11 * b;

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
    c._k = (T)1 - max(r, max(g, b));

    if (c._k == (T)1) {
      c._m = (T)0;
      c._y = (T)0;
      c._c = (T)0;
      return c;
    }

    c._c = ((T)1 - r - c._k) / ((T)1 - c._k);
    c._m = ((T)1 - g - c._k) / ((T)1 - c._k);
    c._y = ((T)1 - b - c._k) / ((T)1 - c._k);

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
  inline typename Utils<T>::LabColorT Utils<T>::RGBToLab(T r, T g, T b)
  {
    T rc = inverseRGBCompand(r);
    T gc = inverseRGBCompand(g);
    T bc = inverseRGBCompand(b);

    // assuming sRGB color space with adapted D50 reference white (Lab is D50)
    // manual matrix multiplication
    T X = (T)0.4360747 * rc + (T)0.3850649 * gc + (T)0.1430804 * bc;
    T Y = (T)0.2225045 * rc + (T)0.7168786 * gc + (T)0.0606169 * bc;
    T Z = (T)0.0139322 * rc + (T)0.0971045 * gc + (T)0.7141733 * bc;

    // ref white: D50 = 96.4212, 100.0, 82.5188
    T e = (T)216 / (T)24389;
    T k = (T)24389 / (T)27;

    T xr = X / (T)0.964212;
    T yr = Y / (T)1;
    T zr = Z / (T)0.825188;

    T fx = (xr > e) ? pow(xr, (T)(1.0 / 3.0)) : (k * xr + (T)16) / (T)116;
    T fy = (yr > e) ? pow(yr, (T)(1.0 / 3.0)) : (k * yr + (T)16) / (T)116;
    T fz = (zr > e) ? pow(zr, (T)(1.0 / 3.0)) : (k * zr + (T)16) / (T)116;

    Utils<T>::LabColorT c;
    c._L = (T)116 * fy - (T)16;
    c._a = (T)500 * (fx - fy);
    c._b = (T)200 * (fy - fz);

    return c;
  }

  template<typename T>
  inline typename Utils<T>::LabColorT Utils<T>::RGBToLab(RGBColorT& c)
  {
    return RGBToLab(c._r, c._g, c._b);
  }

  template<typename T>
  inline typename Utils<T>::LabColorT Utils<T>::RGBAToLab(RGBAColorT & c)
  {
    return RGBToLab(c._r * c._a, c._g * c._a, c._b * c._a);
  }

  template<typename T>
  inline T Utils<T>::rgbCompand(T x)
  {
    if (x > 0.0031308)
      return 1.055 * pow(x, 1.0f / 2.4f) - 0.055;
    else
      return 12.92 * x;
  }

  template<typename T>
  inline T Utils<T>::inverseRGBCompand(T x)
  {
    if (x <= 0.04045)
      return x / 12.92;
    else
      return pow((x + 0.055) / 1.055, 2.4);
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