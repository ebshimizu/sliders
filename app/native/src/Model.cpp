#include "Model.h"

namespace Comp {

LayerParamInfo::LayerParamInfo(string name, string param) : _name(name), _param(param)
{
  _min = DBL_MAX;
  _max = DBL_MIN;
}

void LayerParamInfo::addSample(double val)
{
  _vals.push_back(val);

  if (val < _min)
    _min = val;

  if (val > _max)
    _max = val;
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

void ModelInfo::addVal(string name, string param, double val)
{
  if (_activeParams.count(name) == 0 || _activeParams[name].count(param) == 0) {
    _activeParams[name][param] = LayerParamInfo(name, param);
  }

  _activeParams[name][param].addSample(val);
}

Model::Model(Compositor* c) : _comp(c)
{
}

Model::~Model()
{
}

void Model::analyze(map<string, vector<string>> examples, string base)
{
  // first step is to load the specified files and then throw it to the other
  // analysis function
  map<string, vector<Context>> loadedExamples;

  for (auto& a : examples) {
    for (auto& file : a.second) {
      loadedExamples[a.first].push_back(_comp->contextFromDarkroom(file));
    }
  }

  Context baseCtx = _comp->contextFromDarkroom(base);

  analyze(loadedExamples, baseCtx);
}

void Model::analyze(map<string, vector<Context>> examples, Context base) {
  // right now I'm just collecting diagnostics data about the set
  for (auto& axis : examples) {
    // create info struct
    _trainInfo[axis.first] = ModelInfo(axis.first);

    for (auto& ctx : axis.second) {
      // detect changes compard to the base config
      // for each layer
      for (auto& l : ctx) {
        // check opacity
        if (l.second.getOpacity() != base[l.first].getOpacity()) {
          _trainInfo[axis.first].addVal(l.first, "opacity", l.second.getOpacity());
        }

        // check all the other params
        for (auto& adj : l.second.getAdjustments()) {
          auto data = l.second.getAdjustment(adj);

          if (adj == AdjustmentType::SELECTIVE_COLOR) {
            // special case for selective color data
            auto scData = l.second.getSelectiveColor();
            auto baseScData = base[l.first].getSelectiveColor();

            for (auto& scChannel : scData) {
              for (auto& scParam : scChannel.second) {
                if (scParam.second != baseScData[scChannel.first][scParam.first]) {
                  _trainInfo[axis.first].addVal(l.first, "sc_" + scChannel.first + "_" + scParam.first, scParam.second);
                }
              }
            }
          }
          else {
            for (auto& param : data) {
              if (param.second != base[l.first].getAdjustment(adj)[param.first]) {
                _trainInfo[axis.first].addVal(l.first, param.first, param.second);
              }
            }
          }
        }
      }
    }
  }
}

Context Model::sample()
{
  // for now just return the current context state
  return _comp->getNewContext();
}

const map<string, ModelInfo>& Model::getModelInfo()
{
  return _trainInfo;
}

}