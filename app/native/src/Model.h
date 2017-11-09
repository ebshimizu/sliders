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
  string id();

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

// slider parameterized function types
// since we can't really load arbitrary functions right now (i mean it could just be
// a dense set of points but that's tedious) we have some parametric functions defined.
// each parameter is controlled by one of these functions (or more if intervals work)
class ParamFunction {
public:
  virtual float eval(float x) = 0;
};

class Sawtooth : public ParamFunction {
public:
  Sawtooth(int cycles, float min, float max, bool inverted);

  virtual float eval(float x) override;

  int _cycles;
  float _min;
  float _max;
  bool _inverted;
};

// the only thing dynamic about it is that the amplitude and offset can change over time
class DynamicSine : public ParamFunction {
public:
  DynamicSine(float f, float phase, float A0, float A1, float D0, float D1);

  virtual float eval(float x) override;

  float _f;
  float _phase;

  // range values, A0 is value of A at 0, A1 is value of A at 1, etc.
  float _A0;
  float _A1;
  float _D0;
  float _D1;
};

class LinearInterp : public ParamFunction {
public:
  LinearInterp(vector<float> xs, vector<float> ys);

  virtual float eval(float x) override;

  vector<float> _xs;
  vector<float> _ys;
};

class Slider {
public:
  Slider();
  Slider(vector<LayerParamInfo> params, vector<shared_ptr<ParamFunction>> funcs);

  Context sample(const Context& in, float val);

  // adds a new parameter. By default this is placed at the end of the order
  void addParameter(LayerParamInfo param, shared_ptr<ParamFunction> func);
  void replaceParameters(vector<LayerParamInfo> params, vector<shared_ptr<ParamFunction>> funcs);

  // deletes everything
  void removeAllParameters();

  // export a json document containing function output for each parameter
  void exportGraphData(string filename, int n = 1000);

  string _name;

private:
  // in the slider class, the order vector determines how the layers
  // are composited/adjusted in the slider (for now)
  vector<LayerParamInfo> _params;
  vector<shared_ptr<ParamFunction>> _funcs;
};

// collection of ui elements used by the interface
// these elements are basically self-contained and just detail how to modify a given
// context according to its internal rules. If at some point they need to be managed by a
// non-ui object, an abstract class will be defined and they'll live in the Model object
class UISlider {
public:
  UISlider(string layer, string param, AdjustmentType type);
  UISlider(string layer, string param, AdjustmentType type, string displayName);

  string _displayName;

  Context setVal(float x, Context c);

  // sets the val without updating an associated context
  void setVal(float x) { _val = x; }

  // sets the value based on current context values
  void setVal(Context& c);

  float getVal() { return _val; }
  string getLayer() { return _layer; }
  string getParam() { return _param; }
  AdjustmentType getType() { return _type; }

private:
  string _layer;
  string _param;
  AdjustmentType _type;

  float _initVal;
  float _val;
};

// metaslider is like the regular Slider class with a lerp function.
// a second class is being written to enforce some consistency in how metasliders work
// (since slider function maps arbitrary functions to params, here they are always linear interps)
class UIMetaSlider {
public:
  UIMetaSlider(string displayName);
  ~UIMetaSlider();

  string addSlider(string layer, string param, AdjustmentType t, vector<float> xs, vector<float> ys);
  string addSlider(string layer, string param, AdjustmentType t, float min, float max);

  UISlider* getSlider(string id);
  shared_ptr<LinearInterp> getFunc(string id);

  int size();
  vector<string> names();

  void deleteSlider(string id);

  void setPoints(string id, vector<float> xs, vector<float> ys);

  // given a context, sets the value for each parameter in the meta slider
  Context setContext(float x, Context ctx);

  // scale modifies the values by the given multiplier before returning
  // scale does not affect the maximum value of the metasliders
  Context setContext(float x, float scale, Context ctx);

  // reassigns the maximum sub-slider values to be the value in the given context
  // this falls under the "oddly-specific" category of functions and is present to
  // basically make shimmer work correctly
  void reassignMax(Context c);

  float getVal() { return _val; }

  string _displayName;

private:
  map<string, UISlider*> _sliders;
  map<string, shared_ptr<LinearInterp>> _fs;
  
  float _val;
};

// a sampler will take a set of parameters, a specified objective function, and then returns
// contexts that satisfy a target objective function value
class UISampler {
public:
  UISampler(string displayName);
  ~UISampler();

  void setObjectiveMode(AxisEvalFuncType t);

  string addParam(LayerParamInfo p);
  void deleteParam(string id);

  vector<string> params();
  LayerParamInfo getParamInfo(string id);

  Context sample(float x, Context c);

  float eval(Context& c);

  AxisEvalFuncType getEvalMode() { return _objEvalMode; }
  string _displayName;

private:
  AxisEvalFuncType _objEvalMode;
  map<string, LayerParamInfo> _params;
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

  // automatically creates a file from a series of contexts (loaded from files)
  // basically just does a linear interp from one example to the next, with horizontal
  // distance proportional to distance in feature space
  void sliderFromExamples(string name, vector<string> files);

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