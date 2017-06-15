#include "constraintData.h"

namespace Comp {

ConstraintData::ConstraintData() : _locked(false)
{
}

ConstraintData::~ConstraintData()
{
}

void ConstraintData::setMaskLayer(string layerName, shared_ptr<Image> img, ConstraintType type)
{
  if (_locked) {
    getLogger()->log("Unable to update mask data. Object is locked.", LogLevel::WARN);
    return;
  }

  _rawInput[layerName] = img;
  _type[layerName] = type;
}

shared_ptr<Image> ConstraintData::getRawInput(string layerName)
{
  if (_rawInput.count(layerName) > 0) {
    return _rawInput[layerName];
  }
  else {
    return nullptr;
  }
}

void ConstraintData::deleteMaskLayer(string layerName)
{
  if (_locked) {
    getLogger()->log("Unable to delete mask data. Object is locked.", LogLevel::WARN);
    return;
  }

  _rawInput.erase(layerName);
  _type.erase(layerName);
}

void ConstraintData::deleteAllData()
{
  if (_locked) {
    getLogger()->log("Unable to delete mask data. Object is locked.", LogLevel::WARN);
    return;
  }

  _rawInput.clear();
  _type.clear();
}

}