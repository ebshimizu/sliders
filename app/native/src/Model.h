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
  bool _inverted;
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

  // fills in the parameters to the given context
  void axisVectorToContext(Eigen::VectorXf& x, Context& c);

  string _name;
  map<string, LayerParamInfo> _activeParams;
};

enum AxisEvalFuncType {
  AVERAGE,
  SINGLE_PARAM_MAX,
  SINGLE_PARAM_MIN,
  MEDIAN
};

enum AxisConstraintMode {
  TARGET_RANGE,   // target axis should be in range
  CONSTANT_VALUE  // a specific parameter should have a specific value
};

// depending on the mode, not all of these fields will be used
struct AxisConstraint {
  AxisConstraintMode _mode;
  string _axis;
  float _min;
  float _max;

  string _layer;
  AdjustmentType _type;
  string _param;
  float _val;
  float _tolerance;
};

class AxisDef {
public:
  AxisDef(string name, vector<LayerParamInfo> params, AxisEvalFuncType evalMode);
  ~AxisDef();

  float eval(Context& ctx);

  // samples the axis into the given context
  Context sample(Context ctx, const vector<AxisConstraint>& constraints);

  string _name;

private:
  // takes the average of the parameters active in the axis
  float evalAvg(Context& ctx);

  vector<LayerParamInfo> _params;
  AxisEvalFuncType _evalFuncMode;
};

// The schema is an explicit definition of the possible axes inherent in an action
class Schema {
public:
  // creates an empty schema
  Schema();

  // loads a schema from file
  Schema(string filename);

  Context sample(const Context& in, vector<AxisConstraint>& constraints);
  
  // evaluates the current context along each axis
  map<string, float> axisEval(Context& in);

private:
  void loadFromJson(nlohmann::json data);

  // returns true if all constraints are satisfied
  bool verifyConstraints(Context& ctx, const vector<AxisConstraint>& constraints);

  vector<AxisDef> _axes;
};

class Slider {
public:
  Slider();
  Slider(vector<LayerParamInfo> params);
  Slider(vector<LayerParamInfo> params, vector<int> order);

  Context sample(const Context& in, float val);

  // adds a new parameter. By default this is placed at the end of the order
  void addParameter(LayerParamInfo param);
  void replaceParameters(vector<LayerParamInfo> params);

  // re-orders the parameters
  void setOrder(vector<int> order);

  // deletes everything
  void removeAllParameters();

  string _name;

private:
  // in the slider class, the order vector determines how the layers
  // are composited/adjusted in the slider (for now)
  vector<LayerParamInfo> _params;
  vector<int> _order;
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

  // adds a schema to the model
  void addSchema(string file);
  map<string, float> schemaEval(Context& c);

  // sampling function to get things out of the model
  Context sample();
  Context nonParametricLocalSample(map<string, Context> axes, float alpha = 1, int k = -1);
  Context nonParametricLocalSample(Context init, Context x0, string axis, float alpha = 1, int k = -1);
  Context schemaSample(Context x0, vector<AxisConstraint>& constraints);

  const map<string, ModelInfo>& getModelInfo();
  map<string, vector<Context>> getInputData();

  // just defer all slider ops to the actual object for now
  Slider& getSlider(string name);
  void addSlider(string name, Slider s);
  void deleteSlider(string name);
  vector<string> getSliderNames();

private:
  // generates data structures for some non-parametric search methods
  // based off Exploratory Modeling with Collaborative Design Spaces (Talton et al.)
  float gaussianKernel(Eigen::VectorXf& x, Eigen::VectorXf& xi, Eigen::MatrixXf& sigmai);
  Eigen::MatrixXf computeBandwidthMatrix(Eigen::VectorXf& x, vector<Eigen::VectorXf>& pts, float alpha = 1, int k = -1);
  Eigen::VectorXf knn(Eigen::VectorXf& x, vector<Eigen::VectorXf>& pts, int k);

  // parent composition
  Compositor* _comp;

  // training set contexts
  map<string, vector<Context>> _train;

  // general info about the input data
  map<string, ModelInfo> _trainInfo;

  // non-parametric cache data
  map<string, vector<Eigen::VectorXf>> _axisVectorCache;

  // schema for schema sampling
  Schema _schema;

  // sliders for slider things
  map<string, Slider> _sliders;
};

}