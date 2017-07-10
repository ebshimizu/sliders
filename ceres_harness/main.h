#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <cassert>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <random>

#include "ceres/ceres.h"
#include "glog/logging.h"

using ceres::DynamicAutoDiffCostFunction;
using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;
using namespace std;

#include "ceresFunctions.h"

using namespace Comp;
#include <ceresFunc.h>

class App
{
public:
  // available modes:
  // - single (runs optimizer exactly once)
  // - jitter (random jitter around found solutions)
  // - eval (evaluates the given scene and returns the same scene with a score)
  // - paramStats (parameter statitics computation)
  // - minimaStats (parameter statistics computation around the first found local minima)
  // - paramShiftTest (statistics computed by adjusting groups of parameters to see if adjusting a certain number is better)
  // - random (randomly samples the space of parameters and tracks the frequency of each minima encountered.
  void go(string mode, string loadFrom, string saveTo);

  void setupOptimizer(string loadFrom, string saveTo);
  double runOptimizerOnce();

  // evaluates the problem objective function once and outputs a file
  // containing the same input scene with a score field
  double eval();

  void exportSolution(string filename);

  // runs the optimization multiple times, tracking solution quality and randomizing
  // parameters to try to find better solutions
  void randomReinit();

  // computes some statistics for each parameter around the _initial configuration_ of the solver
  nlohmann::json exportParamStats(string filename);

  // computes some statistics for randomized values around the given local minima (assumed to be the state of _allParams)
  void exportMinimaStats(string filename);

  // outputs some stats about how effective adjusting groups of parameters is to escape local minima
  void paramShiftTest();

  // outputs some stats about of effective adjusting adjustments is to escape local minima
  void adjShiftTest();

  // randomizes the parameters and tracks which minima are found and how often
  // Intended to give a sense of how common various solutions are and how rare
  // stumbling into the proper location is
  void randomize();

  // levels has a requirement that min < max, figure out if we have any here
  void computeLtConstraints();

  // looks at the json data and returns groups of up to 4 parameters that make up
  // an "adjustment group". Typically these are the entire adjustment, but some
  // such as selective color have too many to keep as one group and are broken up
  // into multiple sets
  vector<vector<int> > getAdjGroups();

  // helper for stats
  void computePopStats(vector<double>& vals, double& sum, double& mean, double& var, double& stdDev);

  // l2 distance between two vectors
  double l2vector(vector<double>& x1, vector<double>& x2);

  // Average Lab L2 distance between two rendered images (or at least, the pixels of the image we're looking 
  // at in this program)
  double pixelDist(vector<double>& x1, vector<double>& x2);

  vector<double> _allParams;
  Problem _problem;
  nlohmann::json _data;
  string _outDir;

  // indicates that first should always be less than second
  map<int, int> _ltConstraints;

  // cached layer pixel values for computing a perceptual distance function
  vector<vector<double> > _layerValues;
};

template<class T>
struct cRGBColorT
{
  cRGBColorT() {}
  cRGBColorT(T _x, T _y, T _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }
  T sqMagintude()
  {
    return x * x + y * y + z * z;
  }
  cRGBColorT operator * (T v)
  {
    return RGBColorT(v * x, v * y, v * z);
  }
  cRGBColorT operator + (const cRGBColorT &v)
  {
    return cRGBColorT(x + v.x, y + v.y, z + v.z);
  }
  const T& operator [](int k) const
  {
    if (k == 0) return x;
    if (k == 1) return y;
    if (k == 2) return z;
    return x;
  }
  T x, y, z;
};

typedef cRGBColorT<double> cRGBColor;

template<class T>
vector<T> evalLayerColor(const T* const params, const vector<double> &layerValues)
{
  return ceresFunc<T>(params, layerValues);
}

struct CostTerm
{
  static const int ResidualCount = 3;

  CostTerm(const vector<double> &_layerValues, float _weight, const cRGBColor &_targetColor)
    : layerValues(_layerValues), weight(_weight), targetColor(_targetColor) {
    targetLabColor = Utils<float>::RGBToLab(targetColor.x, targetColor.y, targetColor.z);
  }

  template <typename T>
  bool operator()(const T* const params, T* residuals) const
  {
    // color is RGBA, right now compare vs premult RGB
    vector<T> color = evalLayerColor(params, layerValues);
    //residuals[0] = (color[0] * color[3] - T(targetColor.x)) * (T)weight;
    //residuals[1] = (color[1] * color[3] - T(targetColor.y)) * (T)weight;
    //residuals[2] = (color[2] * color[3] - T(targetColor.z)) * (T)weight;

    // lab space, CIE76
    Utils<T>::LabColorT labColor = Utils<T>::RGBToLab(color[0] * color[3], color[1] * color[3], color[2] * color[3]);
    residuals[0] = (labColor._L - (T)targetLabColor._L) * (T)weight;
    residuals[1] = (labColor._a - (T)targetLabColor._a) * (T)weight;
    residuals[2] = (labColor._b - (T)targetLabColor._b) * (T)weight;

    return true;
  }

  static ceres::CostFunction* Create(const vector<double> &layerValues, float weight, const cRGBColor &targetColor)
  {
    return (new ceres::AutoDiffCostFunction<CostTerm, CostTerm::ResidualCount, ceresFunc_paramACount>(
      new CostTerm(layerValues, weight, targetColor)));
  }

  vector<double> layerValues;
  cRGBColor targetColor;
  float weight;
  Utils<float>::LabColorT targetLabColor;
};
