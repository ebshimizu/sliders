#pragma once

#include "util.h"
#include "Compositor.h"

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

  string _name;
  map<string, map<string, LayerParamInfo>> _activeParams;
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
  void analyze(map<string, vector<string>> examples, string base);
  void analyze(map<string, vector<Context>> examples, Context base);

  // sampling function to get things out of the model
  Context sample();

  const map<string, ModelInfo>& getModelInfo();

private:
  // parent composition
  Compositor* _comp;

  map<string, Context> _train;

  // general info about the input data
  map<string, ModelInfo> _trainInfo;
};

}