#pragma once

#include "util.h"
#include "Compositor.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Comp {

class LayerParamInfo {
public:
  LayerParamInfo() : _name(""), _param("") {}
  LayerParamInfo(AdjustmentType type, string name, string param);

  void addSample(double val);
  int count();

  string _name;
  string _param;
  AdjustmentType _type;
  vector<double> _vals;
  double _min;
  double _max;
};

// structure for containing info about things in the model
class ModelInfo {
public:
  ModelInfo() : _name("") {}
  ModelInfo(string name);
  ~ModelInfo();

  int count();
  void addVal(AdjustmentType t, string name, string param, double val);

  // removes parameters that only ever have one value (indicates no change/irrelevant)
  void cleanup();

  // returns a vector of only the parameters involved in this axis
  Eigen::VectorXf contextToAxisVector(Context& c);

  string _name;
  map<string, LayerParamInfo> _activeParams;
};

// the model class creates and samples from a model defined by a series of examples
// what this means specifically is yet to be determined
class Model {
public:
  // creates an empty model
  Model(Compositor* c);

  // model deleter
  ~Model();

  // runs an analysis and stores data about how to sample the space
  void analyze(map<string, vector<string>> examples);
  void analyze(map<string, vector<Context>> examples);

  // sampling function to get things out of the model
  Context sample();
  Context nonParametricLocalSample(Context x0);

  const map<string, ModelInfo>& getModelInfo();

private:
  // generates data structures for some non-parametric search methods
  // based off Exploratory Modeling with Collaborative Design Spaces (Talton et al.)
  void nonParametricAnalysis();

  float gaussianKernel(Eigen::VectorXf& x, Eigen::VectorXf& xi, Eigen::MatrixXf& sigmai);
  Eigen::MatrixXf computeBandwidthMatrix(Eigen::VectorXf& x, vector<Eigen::VectorXf>& pts, float alpha = 1);
  Eigen::VectorXf knn(Eigen::VectorXf& x, vector<Eigen::VectorXf>& pts, int k);

  // parent composition
  Compositor* _comp;

  // training set contexts
  map<string, vector<Context>> _train;

  // general info about the input data
  map<string, ModelInfo> _trainInfo;

  // non-parametric cache data
  map<string, vector<Eigen::VectorXf>> _axisVectorCache;
};

}