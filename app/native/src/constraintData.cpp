#include "constraintData.h"

namespace Comp {

Superpixel::Superpixel(RGBAColor color, Pointi seed, int id) :
  _seedColor(color), _seed(seed), _id(id)
{
}

Superpixel::~Superpixel()
{
}

void Superpixel::addPixel(Pointi pt)
{
  _pts.push_back(pt);
}

int Superpixel::getID()
{
  return _id;
}

Pointi Superpixel::getSeed()
{
  return _seed;
}

ConstraintData::ConstraintData() : _locked(false), _verboseDebugMode(false)
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

vector<PixelConstraint> ConstraintData::getPixelConstraints(Context & c)
{
  auto flattened = flatten();
  vector<ConnectedComponent> cc = computeConnectedComponents(flattened.second, flattened.first);

  return vector<PixelConstraint>();
}

pair<Grid2D<ConstraintType>, shared_ptr<Image>> ConstraintData::flatten()
{
  // assume rawInput here actually has something in it. This function won't
  // be called otherwise, since the getPixelConstraints should just return basically
  // nothing.
  int w = _rawInput.begin()->second->getWidth();
  int h = _rawInput.begin()->second->getHeight();

  shared_ptr<Image> flattenedImg = shared_ptr<Image>(new Image(w, h));
  Grid2D<ConstraintType> constraintMap(w, h);
  constraintMap.clear(ConstraintType::NO_CONSTRAINT_DEFINED);

  // first we should input all color constraints. the order here is going to be
  // alphabetical due to map sorting. If at some point we want layer ordering
  // another vector will need to be added to this.
  for (auto& t : _type) {
    // just overwrite the colors in the flattened image if alpha > 0
    // if the width and heights aren't equal this is going to crash so just don't do that
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        // if the pixel we're looking at is already fixed, nothing can overwrite that
        if (constraintMap(x, y) == ConstraintType::FIXED)
          continue;

        RGBAColor c = _rawInput[t.first]->getPixel(x, y);

        // alpha 0 indicates nothing was drawn in the specified area
        if (c._a > 0) {
          // we discard the alpha channel from these things since it appears that the canvas
          // actually dithers for us
          flattenedImg->setPixel(x, y, c._r, c._g, c._b, 1);
          constraintMap(x, y) = t.second;
        }
      }
    }
  }

  if (_verboseDebugMode)
  {
    flattenedImg->save("pxConstraint_flattened.png");
    getLogger()->log(constraintMap.toString(), LogLevel::DBG);
  }

  return pair<Grid2D<ConstraintType>, shared_ptr<Image>>(constraintMap, flattenedImg);
}

vector<ConnectedComponent> ConstraintData::computeConnectedComponents(shared_ptr<Image> pixels, Grid2D<ConstraintType>& typeMap)
{
  vector<ConnectedComponent> ccs;

  shared_ptr<Image> pxLog = shared_ptr<Image>(new Image(pixels->getWidth(), pixels->getHeight()));
  Grid2D<int> visited(pxLog->getWidth(), pxLog->getHeight());
  visited.clear(0);
  int componentID = 0;

  // basically does a dfs at every pixel. if the pixel has already been visited we skip it
  // (each pixel only visited once). The dfs will record which pixels it visits in a new image.
  // pixels are considered connected and the dfs will affect them if they are a) the same type
  // and b) (for color only so far) the same color
  for (int y = 0; y < pixels->getHeight(); y++) {
    for (int x = 0; x < pixels->getWidth(); x++) {
      if (visited(x, y) == 0) {
        // start a DFS from this point with a new log image
        shared_ptr<Image> component = shared_ptr<Image>(new Image(pixels->getWidth(), pixels->getHeight()));
        recordedDFS(x, y, visited, component, pixels, typeMap);

        // add a new connected component
        ConnectedComponent newComp;
        newComp._id = componentID;
        newComp._pixels = component;
        newComp._type = typeMap(x, y);

        ccs.push_back(newComp);
        componentID++;
      }
    }
  }

  if (_verboseDebugMode) {
    // save components
    for (auto& c : ccs) {
      c._pixels->save("pxConstraint_component" + to_string(c._id) + ".png");
      getLogger()->log("Component " + to_string(c._id) + " has type " + to_string(c._type), Comp::DBG);
    }
  }

  return ccs;
}

void ConstraintData::recordedDFS(int originX, int originY, Grid2D<int>& visited, shared_ptr<Image>& pxLog,
  shared_ptr<Image>& src, Grid2D<ConstraintType>& typeMap)
{
  // stack based dfs
  vector<pair<int, int>> stack;
  stack.push_back(pair<int, int>(originX, originY));

  while (stack.size() != 0) {
    pair<int, int> current = stack.back();
    stack.pop_back();

    int x = current.first;
    int y = current.second;

    // on entering, we assume this is the first time we've seen this pixel
    visited(x, y) = 1;
    pxLog->setPixel(x, y, 1, 1, 1, 1);

    ConstraintType t = typeMap(x, y);
    RGBAColor c = src->getPixel(x, y);

    vector<pair<int, int>> neighbors = { { x + 1, y },{ x - 1, y },{ x, y + 1 },{ x, y - 1 } };

    for (auto& n : neighbors) {
      // bounds check
      if (n.first < 0 || n.first >= pxLog->getWidth() || n.second < 0 || n.second >= pxLog->getHeight())
        continue;

      // type and already visited check.
      if (visited(n.first, n.second) == 0 && t == typeMap(n.first, n.second)) {
        // color equality check (if using target color)
        if ((t == TARGET_COLOR && src->pxEq(x, y, n.first, n.second)) || t != TARGET_COLOR) {
          stack.push_back(pair<int, int>(n.first, n.second));
        }
      }
    }
  }
}

vector<Superpixel> ConstraintData::extractSuperpixels(ConnectedComponent component, int numSeeds)
{
  // initialize a grid of the assignment state and then pick seeds for the superpixels
  // assignment state is organized as follows:
  // -1: unassigned, -2: out of bounds, >0: assigned to superpixel n
  shared_ptr<Image> px = component._pixels;
  Grid2D<int> assignmentState(px->getWidth(), px->getHeight());
  vector<Pointi> coords;

  for (int y = 0; y < px->getHeight(); y++) {
    for (int x = 0; x < px->getWidth(); x++) {
      if (px->getPixel(x, y)._a > 0) {
        // if part of the component, add to coordinate list and assignment state
        assignmentState(x, y) = -1;
        coords.push_back(Pointi(x, y));
      }
      else {
        assignmentState(x, y) = -2;
      }
    }
  }

  if (numSeeds > coords.size()) {
    // TODO: return superpixels for each pixel in here since there are more seeds than pixels
  }

  // pick random points to be the things
  mt19937 rng(random_device());
  uniform_int_distribution<int> dist(0, coords.size());
  set<int> picked;
  vector<Superpixel> superpixels;

  while (picked.size() < numSeeds) {
    int i = dist(rng);

    if (picked.count(i) == 0) {
      picked.insert(i);
      Pointi seed = coords[i];
      Superpixel newSp(px->getPixel(seed._x, seed._y), seed, picked.size() - 1);
      superpixels.push_back(newSp);
    }
  }

  // have seeds, run superpixel extraction
  growSuperpixels(superpixels, px, assignmentState);
}

void ConstraintData::growSuperpixels(vector<Superpixel>& superpixels, shared_ptr<Image>& src, Grid2D<int>& assignmentState)
{
  priority_queue<AssignmentAttempt> queue;

  // insert seeds
  for (int i = 0; i < superpixels.size(); i++) {
    assignPixel(superpixels[i], src, assignmentState, queue, superpixels[i].getSeed());
  }
}

void ConstraintData::assignPixel(Superpixel & sp, shared_ptr<Image>& src, Grid2D<int>& assignmentState,
  priority_queue<AssignmentAttempt>& queue, Pointi & pt)
{
  // -1 indicates unassigned, if the state is already assigned or is out of bouds, return
  if (assignmentState(pt._x, pt._y) != -1) {
    return;
  }
  assignmentState(pt._x, pt._y) = sp.getID();
  sp.addPixel(pt);

  // add neighbors
  vector<int> dx = { -1, 1, 0, 0 };
  vector<int> dy = { 0, 0, -1, 1 };
  for (int i = 0; i < dx.size(); i++) {
    Pointi npt(pt._x + dx[i], pt._y + dy[i]);

    // if point is in bounds and also not yet assigned, add it to the queue
    if (isValid(npt._x, npt._y, src->getWidth(), src->getHeight()) && assignmentState(npt._x, npt._y) == -1) {
      AssignmentAttempt newEntry;
      newEntry._pt = npt;
      newEntry._score = sp.dist(src->getPixel(npt._x, npt._y), npt);
      newEntry._superpixelID = sp.getID();
      queue.push(newEntry);
    }
  }
}

bool ConstraintData::isValid(int x, int y, int width, int height)
{
  return (x > 0 && x < width && y > 0 && y < height);
}

}