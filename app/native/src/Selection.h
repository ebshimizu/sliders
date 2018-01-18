#pragma once

#include "util.h"
#include "Logger.h"
#include <random>
#include <set>

namespace Comp {

enum GoalType {
  SELECT_ANY = 0,
  SELECT_GROUP = 1,
  SELECT_TARGET_COLOR = 2,
  SELECT_TARGET_BRIGHTNESS = 3,
  SELECT_TARGET_CHROMA = 4,
  GOAL_COLORIZE = 5,
  GOAL_SATURATE = 6,
  GOAL_CONTRAST = 7
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
  bool meetsGoal(float val);

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

// quick implementation based on
// https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
// assumes every axis is limited to [0,1] (which is true in this instance)
class PoissonDisk {
public:
  PoissonDisk(float r, int n, int k = 30);
  ~PoissonDisk();

  void sample(vector<float> x0 = { });

  vector<vector<float>> allPoints();

  vector<vector<float>> selectedPoints(set<int> indices);

  // returns a list of points nearby (within 3 cell radius) of the given list
  vector<vector<float>> nearby(vector<vector<float>>& pts);

private:
  void init();

  // inserts a sample into the background grid (and the accepted pt list)
  int insert(vector<float>& pt);

  // given integer cell coordinates, returns the flattened index into the bg array;
  int gridIndex(vector<int>& pt);

  // floating pt coord to grid index
  int ptToGrid(float x);
  int ptToIndex(vector<float>& x);

  // if outside [0, 1], point is invalid
  bool ptInBounds(vector<float>& x);

  float l2Dist(vector<float>& x, vector<float>& y);

  // returns a list of adjacent point ids to the given grid point
  vector<int> adjacentPts(vector<float>& x);
  vector<int> adjacentPts(vector<int>& x);

  vector<int> adjacentPts(vector<float>& x, int radius);
  vector<int> adjacentPts(vector<int>& x, int radius);

  vector<int> _bgArray;
  vector<vector<float>> _pts;
  vector<int> _active;

  float _r;
  int _n;
  int _k;

  // the grid is a n-D cube, this is the axis size in cells
  int _gridSize;
  float _cellSize;
};

}