#pragma once

#include "util.h"
#include "Logger.h"

namespace Comp {

enum GoalType {
  SELECT_ANY,
  SELECT_GROUP,
  SELECT_TARGET_COLOR,
  SELECT_TARGET_BRIGHTNESS
};

enum GoalTarget {
  NO_TARGET,
  MORE,
  LESS,
  EXACT
};

class Goal {
public:
  Goal(GoalType type, GoalTarget target);
  ~Goal();

  void setTargetColor(RGBAColor targetColor);

  // single pixel goal target determination
  bool meetsGoal(RGBAColor testColor);

  // returns a numeric value indicating how far off the test color is
  // from the goal
  // the return value will be positive if the target is not met (return
  // value is distance to goal)
  // the return value will be negative if the target is met and the
  // value represents the extent to which the target is exceeded
  float goalObjective(RGBAColor testColor);

  GoalType getType() { return _type; }
  GoalTarget getTarget() { return _target; }

private:
  GoalType _type;
  GoalTarget _target;

  RGBAColor _targetColor;
};

}