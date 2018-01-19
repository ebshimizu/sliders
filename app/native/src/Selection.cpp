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
  else if (_type == GOAL_COLORIZE) {
    if (_target == GoalTarget::EXACT)
      return (abs(val) <= 3);
    else if (_target == GoalTarget::MORE) {
      return ((val - 10) <= 0);
    }
    else if (_target == GoalTarget::LESS) {
      return (-val) - 10 > 0;
    }
  }
  else {
    if (_target == GoalTarget::EXACT) {
      // these values are based on the lab jnd. 
      return (abs(val) <= 2.3 * 4);
    }
    else {
      return val <= -0.1;
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
  case GOAL_COLORIZE: {
    // colorize is HUE only
    auto iHSL = Utils<float>::RGBToHSL(_targetColor._r * _targetColor._a, _targetColor._g * _targetColor._a, _targetColor._b * _targetColor._a);
    auto tHSL = Utils<float>::RGBToHSL(testColor._r * testColor._a, testColor._g * testColor._a, testColor._b * testColor._a);

    diff = min(min(abs(iHSL._h - tHSL._h), abs(iHSL._h - (tHSL._h + 360))), abs(iHSL._h - (tHSL._h - 360)));
    break;
  }
  case GOAL_SATURATE: {
    auto iHSL = Utils<float>::RGBToHSL(_originalColor._r * _originalColor._a, _originalColor._g * _originalColor._a, _originalColor._b * _originalColor._a);
    auto tHSL = Utils<float>::RGBToHSL(testColor._r * testColor._a, testColor._g * testColor._a, testColor._b * testColor._a);

    diff = iHSL._s - tHSL._s;
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

PoissonDisk::PoissonDisk(float r, int n, int k) : _r(r), _n(n), _k(k)
{
  init();
}

PoissonDisk::~PoissonDisk()
{
}

void PoissonDisk::sample(vector<float> x0)
{
  // rng objects
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<float> zeroOne(0, 1);
  uniform_real_distribution<float> sphereRadius(pow(_r, _n), pow(2 * _r, _n));
  normal_distribution<float> norm;

  if (x0.size() != _n) {
    // sample if no pt given or invalid
    x0.resize(_n);
    for (int i = 0; i < _n; i++)
      x0[i] = zeroOne(gen);
  }

  // insert initial pt
  int x0i = insert(x0);
  _active.push_back(x0i);

  // while the active list isn't empty
  while (_active.size() > 0) {
    // pull a random index from it
    uniform_int_distribution<int> idxSampler(0, _active.size() - 1);
    int selected = idxSampler(gen);

    vector<float> pt = _pts[_active[selected]];

    int count = 0;
    while (count < _k) {
      // generate a pt between r and 2r of the current pt
      vector<float> newpt;
      newpt.resize(pt.size());
      float dist = 0;

      float r = pow(sphereRadius(gen), 1.0f / _n);

      for (int j = 0; j < _n; j++) {
        float p = norm(gen);
        newpt[j] = p;
        dist += p;
      }

      dist = sqrt(dist);

      // scale point
      for (int j = 0; j < _n; j++) {
        newpt[j] = r * newpt[j] + pt[j];
      }

      // validate
      if (ptInBounds(newpt)) {
        // check distance to neighbors
        vector<int> adjacent = adjacentPts(newpt);

        bool accept = true;
        for (auto& adjacentPt : adjacent) {
          vector<float> y = _pts[adjacentPt];

          // check l2 dist
          if (l2Dist(newpt, y) < r) {
            accept = false;
            break;
          } 
        }

        if (accept) {
          int id = insert(newpt);

          // sometimes id conflicts happen, in which case we just skip
          // this shouldn't happen really, but instead of crashing we'll continue
          if (id > 0) {
            _active.push_back(id);
            break;  // count < _k
          }
        }
        else {
          count++;
        }
      }
      // validation failure is not marked as a failure technically, since it's not a valid point
    }

    // if we failed to find a thing, remove the point from the active set
    if (count == _k) {
      _active.erase(_active.begin() + selected);
    }
  }
}

vector<vector<float>> PoissonDisk::allPoints()
{
  return _pts;
}

vector<vector<float>> PoissonDisk::selectedPoints(set<int> indices)
{
  vector<vector<float>> pts;

  for (auto& id : indices) {
    if (id > 0 && id < _pts.size())
      pts.push_back(_pts[id]);
  }

  return pts;
}

vector<vector<float>> PoissonDisk::nearby(vector<vector<float>>& pts)
{
  set<int> uniqueIds;

  for (auto& p : pts) {
    vector<int> nearby = adjacentPts(p, 3);

    for (auto id : nearby)
      uniqueIds.insert(id);
  }

  return selectedPoints(uniqueIds);
}

void PoissonDisk::init()
{
  _active.clear();

  // determine cell size
  _cellSize = _r / sqrt(_n);

  // cells per axis rounded up
  _gridSize = ceil(1.0f / _cellSize);

  // create the bg array
  _bgArray.resize(pow(_gridSize, _n));

  // set all array values to -1
  fill(_bgArray.begin(), _bgArray.end(), -1);
}

int PoissonDisk::insert(vector<float>& pt)
{
  // debug 
  if (_bgArray[ptToIndex(pt)] != -1) {
    getLogger()->log("Poisson Disk grid cell not empty on attempted insert", LogLevel::ERR);
    return -1;
  }

  int idx = _pts.size();
  _pts.push_back(pt);

  _bgArray[ptToIndex(pt)] = idx;
  return idx;
}

int PoissonDisk::gridIndex(vector<int>& pt)
{
  // form is basically x[0] + x[1] * dim[0] + ... + x[n] * dim[n - 1]
  // but dim here is all the same
  // so it's kinda easy
  int idx = pt[0];
  int d = 1;
  for (int i = 1; i < pt.size(); i++) {
    d *= _gridSize;
    idx += pt[i] * d;
  }

  return idx;
}

int PoissonDisk::ptToGrid(float x)
{
  return floor(x / _cellSize);
}

int PoissonDisk::ptToIndex(vector<float>& x)
{
  vector<int> pt;
  pt.resize(x.size());

  for (int i = 0; i < x.size(); i++)
    pt[i] = ptToGrid(x[i]);

  return gridIndex(pt);
}

bool PoissonDisk::ptInBounds(vector<float>& x)
{
  for (int i = 0; i < x.size(); i++) {
    if (x[i] < 0 || x[i] > 1)
      return false;

    // cell check
    int cell = ptToGrid(x[i]);
    if (cell < 0 || cell >= _gridSize)
      return false;
  }

  return true;
}

float PoissonDisk::l2Dist(vector<float>& x, vector<float>& y)
{
  float sum = 0;
  for (int i = 0; i < x.size(); i++) {
    sum += (x[i] - y[i]) * (x[i] - y[i]);
  }

  return sqrt(sum);
}

vector<int> PoissonDisk::adjacentPts(vector<float>& x)
{
  vector<int> grid;
  for (int i = 0; i < x.size(); i++) {
    grid.push_back(ptToGrid(x[i]));
  }

  return adjacentPts(grid);
}

vector<int> PoissonDisk::adjacentPts(vector<float>& x, int radius) {
  vector<int> grid;
  for (int i = 0; i < x.size(); i++) {
    grid.push_back(ptToGrid(x[i]));
  }

  return adjacentPts(grid, radius);
}

vector<int> PoissonDisk::adjacentPts(vector<int>& x) {
  // for simplicity, diagonals are not considered
  // this should hopefully not affect correctness
  vector<int> pts;
  for (int i = 0; i < x.size(); i++) {
    vector<int> testPt = x;
    testPt[i] -= 1;

    if (testPt[i] >= 0 && _bgArray[gridIndex(testPt)] != -1)
      pts.push_back(_bgArray[gridIndex(testPt)]);

    testPt[i] = x[i] + 1;
    if (testPt[i] < _n && _bgArray[gridIndex(testPt)] != -1)
      pts.push_back(_bgArray[gridIndex(testPt)]);
  }

  // don't forget the cell the point is, uh, in already
  if (_bgArray[gridIndex(x)] != -1)
    pts.push_back(_bgArray[gridIndex(x)]);

  return pts;
}

vector<int> PoissonDisk::adjacentPts(vector<int>& x, int radius)
{
  vector<int> pts;
  for (int i = 0; i < x.size(); i++) {
    vector<int> testPt = x;
    int base = x[i];

    for (int j = -radius; j <= radius; j++) {
      if (j == 0)
        continue;

      testPt[i] = base + j;

      if (testPt[i] >= 0 && testPt[i] < _gridSize && _bgArray[gridIndex(testPt)] != -1)
        pts.push_back(_bgArray[gridIndex(testPt)]);
    }
  }

  // don't forget the cell the point is, uh, in already
  if (_bgArray[gridIndex(x)] != -1)
    pts.push_back(_bgArray[gridIndex(x)]);

  return pts;
}

}