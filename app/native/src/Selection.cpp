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

bool Goal::meetsGoal(RGBAColor testColor) {
  if (_target == GoalTarget::NO_TARGET)
    return true;
  
  float diff = goalObjective(testColor);

  if (_target == GoalTarget::EXACT) {
    // diff should be within a tolerance, arbitrarily 0.1 for now
    // TODO: threshold should be modifiable
    return (abs(diff) <= 0.1);
  }
  else if (_target == GoalTarget::MORE) {
    return diff >= 0;
  }
  else if (_target == GoalTarget::LESS) {
    // lil bit weird here but a positive diff value indicates a
    // poorer match (minimization)
    return diff < 0;
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
    // right now this is just the RGB L2 but there might want to be an option for
    // hue/sat differences at some point
    diff = Utils<float>::RGBAL2Diff(_targetColor, testColor);
    break;
  }
  case SELECT_TARGET_BRIGHTNESS: {
    auto iHSL = Utils<float>::RGBToHSL(_targetColor._r * _targetColor._a, _targetColor._g * _targetColor._a, _targetColor._b * _targetColor._a);
    auto tHSL = Utils<float>::RGBToHSL(testColor._r * testColor._a, testColor._g * testColor._a, testColor._b * testColor._a);

    // should be negative when test > init
    diff = iHSL._l - tHSL._l;

    break;
  }
  default:
    diff = 0;
    break;
  }

  return diff;
}

}