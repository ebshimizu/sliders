#include "Model.h"
#include "gibbs_with_gaussian_mixture.h"

namespace Comp {

LayerParamInfo::LayerParamInfo(AdjustmentType t, string name, string param) :
  _type(t), _name(name), _param(param)
{
  _min = DBL_MAX;
  _max = DBL_MIN;
  _inverted = false;
}

void LayerParamInfo::addSample(double val)
{
  // check for duplicates
  bool accept = true;
  for (auto& v : _vals) {
    if (v == val) {
      accept = false;
      break;
    }
  }

  if (accept) {
    _vals.push_back(val);

    if (val < _min)
      _min = val;

    if (val > _max)
      _max = val;
  }
}

int LayerParamInfo::count()
{
  return _vals.size();
}

ModelInfo::ModelInfo(string name) : _name(name)
{
}

ModelInfo::~ModelInfo()
{
}

int ModelInfo::count()
{
  return _activeParams.size();
}

void ModelInfo::addVal(AdjustmentType t, string name, string param, double val)
{
  string key = name + ":" + param;
  if (_activeParams.count(key) == 0) {
    _activeParams[key] = LayerParamInfo(t, name, param);
  }

  _activeParams[key].addSample(val);
}

void ModelInfo::cleanup()
{
  vector<string> toRemove;
  for (auto& info : _activeParams) {
    if (info.second.count() <= 1)
      toRemove.push_back(info.first);
  }

  for (auto& s : toRemove) {
    _activeParams.erase(s);
  }
}

Eigen::VectorXf ModelInfo::contextToAxisVector(Context & c)
{
  Eigen::VectorXf v;
  v.resize(count());

  int i = 0;
  for (auto& param : _activeParams) {
    Layer& l = c[param.second._name];

    if (param.second._type == AdjustmentType::OPACITY) {
      if (l._visible) {
        v[i] = l.getOpacity();
      }
      else {
        v[i] = 0;
      }
    }
    else {
      v[i] = l.getAdjustment(param.second._type)[param.second._param];
    }

    i++;
  }

  return v;
}

void ModelInfo::axisVectorToContext(Eigen::VectorXf & x, Context & c)
{
  int i = 0;
  for (auto& param : _activeParams) {
    Layer& l = c[param.second._name];

    if (param.second._type == AdjustmentType::OPACITY) {
      l.setOpacity(x(i));
    }
    else {
      l.addAdjustment(param.second._type, param.second._param, x(i));
    }

    i++;
  }
}

AxisDef::AxisDef(string name, vector<LayerParamInfo> params, AxisEvalFuncType t) :
  _name(name), _params(params), _evalFuncMode(t)
{

}

AxisDef::~AxisDef()
{
}

float AxisDef::eval(Context & ctx)
{
  switch (_evalFuncMode) {
  case AVERAGE:
    return evalAvg(ctx);
  default:
    return evalAvg(ctx);
  }
}

Context AxisDef::sample(Context ctx, const vector<AxisConstraint>& constraints)
{
  // TODO: in the case of infeasible constraints, should have a termination condition

  // rng
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<float> zeroOne(0, 1);

  vector<float> results;
  vector<int> c;
  vector<float> s;    // unused currently

  float min = 0;
  float max = 1;

  // look at the constraints and if there's a range constraint, pick a target mean in there
  for (auto& c : constraints) {
    if (c._mode == AxisConstraintMode::TARGET_RANGE && c._axis == _name) {
      min = c._min;
      max = c._max;
    }
  }

  uniform_real_distribution<float> targetAvg(min, max);
  float target = targetAvg(gen);

  for (auto& p : _params) {
    if (p._type == AdjustmentType::OPACITY) {
      results.push_back(ctx[p._name].getOpacity());
    }
    else {
      results.push_back(ctx[p._name].getAdjustment(p._type)[p._param]);
    }

    c.push_back(0);
    s.push_back(1);
  }

  // hey look it's the lighting code
  GibbsSamplingGaussianMixturePrior(results, c, s, results.size(), 0, target, target, false);

  for (int i = 0; i < results.size(); i++) {
    auto p = _params[i];
    float val = results[i];

    if (p._inverted) {
      val = 1 - val;
    }

    if (p._type == AdjustmentType::OPACITY) {
      ctx[p._name].setOpacity(val);
    }
    else {
      ctx[p._name].getAdjustment(p._type)[p._param] = val;
    }
  }

  return ctx;
}

float AxisDef::evalAvg(Context & ctx)
{
  float sum = 0;
  int count = 0;

  for (const auto& p : _params) {
    float val;

    if (p._type == AdjustmentType::OPACITY) {
      val = ctx[p._name].getOpacity();
    }
    else {
      val = ctx[p._name].getAdjustment(p._type)[p._param];
    }

    if (p._inverted) {
      sum += 1 - val;
    }
    else {
      sum += val;
    }

    count++;
  }

  return sum / count;
}

Schema::Schema()
{
}

Schema::Schema(string filename)
{
  nlohmann::json data;
  ifstream input(filename);

  if (!input.is_open()) {
    // failing to load will return an empty context
    getLogger()->log("Failed to open file " + filename, LogLevel::ERR);
    return;
  }

  input >> data;

  loadFromJson(data);
}

Context Schema::sample(const Context & in, vector<AxisConstraint>& constraints)
{
  Context temp(in);
  bool accept = false;

  // TODO: needs escape condition when constraints are infeasible
  while (!accept) {
    for (auto& a : _axes) {
      temp = a.sample(temp, constraints);
    }

    accept = verifyConstraints(temp, constraints);
  }

  return temp;
}

map<string, float> Schema::axisEval(Context& in)
{
  map<string, float> vals;

  for (auto& a : _axes) {
    vals[a._name] = a.eval(in);
  }

  return vals;
}

void Schema::loadFromJson(nlohmann::json data)
{
  _axes.clear();

  for (nlohmann::json::iterator it = data.begin(); it != data.end(); ++it) {
    string name = it.value()["axisName"].get<string>();
    AxisEvalFuncType mode = (AxisEvalFuncType)it.value()["evalFunc"].get<int>();
    vector<LayerParamInfo> params;

    for (int i = 0; i < it.value()["params"].size(); i++) {
      string name = it.value()["params"][i]["layerName"].get<string>();
      AdjustmentType type = (AdjustmentType)it.value()["params"][i]["adjustmentType"].get<int>();
      string param = it.value()["params"][i]["param"].get<string>();
      LayerParamInfo info(type, name, param);

      if (it.value()["params"][i].count("inverted") > 0) {
        info._inverted = it.value()["params"][i]["inverted"].get<bool>();
      }
      
      params.push_back(info);
    }

    _axes.push_back(AxisDef(name, params, mode));
  }
}

bool Schema::verifyConstraints(Context & ctx, const vector<AxisConstraint>& constraints)
{
  bool accept = true;

  // eval each axis
  map<string, float> axisVals;
  for (int i = 0; i < _axes.size(); i++) {
    axisVals[_axes[i]._name] = _axes[i].eval(ctx);
  }

  for (auto& c : constraints) {
    if (c._mode == AxisConstraintMode::TARGET_RANGE) {
      float val = axisVals[c._axis];

      if (val < c._min || val > c._max) {
        accept = false;
        break;
      }
    }
    else if (c._mode == AxisConstraintMode::CONSTANT_VALUE) {
      float val;
      if (c._type == AdjustmentType::OPACITY) {
        val = ctx[c._layer].getOpacity();
      }
      else {
        val = ctx[c._layer].getAdjustment(c._type)[c._param];
      }

      if (abs(val - c._val) > c._tolerance) {
        accept = false;
        break;
      }
    }
  }

  return accept;
}

Sawtooth::Sawtooth(int cycles, float min, float max, bool inverted) :
  _cycles(cycles), _min(min), _max(max), _inverted(inverted) {

}

float Sawtooth::eval(float x)
{
  float pval = fmod(x * _cycles, 1);
  float range = _max - _min;
  pval *= range + _min;

  if (x == 1)
    pval = range + _min;

  if (_inverted) {
    pval = (range + _min) - pval;
  }

  return pval;
}

DynamicSine::DynamicSine(float f, float phase, float A0, float A1, float D0, float D1) :
  _f(f), _phase(phase), _A0(A0), _A1(A1), _D0(D0), _D1(D1)
{
}

float DynamicSine::eval(float x)
{
  float freq = 2 * M_PI * _f;

  // lerp for A and D
  float A = _A0 * (1 - x) + _A1 * x;
  float D = _D0 * (1 - x) + _D1 * x;

  return A * sin(freq * x + _phase) + D;
}

Slider::Slider() {
  // empty constructor
}

Slider::Slider(vector<LayerParamInfo> params, vector<shared_ptr<ParamFunction>> funcs) : _params(params), _funcs(funcs)
{
}

Context Slider::sample(const Context & in, float val)
{
  // the sampling function for this works like this at the moment:
  // - just sample the function for each parameter at the given value
  Context out(in);

  // abort early if functions and parameters aren't the same size
  if (_params.size() != _funcs.size())
    return out;

  for (int i = 0; i < _params.size(); i++) {
    LayerParamInfo param = _params[i];
    float pval = _funcs[i]->eval(val);

    if (param._type == AdjustmentType::OPACITY) {
      out[param._name].setOpacity(pval);
    }
    else {
      out[param._name].getAdjustment(param._type)[param._param] = pval;
    }
  }

  return out;
}

void Slider::addParameter(LayerParamInfo param, shared_ptr<ParamFunction> func)
{
  _params.push_back(param);
  _funcs.push_back(func);
}

void Slider::replaceParameters(vector<LayerParamInfo> params, vector<shared_ptr<ParamFunction>> funcs)
{
  removeAllParameters();
  _params = params;
  _funcs = funcs;
}

void Slider::removeAllParameters()
{
  _params.clear();
  _funcs.clear();
}

void Slider::exportGraphData(string filename, int n)
{
  nlohmann::json out;

  for (int i = 0; i < _params.size(); i++) {
    LayerParamInfo param = _params[i];
    shared_ptr<ParamFunction> f = _funcs[i];

    nlohmann::json data;
    string name = param._name + ":" + param._param;
    data["paramName"] = name;
    data["type"] = param._type;

    nlohmann::json xs;
    nlohmann::json ys;

    for (int j = 0; j < n; j++) {
      float val = (float)j / n;
      xs.push_back(val);
      ys.push_back(f->eval(val));
    }

    data["x"] = xs;
    data["y"] = ys;

    out[name] = data;
  }

  ofstream file(filename);
  file << out.dump(2);
}

Model::Model(Compositor* c) : _comp(c)
{
}

Model::~Model()
{
}

void Model::analyze(map<string, vector<string>> examples)
{
  // first step is to load the specified files and then throw it to the other
  // analysis function
  map<string, vector<Context>> loadedExamples;

  for (auto& a : examples) {
    for (auto& file : a.second) {
      loadedExamples[a.first].push_back(_comp->contextFromDarkroom(file));
    }
  }

  analyze(loadedExamples);
}

void Model::analyze(map<string, vector<Context>> examples) {
  _train = examples;

  // right now I'm just collecting diagnostics data about the set
  for (auto& axis : examples) {
    // create info struct
    _trainInfo[axis.first] = ModelInfo(axis.first);

    for (auto& ctx : axis.second) {
      // record unique values for each axis
      for (auto& l : ctx) {
        // check opacity
        float opacity = l.second.getOpacity();
        if (!l.second._visible) {
          opacity = 0;
        }

        _trainInfo[axis.first].addVal(AdjustmentType::OPACITY, l.first, "opacity", opacity);

        // check all the other params
        for (auto& adj : l.second.getAdjustments()) {
          auto data = l.second.getAdjustment(adj);

          if (adj == AdjustmentType::SELECTIVE_COLOR) {
            // special case for selective color data
            auto scData = l.second.getSelectiveColor();

            for (auto& scChannel : scData) {
              for (auto& scParam : scChannel.second) {
                _trainInfo[axis.first].addVal(adj, l.first, "sc_" + scChannel.first + "_" + scParam.first, scParam.second);
              }
            }
          }
          else {
            for (auto& param : data) {
              _trainInfo[axis.first].addVal(adj, l.first, param.first, param.second);
            }
          }
        }
      }
    }

    _trainInfo[axis.first].cleanup();
  }
}

void Model::addSchema(string file)
{
  _schema = Schema(file);
}

map<string, float> Model::schemaEval(Context & c)
{
  return _schema.axisEval(c);
}

Context Model::sample()
{
  // first version: take the info in the analysis thing and then
  // sample uniformly at random from the given ranges
  Context ctx = _comp->getNewContext();
  
  // rng
  random_device rd;
  mt19937 gen(rd());

  for (auto& axis : _trainInfo) {
    for (auto& p : axis.second._activeParams) {
      string layer = p.second._name;
      string param = p.second._param;

      uniform_real_distribution<float> range(p.second._min, p.second._max);
      float sample = range(gen);

      if (p.first == "opacity") {
        ctx[layer].setOpacity(sample);
      }
      else {
        ctx[layer].addAdjustment(p.second._type, param, sample);
      }
      // something something selective color :(
    }
  }

  return ctx;
}

Context Model::nonParametricLocalSample(map<string, Context> axes, float alpha, int k)
{
  Context ret = _comp->getNewContext();

  for (auto& axis : axes) {
    ret = nonParametricLocalSample(ret, axis.second, axis.first, alpha, k);
  }

  return ret;
}

Context Model::nonParametricLocalSample(Context init, Context ctx0, string axis, float alpha, int k)
{
  // samples a single axis
  vector<Eigen::VectorXf> ctxVectors;
  for (auto& c : _train[axis]) {
    ctxVectors.push_back(_trainInfo[axis].contextToAxisVector(c));
  }

  Eigen::VectorXf x0 = _trainInfo[axis].contextToAxisVector(ctx0);

  // need to sample using the log rules in appendix B for stability reasons (they claim)
  Eigen::MatrixXf sigma0 = computeBandwidthMatrix(x0, ctxVectors, alpha, k);

  stringstream ss;
  ss << "sigma0: " << sigma0;
  getLogger()->log(ss.str(), LogLevel::DBG);

  // compute weights
  vector<float> dists;
  int maxIdx = -1;
  float maxDist = -1e10;

  for (int i = 0; i < ctxVectors.size(); i++) {
    Eigen::MatrixXf sigmai = computeBandwidthMatrix(ctxVectors[i], ctxVectors, alpha, k);
    Eigen::MatrixXf sigmax = sigma0 + sigmai;
    float dist = log(gaussianKernel(x0, ctxVectors[i], sigmax));
    dists.push_back(dist);

    if (dist > maxDist) {
      maxDist = dist;
      maxIdx = i;
    }
  }

  float Lm = dists[maxIdx];
  float denom = 0;
  
  for (int i = 0; i < ctxVectors.size(); i++) {
    denom += exp(dists[i] - Lm);
  }

  // compute distribution probabilities
  map<float, int> probs;
  float accum = 0;

  for (int i = 0; i < ctxVectors.size(); i++) {
    accum += exp(dists[i] - Lm) / denom;
    probs[accum] = i;
  }

  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<float> zeroOne(0, 1);

  int dist = probs.upper_bound(zeroOne(gen))->second;

  // sample from gaussian dist 
  Eigen::MatrixXf si = computeBandwidthMatrix(ctxVectors[dist], ctxVectors, alpha, k);
  Eigen::MatrixXf coVar = (sigma0.inverse() + si.inverse()).inverse();
  Eigen::VectorXf mean = coVar *  (sigma0.inverse() * x0 + si.inverse() * ctxVectors[dist]);

  // sampling as recommended in wikipedia
  Eigen::VectorXf z;
  z.resizeLike(mean);
  normal_distribution<float> stdNorm;

  for (int i = 0; i < mean.size(); i++) {
    z(i) = stdNorm(gen);
  }

  Eigen::VectorXf result = mean + coVar.llt().matrixL() * z;

  ss = stringstream();
  ss << "result vector: \n" << result;
  getLogger()->log(ss.str(), LogLevel::DBG);

  _trainInfo[axis].axisVectorToContext(result, init);

  return init;
}

Context Model::schemaSample(Context x0, vector<AxisConstraint>& constraints)
{
  return _schema.sample(x0, constraints);
}

const map<string, ModelInfo>& Model::getModelInfo()
{
  return _trainInfo;
}

map<string, vector<Context>> Model::getInputData()
{
  return _train;
}

Slider& Model::getSlider(string name)
{
  if (_sliders.count(name) > 0) {
    return _sliders[name];
  }

  // don't add extra entries if [name] doesn't exist
  // but still return something i guess???
  return Slider();
}

void Model::addSlider(string name, Slider s)
{
  _sliders[name] = s;
}

void Model::deleteSlider(string name)
{
  _sliders.erase(name);
}

vector<string> Model::getSliderNames()
{
  vector<string> ids;
  for (auto& s : _sliders) {
    ids.push_back(s.first);
  }

  return ids;
}

float Model::gaussianKernel(Eigen::VectorXf& x, Eigen::VectorXf& xi, Eigen::MatrixXf& sigmai)
{
  int n = x.size();

  float denom = pow((2 * M_PI), n / 2.0f) * sqrt(sigmai.norm());
  return exp(-0.5 * (x - xi).transpose() * sigmai.inverse() * (x - xi)) / denom;
}

Eigen::MatrixXf Model::computeBandwidthMatrix(Eigen::VectorXf& x, vector<Eigen::VectorXf>& pts, float alpha, int k)
{
  int n = x.size();
  int N = pts.size();

  if (k == -1) {
    k = n;
  }

  Eigen::MatrixXf sigma;
  sigma.resize(n, n);

  // standard computation
  float denom = 0;
  vector<float> weights;
  Eigen::MatrixXf sigmai = alpha * (x - knn(x, pts, k)).squaredNorm() * Eigen::MatrixXf::Identity(n, n);

  for (int i = 0; i < pts.size(); i++) {
    float w = gaussianKernel(x, pts[i], sigmai);
    denom += w;
    weights.push_back(w);
  }

  for (int t = 0; t < n; t++) {
    for (int s = 0; s < n; s++) {
      float sst = 0;
      
      for (int i = 0; i < N; i++) {
        float wi = weights[i] / denom;
        sst += wi * (pts[i](s) - x(s)) * (pts[i](t) - x(t));
      }

      sigma(s, t) = sst;
    }
  }

  stringstream ss;
  ss << "Sigma: \n" << sigma;
  getLogger()->log(ss.str(), LogLevel::DBG);

  // bandwidth shrinkage
  Eigen::MatrixXf targetMatrix;
  targetMatrix.resizeLike(sigma);

  for (int t = 0; t < n; t++) {
    for (int s = 0; s < n; s++) {
      if (s == t) {
        targetMatrix(s, t) = sigma(s, s);
      }
      else {
        targetMatrix(s, t) = 0;
      }
    }
  }

  ss = stringstream();
  ss << "Target Matrix: \n" << targetMatrix;
  getLogger()->log(ss.str(), LogLevel::DBG);

  // lambda compute
  float lnum = 0;
  float ldenom = 0;

  for (int t = 0; t < n; t++) {
    for (int s = 0; s < n; s++) {
      if (t != s) {
        float wstSum = sigma(s, t);
        float wAvg = denom / N;

        float term1 = 0;
        float term2 = 0;
        float term3 = 0;

        // ugh nested loops for this computation
        for (int i = 0; i < N; i++) {
          float wist = (pts[i](s) - x(s)) * (pts[i](t) - x(t));
          float wi = weights[i] / denom;

          term1 += pow(wi * wist - wAvg * wstSum, 2);
          term2 += (wi - wAvg) * (wi * wist - wAvg * wstSum);
          term3 += pow(wi - wAvg, 2);
        }

        float varst = (N / (N - 1.0f)) * (term1 - 2 * wstSum * term2 + pow(wstSum, 2) * term3);

        lnum += varst;
        ldenom += pow(sigma(s, t), 2);
      }
    }
  }

  float lambda = clamp(lnum / ldenom, 0.0f, 1.0f);

  // sigma adjustment
  return lambda * targetMatrix + (1 - lambda) * sigma;
}

Eigen::VectorXf Model::knn(Eigen::VectorXf & x, vector<Eigen::VectorXf>& pts, int k)
{
  // returns the k-th nearest neighbor. If k > N, returns the last element in the points
  multimap<float, int> sorted;

  for (int i = 0; i < pts.size(); i++) {
    float dist = (x - pts[i]).norm();
    sorted.insert(std::pair<float, int>(dist, i));
  }

  // iterate
  int closest = -1;
  for (auto& x : sorted) {
    k--;
    closest = x.second;

    if (k <= 0)
      break;
  }

  return pts[closest];
}

}