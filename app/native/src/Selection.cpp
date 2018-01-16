#include "Selection.h"

namespace Comp {

Goal::Goal(GoalType type, GoalTarget target) : _type(type), _target(target) {
  
}

Goal::~Goal() {

}

void Goal::setTargetColor(RGBAColor targetColor)
{
  _targetColor = targetColor;
}

void Goal::setTargetColor(vector<RGBAColor> targets)
{
  _targetColors = targets;
}

void Goal::setOriginalColors(vector<RGBAColor> original)
{
  _originalColors = original;
}

bool Goal::meetsGoal(RGBAColor testColor) {
  if (_target == GoalTarget::NO_TARGET)
    return true;
  
  float diff = goalObjective(testColor);

  return meetsGoal(diff);
}

bool Goal::meetsGoal(vector<RGBAColor> testColors)
{
  if (testColors.size() != _originalColors.size()) {
    getLogger()->log("target color length mismatch. Aborting...", Comp::ERR);
    return false;
  }

  return meetsGoal(goalObjective(testColors));
}

bool Goal::meetsGoal(float val)
{
  if (_type == SELECT_TARGET_CHROMA) {
    // target chroma is a bit weird, how do we tell if the goal of "more" is achieved compared to "less"
    // i'd view "more" as "similar" and "less" as "different"
    if (_target == GoalTarget::EXACT)
      return (abs(val) <= 2.3);
    else if (_target == GoalTarget::MORE) {
      // within a threshold of 5 Lab JND which is 11.5
      return ((val - 11.5) <= 0);
    }
    else if (_target == GoalTarget::LESS) {
      // outside a threshold of 5 Lab JND which is 11.5
      return (-val) - 11.5 > 0;
    }
  }
  else {
    if (_target == GoalTarget::EXACT) {
      // exact expectes results to be really tight, so 2.3 (the lab JND) should serve well
      return (abs(val) <= 2.3);
    }
    else {
      return val <= 0;
    }
  }

  // if somehow we get here just return false and log it, we shouldn't get to this point
  getLogger()->log("Unexpected: goal conditions not checked, returning false");
  return false;
}

float Goal::goalObjective(RGBAColor testColor)
{
  float diff;

  switch (_type) {
  // select any and select group are for selecting, well, any layer
  // so the color doesn't really matter
  case SELECT_ANY:
  case SELECT_GROUP:
    diff = 0;
    break;
  case SELECT_TARGET_COLOR: {
    // lab l2 diff
    diff = Utils<float>::LabL2Diff(_targetColor, testColor);
    break;
  }
  case SELECT_TARGET_BRIGHTNESS: {
    auto iHSL = Utils<float>::RGBToHSL(_originalColor._r * _originalColor._a, _originalColor._g * _originalColor._a, _originalColor._b * _originalColor._a);
    auto tHSL = Utils<float>::RGBToHSL(testColor._r * testColor._a, testColor._g * testColor._a, testColor._b * testColor._a);

    // should be negative when test > init
    diff = iHSL._l - tHSL._l;

    break;
  }
  case SELECT_TARGET_CHROMA: {
    auto oLab = Utils<float>::RGBAToLab(_targetColor);
    auto tLab = Utils<float>::RGBAToLab(testColor);

    diff = sqrt((oLab._a - tLab._a) * (oLab._a - tLab._a) + (oLab._b - tLab._b) * (oLab._b - tLab._b));

    break;
  }
  default:
    diff = 0;
    break;
  }

  // if direction is less, diff should be inverted (maximization)
  if (_target == GoalTarget::LESS)
    diff = -diff;

  return diff;
}

float Goal::goalObjective(vector<RGBAColor> testColors)
{
  float sum = 0;

  for (int i = 0; i < testColors.size(); i++) {
    _originalColor = _originalColors[i];

    float val = goalObjective(testColors[i]);
    sum += val;
  }

  return sum / _originalColors.size();
}

}