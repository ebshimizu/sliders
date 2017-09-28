#include "Model.h"

namespace Comp {

LayerParamInfo::LayerParamInfo(AdjustmentType t, string name, string param) :
  _type(t), _name(name), _param(param)
{
  _min = DBL_MAX;
  _max = DBL_MIN;
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
  if (_activeParams.count(name) == 0 || _activeParams[name].count(param) == 0) {
    _activeParams[name][param] = LayerParamInfo(t, name, param);
  }

  _activeParams[name][param].addSample(val);
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
        _trainInfo[axis.first].addVal(AdjustmentType::OPACITY, l.first, "opacity", l.second.getOpacity());

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
  }
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
    for (auto& l : axis.second._activeParams) {
      for (auto& p : l.second) {
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
  }

  return ctx;
}

const map<string, ModelInfo>& Model::getModelInfo()
{
  return _trainInfo;
}

}