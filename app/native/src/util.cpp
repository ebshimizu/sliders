#include "util.h"

namespace Comp {
  Curve::Curve(vector<Point> pts) : _pts(pts)
  {
    computeTangents();
  }

  inline void Curve::computeTangents()
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
}