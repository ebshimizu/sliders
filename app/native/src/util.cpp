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

  nlohmann::json loadCeresData(string file, vector<double>& params, vector<vector<double>>& layerPixels,
    vector<vector<double>>& targets, vector<double>& weights)
  {
    nlohmann::json data;

    ifstream input(file);
    input >> data;

    // here's what we need: parameters in proper order with initial values,
    // layer pixels in a flat vector, target colors (also flat), and weights
    nlohmann::json jsonParams = data["params"];
    params.clear();
    for (auto& p : jsonParams) {
      params.push_back(p["value"]);
    }

    // layer stuff
    nlohmann::json res = data["residuals"];
    layerPixels.clear();
    targets.clear();
    weights.clear();

    for (auto r : res) {
      vector<double> pixelVals;
      for (auto l : r["layers"]) {
        pixelVals.push_back(l["r"].get<double>());
        pixelVals.push_back(l["g"].get<double>());
        pixelVals.push_back(l["b"].get<double>());
        pixelVals.push_back(l["a"].get<double>());
      }
      layerPixels.push_back(pixelVals);

      vector<double> target;
      target.push_back(r["target"]["r"].get<double>());
      target.push_back(r["target"]["g"].get<double>());
      target.push_back(r["target"]["b"].get<double>());
      targets.push_back(target);

      weights.push_back(r["weight"]);
    }

    return data;
  }
}