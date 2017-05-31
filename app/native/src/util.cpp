#include "util.h"

namespace Comp {
  template<typename T>
  inline Utils<T>::HSLColorT Utils<T>::RGBToHSL(T r, T g, T b)
  {
    HSLColorT hsl;

    float cmax = max(r, max(g, b));
    float cmin = min(r, min(g, b));

    hsl._l = (cmax + cmin) / 2;

    if (cmax == cmin) {
      hsl._h = 0;
      hsl._s = 0;
    }
    else {
      float d = cmax - cmin;

      hsl._s = (hsl._l == 1) ? 0 : d / (1 - abs(2 * hsl._l - 1));

      if (cmax == r) {
        hsl._h = (float) fmod((g - b) / d, 6);
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

  template<typename T>
  inline Utils<T>::HSLColorT Utils<T>::RGBToHSL(RGBColorT & c)
  {
    return RGBToHSL(c._r, c._g, c._b);
  }

  template<typaname T>
  inline Utils<T>::RGBColorT Utils<T>::HSLToRGB(float h, float s, float l)
  {
    RGBColorT rgb;

    // h needs to be positive
    while (h < 0) {
      h += 360;
    }

    // but not greater than 360 so
    h = (float) fmod(h, 360);
    s = (s > 1) ? 1 : (s < 0) ? 0 : s;
    l = (l > 1) ? 1 : (l < 0) ? 0 : l;

    float c = (1 - abs(2 * l - 1)) * s;
    float hp = h / 60;
    float x = c * (float)(1 - abs(fmod(hp, 2) - 1));

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

  RGBColor HSLToRGB(HSLColor & c)
  {
    return HSLToRGB(c._h, c._s, c._l);
  }

  RGBColor HSYToRGB(float h, float s, float y)
  {
    RGBColor rgb;

    // h needs to be positive
    while (h < 0) {
      h += 360;
    }

    h = (float)fmod(h, 360);
    s = (s > 1) ? 1 : (s < 0) ? 0 : s;
    y = (y > 1) ? 1 : (y < 0) ? 0 : y;

    float c = s;
    float hp = h / 60;
    float x = c * (float)(1 - abs(fmod(hp, 2) - 1));

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

  RGBColor HSYToRGB(HSYColor & c)
  {
    return HSYToRGB(c._h, c._s, c._y);
  }

  RGBColor CMYKToRGB(float c, float m, float y, float k)
  {
    RGBColor rgb;

    rgb._r = (1 - c) * (1 - k);
    rgb._g = (1 - m) * (1 - k);
    rgb._b = (1 - y) * (1 - k);

    return rgb;
  }

  RGBColor CMYKToRGB(CMYKColor & c)
  {
    return CMYKToRGB(c._c, c._m, c._y, c._k);
  }

  RGBColor LabToRGB(float L, float a, float b)
  {
    // convert to XYZ first, D50 is the Photoshop ref white
    // D50 = 96.4212, 100.0, 82.5188
    float e = 0.008856;
    float k = 903.3;

    float fy = (L + 16) / 116;
    float fx = a / 500 + fy;
    float fz = fy - b / 200;

    float fx3 = pow(fx, 3);
    float fz3 = pow(fz, 3);

    float xr = (fx3 > e) ? fx3 : (116 * fx - 16) / k;
    float yr = (L > k * e) ? pow((L + 16) / 116, 3) : L / k;
    float zr = (fz3 > e) ? fz3 : (116 * fz - 16) / k;

    float X = (xr * 96.4242) / 100.0f;
    float Y = (yr * 100.0) / 100.0f;
    float Z = (zr * 82.5188) / 100.0f;

    RGBColor rgb;

    rgb._r = clamp(rgbCompand(X *  3.2406 + Y * -1.5372 + Z * -0.4986), 0, 1);
    rgb._g = clamp(rgbCompand(X * -0.9689 + Y *  1.8758 + Z *  0.0415), 0, 1);
    rgb._b = clamp(rgbCompand(X *  0.0557 + Y * -0.2040 + Z *  1.0570), 0, 1);

    return rgb;
  }

  template<typename T>
  Utils<T>::HSYColorT Utils<T>::RGBToHSY(T r, T g, T b)
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

  template<typename T>
  Utils<T>::HSYColorT Utils<T>::RGBToHSY(RGBColorT& c)
  {
    return RGBToHSY(c._r, c._g, c._b);
  }

  CMYKColor RGBToCMYK(float r, float g, float b)
  {
    CMYKColor c;
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

  CMYKColor RGBToCMYK(RGBColor & c)
  {
    return RGBToCMYK(c._r, c._g, c._b);
  }

  float rgbCompand(float x)
  {
    if (x > 0.0031308)
      return 1.055 * pow(x, 1.0f / 2.4f) - 0.055;
    else
      return 12.92 * x;
  }

  float clamp(float val, float mn, float mx)
  {
    return (val > mx) ? mx : ((val < mn) ? mn : val);
  }

  Curve::Curve(vector<Point> pts) : _pts(pts)
  {
    computeTangents();
  }

  void Curve::computeTangents()
  {
    _m.clear();
    _m.resize(_pts.size());

    // catmull-rom spline tangents
    for (int i = 0; i < _pts.size(); i++) {
      // edge cases
      if (i == 0) {
        _m[i] = (_pts[i + 1]._y - _pts[i]._y) / (_pts[i + 1]._x - _pts[i]._x);
      }
      else if (i == _pts.size() - 1) {
        _m[i] = (_pts[i]._y - _pts[i - 1]._y) / (_pts[i]._x - _pts[i - 1]._x);
      }
      // general case
      else {
        _m[i] = ((_pts[i + 1]._y - _pts[i - 1]._y) / (_pts[i + 1]._x - _pts[i - 1]._x));
      }
    }
  }

  float Curve::eval(float x)
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
    float t = (x - _pts[k]._x) / (_pts[k + 1]._x - _pts[k]._x);

    // compute interp basis
    float h00 = 2 * t * t * t - 3 * t * t + 1;
    float h10 = t * t * t - 2 * t * t + t;
    float h01 = -2 * t * t * t + 3 * t * t;
    float h11 = t * t * t - t * t;

    // compute p
    float p = h00 * _pts[k]._y + h10 * _m[k] * (_pts[k + 1]._x - _pts[k]._x) +
              h01 * _pts[k + 1]._y + h11 * _m[k + 1] * (_pts[k + 1]._x - _pts[k]._x);

    return p;
  }

  RGBColor Gradient::eval(float x)
  {
    if (_x.size() == 0)
      return RGBColor();

    int k = -1;

    // sometimes there will be nothing at x = 0 so clamp to nearest
    if (x < _x[0]) {
      return _colors[0];
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
      return _colors[_x.size() - 1];
    }

    // linear interpolation
    float a = (x - _x[k]) / (_x[k + 1] - _x[k]);
    RGBColor c1 = _colors[k];
    RGBColor c2 = _colors[k + 1];

    RGBColor c;
    c._r = c1._r * (1 - a) + c2._r * a;
    c._g = c1._g * (1 - a) + c2._g * a;
    c._b = c1._b * (1 - a) + c2._b * a;

    return c;
  }
}