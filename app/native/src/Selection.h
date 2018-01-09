#pragma once

#include "util.h"
#include "Logger.h"

namespace Comp {

enum GoalType {
  SELECT_ANY = 0,
  SELECT_GROUP = 1,
  SELECT_TARGET_COLOR = 2,
  SELECT_TARGET_BRIGHTNESS = 3
};

enum GoalTarget {
  NO_TARGET = 0,
  MORE = 1,
  LESS = 2,
  EXACT = 3
};

// return value for goal based selection
// subject to a lot of change
struct GoalResult {
  string _param;
  float _val;
};

class Goal {
public:
  Goal(GoalType type, GoalTarget target);
  ~Goal();

  void setTargetColor(RGBAColor targetColor);
  void setTargetColor(vector<RGBAColor> targets);
  void setOriginalColors(vector<RGBAColor> original);

  // single pixel goal target determination
  bool meetsGoal(RGBAColor testColor);
  bool meetsGoal(vector<RGBAColor> testColors);

  // returns a numeric value indicating how far off the test color is
  // from the goal
  // the return value will be positive if the target is not met (return
  // value is distance to goal)
  // the return value will be negative if the target is met and the
  // value represents the extent to which the target is exceeded
  float goalObjective(RGBAColor testColor);
  float goalObjective(vector<RGBAColor> testColors);

  GoalType getType() { return _type; }
  GoalTarget getTarget() { return _target; }

private:
  bool meetsGoal(float val);

  GoalType _type;
  GoalTarget _target;

  RGBAColor _targetColor;
  RGBAColor _originalColor;
  vector<RGBAColor> _targetColors;

  // little redundant right now, this is used to store the current pixels
  // that are being compared against
  // target color is specifically for a color target
  vector<RGBAColor> _originalColors;
};

}