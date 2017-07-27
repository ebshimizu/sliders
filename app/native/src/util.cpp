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
    // sometimes there might not be any parameter constraints, in this case the calling
    // function should be prepared to deal with 0 length param vectors.
    if (data.count("params") > 0) {
      nlohmann::json jsonParams = data["params"];
      params.clear();
      for (auto& p : jsonParams) {
        params.push_back(p["value"]);
      }
    }

    if (data.count("residuals") > 0) {
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
    }

    return data;
  }

  Stats::Stats(vector<double>& c1, vector<double>& c2, nlohmann::json & info)
  {
    computeStats(c1, c2, info);
  }

  Stats::~Stats() {

  }

  /*
  Saving for use later. this function doesn't work in utils due to Context not being defined as well
  as Layers not existing yet.

  void Stats::computeStats(Context& c1, Context& c2) {
    _paramChanges.clear();

    // at the basic level, this is just pairwise differences between parameter
    // values. Only differences above 0.01 are reported (1% changse are usually really not noticable)
    for (auto& l : c2) {
      string pfx = l.first + "_";

      // opacity
      float opdiff = l.second.getOpacity() - c1[l.first].getOpacity();

      if (abs(opdiff) > 0.01) {
        _paramChanges[pfx + "opacity"] = opdiff;
      }

      for (auto& a : l.second.getAdjustments()) {
        // adjustment differences
        for (auto& adj : l.second.getAdjustment(a)) {
          float adjDiff = adj.second - c1[l.first].getAdjustment(a)[adj.first];

          if (abs(adjDiff) > 0.01) {
            _paramChanges[pfx + adj.first] = adjDiff;
          }
        }
      }

      // selective color
      vector<string> channels = { "reds", "yellows", "greens", "cyans", "blues", "magentas", "neutrals", "blacks", "whites" };
      vector<string> params = { "cyan", "magenta", "yellow", "black" };

      for (auto c : channels) {
        for (auto p : params) {
          float adjDiff = l.second.getSelectiveColorChannel(c, p) - c1[l.first].getSelectiveColorChannel(c, p);

          if (abs(adjDiff) > 0.01) {
            _paramChanges[pfx + "sc_" + c + "_" + p] = adjDiff;
          }
        }
      }
    }
  }
  */

  void Stats::computeStats(vector<double>& c1, vector<double>& c2, nlohmann::json & info)
  {
    // this function extracts some (hopefully meaningful) stats about what happened during
    // an optimization pass. The json object is the configuration sent to ceres and
    // is used to match the values in the vectors with semantically meaningful values
    nlohmann::json paramInfo = info["params"];

    for (int i = 0; i < c1.size(); i++) {
      float diff = c2[i] - c1[i];

      if (abs(diff) > 0.01) {
        string name = paramInfo[i]["layerName"].get<string>() + "_" + paramInfo[i]["adjustmentName"].get<string>();

        if (paramInfo[i]["adjustmentType"].get<int>() == AdjustmentType::SELECTIVE_COLOR) {
          name += paramInfo[i]["selectiveColor"]["channel"].get<string>() + "_" + paramInfo[i]["selectiveColor"]["color"].get<string>();
        }

        _paramChanges[name] = diff;
      }
    }
  }

  string Stats::detailedSummary() {
    string ret = "";

    if (_paramChanges.size() > 0) {
      for (auto& param : _paramChanges) {
        ret += param.first + ":" + ((param.second > 0) ? " +" : " ") + to_string(param.second) + "\n";
      }
    }
    else {
      ret = "No Changes";
    }

    return ret;
  }

}