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

double Superpixel::dist(RGBAColor color, Pointi pt)
{
  float scaledPtx = (pt._x / (float)_w) * _xyScale;
  float scaledPty = (pt._y / (float)_h) * _xyScale;
  float scaledSeedx = (_seed._x / (float)_w) * _xyScale;
  float scaledSeedy = (_seed._y / (float)_h) * _xyScale;

  return sqrt(
    pow(scaledPtx - scaledSeedx, 2) +
    pow(scaledPty - scaledSeedy, 2) +
    pow(color._r * color._a - _seedColor._r * _seedColor._a, 2) +
    pow(color._g * color._a - _seedColor._g * _seedColor._a, 2) +
    pow(color._b * color._a - _seedColor._b * _seedColor._a, 2)
  );
}

void Superpixel::setScale(float xy, int w, int h)
{
  _xyScale = xy;
  _w = w;
  _h = h;
}

void Superpixel::recenter()
{
  Point center;

  for (auto& p : _pts) {
    center._x += p._x;
    center._y += p._y;
  }

  center._x /= float(_pts.size());
  center._y /= float(_pts.size());

  Pointi bestCoord = _pts[0];
  float bestDistSq = 100000000.0f;
  for (auto &p : _pts)
  {
    float curDistSq = sqrt(pow(center._x - p._x, 2) + pow(center._y - p._y, 2));
    if (curDistSq < bestDistSq)
    {
      bestDistSq = curDistSq;
      bestCoord = p;
    }
  }

  _seed = bestCoord;
}

void Superpixel::reset(shared_ptr<Image>& src)
{
  _pts.clear();
  _seedColor = src->getPixel(_seed._x, _seed._y);
}

int Superpixel::getID()
{
  return _id;
}

void Superpixel::setID(int id)
{
  _id = id;
}

Pointi Superpixel::getSeed()
{
  return _seed;
}

ConstraintData::ConstraintData() : _locked(false), _verboseDebugMode(false), _superpixelIters(5),
  _unconstDensity(1000), _constDensity(250), _totalUnconstrainedWeight(1), _totalConstrainedWeight(4)
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

vector<PixelConstraint> ConstraintData::getPixelConstraints(Context& c, shared_ptr<Image>& currentRender)
{
  auto flattened = flatten();
  vector<ConnectedComponent> cc = computeConnectedComponents(flattened.second, flattened.first);

  int totalConstrained = 0;
  int totalUnconstrained = 0;

  // cound the pixels in each component
  for (auto& c : cc) {
    auto& data = c._pixels->getData();
    c._pixelCount = 0;
    for (int i = 0; i < data.size() / 4; i++) {
      // alpha 1 indicates pixel belongs to the specified component
      if (data[i * 4 + 3] == 255)
        c._pixelCount++;
    }

    if (c._type == NO_CONSTRAINT_DEFINED) {
      totalUnconstrained += c._pixelCount;
    }
    else {
      totalConstrained += c._pixelCount;
    }

    if (_verboseDebugMode) {
      getLogger()->log("Component " + to_string(c._id) + " size: " + to_string(c._pixelCount), Comp::DBG);
    }
  }

  vector<Superpixel> superpixels;
  for (auto& c : cc) {
    int budget;

    if (c._type == NO_CONSTRAINT_DEFINED) {
      budget = (int) (c._pixelCount / _unconstDensity);
    }
    else {
      budget = (int) (c._pixelCount / _constDensity);
    }

    if (budget == 0)
      budget = 1;

    auto res = extractSuperpixels(c, budget, currentRender);
    for (auto& sp : res) {
      sp._constraintID = c._id;
      sp.setID(sp.getID() + superpixels.size());
      superpixels.push_back(sp);
    }
  }

  if (_verboseDebugMode) {
    // draw the superpixels
    Image spImg(flattened.second->getWidth(), flattened.second->getHeight());
    for (auto& sp : superpixels) {
      // color assignment
      // each constraint has a hue range and a specific value associated with it
      // saturation and hue are allowed to vary within a particular constraint to show
      // the pixel boundaries
      float baseHue = (360 / 4) * cc[sp._constraintID]._type;

      float hue = baseHue + (30 / 3) * (sp.getID() % 10);
      float sat = 1 - (0.1f * ((sp.getID() / 10) % 10));
      float val = 0.5f;

      RGBColor spColor = Utils<float>::HSLToRGB(hue, sat, val);

      for (auto& p : sp.getPoints()) {
        spImg.setPixel(p._x, p._y, spColor._r, spColor._g, spColor._b, 1);
      }
    }

    currentRender->save("pxConstraint_src.png");
    spImg.save("pxConstraint_superpixels.png");
  }

  // process superpixels into constraints
  int unconstr = 0;
  int constr = 0;

  vector<PixelConstraint> constraints;
  for (auto& sp : superpixels) {
    PixelConstraint pc;
    pc._pt = Point(sp.getSeed()._x, sp.getSeed()._y);
    pc._type = cc[sp._constraintID]._type;

    if (cc[sp._constraintID]._type == TARGET_COLOR) {
      RGBAColor c = flattened.second->getPixel(sp.getSeed()._x, sp.getSeed()._y);
      RGBColor cmult;
      cmult._r = c._r * c._a;
      cmult._g = c._g * c._a;
      cmult._b = c._b * c._a;

      pc._color = cmult;
    }
    else if (cc[sp._constraintID]._type == TARGET_HUE) {
      RGBAColor target = flattened.second->getPixel(sp.getSeed()._x, sp.getSeed()._y);
      RGBAColor base = currentRender->getPixel(sp.getSeed()._x, sp.getSeed()._y);

      // extract the hue and then shift the base color to match the target hue.
      HSYColor targetHSY = Utils<float>::RGBToHSY(target._r * target._a, target._g * target._a, target._b * target._a);
      HSYColor baseHSY = Utils<float>::RGBToHSY(base._r * base._a, base._g * base._a, base._b * base._a);

      baseHSY._h = targetHSY._h;
      auto rgbRes = Utils<float>::HSYToRGB(baseHSY);
      pc._color._r = clamp(rgbRes._r, 0.0f, 1.0f);
      pc._color._g = clamp(rgbRes._g, 0.0f, 1.0f);
      pc._color._b = clamp(rgbRes._b, 0.0f, 1.0f);
    }
    else if (cc[sp._constraintID]._type == TARGET_HUE_SAT) {
      RGBAColor target = flattened.second->getPixel(sp.getSeed()._x, sp.getSeed()._y);
      RGBAColor base = currentRender->getPixel(sp.getSeed()._x, sp.getSeed()._y);

      // extract the hue and saturation and then shift the base color to match the target hue.
      HSYColor targetHSY = Utils<float>::RGBToHSY(target._r * target._a, target._g * target._a, target._b * target._a);
      HSYColor baseHSY = Utils<float>::RGBToHSY(base._r * base._a, base._g * base._a, base._b * base._a);

      baseHSY._h = targetHSY._h;
      baseHSY._s = targetHSY._s;
      auto rgbRes = Utils<float>::HSYToRGB(baseHSY);
      pc._color._r = clamp(rgbRes._r, 0.0f, 1.0f);
      pc._color._g = clamp(rgbRes._g, 0.0f, 1.0f);
      pc._color._b = clamp(rgbRes._b, 0.0f, 1.0f);
    }
    else if (cc[sp._constraintID]._type == TARGET_BRIGHTNESS) {
      RGBAColor target = flattened.second->getPixel(sp.getSeed()._x, sp.getSeed()._y);
      RGBAColor base = currentRender->getPixel(sp.getSeed()._x, sp.getSeed()._y);

      // extract the brightness and shift to match
      HSYColor targetHSY = Utils<float>::RGBToHSY(target._r * target._a, target._g * target._a, target._b * target._a);
      HSYColor baseHSY = Utils<float>::RGBToHSY(base._r * base._a, base._g * base._a, base._b * base._a);

      baseHSY._y = targetHSY._y;
      auto rgbRes = Utils<float>::HSYToRGB(baseHSY);
      pc._color._r = clamp(rgbRes._r, 0.0f, 1.0f);
      pc._color._g = clamp(rgbRes._g, 0.0f, 1.0f);
      pc._color._b = clamp(rgbRes._b, 0.0f, 1.0f);
    }
    else if (cc[sp._constraintID]._type == FIXED || cc[sp._constraintID]._type == NO_CONSTRAINT_DEFINED) {
      RGBAColor c = currentRender->getPixel(sp.getSeed()._x, sp.getSeed()._y);
      RGBColor cmult;
      cmult._r = c._r * c._a;
      cmult._g = c._g * c._a;
      cmult._b = c._b * c._a;

      pc._color = cmult;
    }

    if (cc[sp._constraintID]._type == NO_CONSTRAINT_DEFINED) {
      unconstr++;
    }
    else {
      constr++;
    }

    constraints.push_back(pc);
  }


  if (_verboseDebugMode) {
    // draw the superpixels again, this time color is the constraint color defined.
    Image spImg(flattened.second->getWidth(), flattened.second->getHeight());
    for (int i = 0; i < superpixels.size(); i++) {
      // color assignment
      RGBColor spColor = constraints[i]._color;

      for (auto& p : superpixels[i].getPoints()) {
        spImg.setPixel(p._x, p._y, spColor._r, spColor._g, spColor._b, 1);
      }
    }

    spImg.save("pxConstraint_constraints.png");
  }

  // weight calculations
  for (int i = 0; i < constraints.size(); i++) {
    if (constraints[i]._type == NO_CONSTRAINT_DEFINED) {
      constraints[i]._weight = _totalUnconstrainedWeight / unconstr;
    }
    else {
      constraints[i]._weight = _totalConstrainedWeight / constr;
    }
  }

  // save constraints for future use
  _extractedSuperpixels = superpixels;
  _extractedConstraints = constraints;

  return constraints;
}

void ConstraintData::computeErrorMap(shared_ptr<Image>& currentRender, string filename)
{
  Image errorImg(currentRender->getWidth(), currentRender->getHeight());

  for (int i = 0; i < _extractedSuperpixels.size(); i++) {
    // get the target color
    RGBColor target = _extractedConstraints[i]._color;
    auto labTarget = Utils<float>::RGBToLab(target);
    Point loc = _extractedConstraints[i]._pt;

    // bounds check
    if (loc._x < 0 || loc._x > currentRender->getWidth() || loc._y < 0 || loc._y > currentRender->getHeight())
      continue;

    // get the current color
    RGBAColor rendered = currentRender->getPixel((int)loc._x, (int)loc._y);
    auto labRendered = Utils<float>::RGBToLab(rendered._r * rendered._a, rendered._g * rendered._a, rendered._b * rendered._a);

    // compute the difference and assign a color
    Utils<float>::LabColorT errorColor;
    errorColor._L = pow(labRendered._L - labTarget._L, 2);
    errorColor._a = pow(labRendered._a - labTarget._a, 2);
    errorColor._b = pow(labRendered._b - labTarget._b, 2);

    // scaling?
    RGBColor displayColor;
    float scaledErr = clamp(sqrt(errorColor._L + errorColor._a + errorColor._b) / 100, 0.0f, 1.0f);
    displayColor._r = scaledErr;
    displayColor._g = scaledErr;
    displayColor._b = scaledErr;

    // put the error per channel into the superpixel pixels
    for (auto& p : _extractedSuperpixels[i].getPoints()) {
      errorImg.setPixel(p._x, p._y, displayColor._r, displayColor._g, displayColor._b, 1);
    }
  }

  // save
  errorImg.save(filename);
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

        // the canvas anti-aliases so to prevent the constraint generator from generating
        // 1000s of disconnected regions, we discard all pixels with less than 1 alpha
        if (c._a == 1) {
          flattenedImg->setPixel(x, y, c._r, c._g, c._b, 1);
          constraintMap(x, y) = t.second;
        }
      }
    }
  }

  if (_verboseDebugMode)
  {
    flattenedImg->save("pxConstraint_flattened.png");
    //getLogger()->log(constraintMap.toString(), LogLevel::DBG);
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
        // color equality check (if using target color or hue)
        if (t == TARGET_COLOR || t == TARGET_HUE || t == TARGET_HUE_SAT) {
          if (src->pxEq(x, y, n.first, n.second)) {
            stack.push_back(pair<int, int>(n.first, n.second));
          }
        }
        else {
          stack.push_back(pair<int, int>(n.first, n.second));
        }
      }
    }
  }
}

vector<Superpixel> ConstraintData::extractSuperpixels(ConnectedComponent& component, int numSeeds, shared_ptr<Image>& px)
{
  // initialize a grid of the assignment state and then pick seeds for the superpixels
  // assignment state is organized as follows:
  // -1: unassigned, -2: out of bounds, >0: assigned to superpixel n
  Grid2D<int> assignmentState(px->getWidth(), px->getHeight());
  vector<Pointi> coords;
  shared_ptr<Image> ccBounds = component._pixels;

  for (int y = 0; y < px->getHeight(); y++) {
    for (int x = 0; x < px->getWidth(); x++) {
      if (ccBounds->getPixel(x, y)._a > 0) {
        // if part of the component, add to coordinate list and assignment state
        assignmentState(x, y) = -1;
        coords.push_back(Pointi(x, y));
      }
      else {
        assignmentState(x, y) = -2;
      }
    }
  }

  vector<Superpixel> superpixels;
  if (numSeeds > coords.size()) {
    for (int i = 0; i < coords.size(); i++) {
      Superpixel newSp(px->getPixel(coords[i]._x, coords[i]._y), coords[i], i);
      superpixels.push_back(newSp);
    }

    return superpixels;
  }

  Grid2D<int> initState = assignmentState;

  // pick random points to be the things
  random_device rd;
  mt19937 rng(rd());
  uniform_int_distribution<int> dist(0, coords.size() - 1);
  set<int> picked;

  while (picked.size() < numSeeds) {
    int i = dist(rng);

    if (picked.count(i) == 0) {
      picked.insert(i);
      Pointi seed = coords[i];
      Superpixel newSp(px->getPixel(seed._x, seed._y), seed, picked.size() - 1);
      newSp.setScale(1.5, px->getWidth(), px->getHeight());
      superpixels.push_back(newSp);
    }
  }

  for (int i = 0; i < _superpixelIters; i++) {
    // have seeds, run superpixel extraction
    growSuperpixels(superpixels, px, assignmentState);

    // recenter
    for (auto& sp : superpixels) {
      sp.recenter();
      sp.reset(px);
    }

    // reinit
    assignmentState = initState;

    if (_verboseDebugMode) {
      getLogger()->log("Superpixel centers after iteration " + to_string(i), LogLevel::DBG);

      for (auto& sp : superpixels) {
        Pointi seed = sp.getSeed();
        getLogger()->log("ID " + to_string(sp.getID()) + ": (" + to_string(seed._x) + "," + to_string(seed._y) + ")", LogLevel::DBG);
      }
    }
  }

  // run one more time
  growSuperpixels(superpixels, px, assignmentState);
  for (auto& sp : superpixels)
    sp.recenter();

  if (_verboseDebugMode) {
    getLogger()->log("Superpixel centers after final iteration", LogLevel::DBG);

    for (auto& sp : superpixels) {
      Pointi seed = sp.getSeed();
      getLogger()->log("ID " + to_string(sp.getID()) + ": (" + to_string(seed._x) + "," + to_string(seed._y) + ")", LogLevel::DBG);
    }
  }

  return superpixels;
}

void ConstraintData::growSuperpixels(vector<Superpixel>& superpixels, shared_ptr<Image>& src, Grid2D<int>& assignmentState)
{
  priority_queue<AssignmentAttempt> queue;

  // insert seeds
  for (int i = 0; i < superpixels.size(); i++) {
    assignPixel(superpixels[i], src, assignmentState, queue, superpixels[i].getSeed());
  }

  while (!queue.empty()) {
    AssignmentAttempt aa = queue.top();
    queue.pop();
    assignPixel(superpixels[aa._superpixelID], src, assignmentState, queue, aa._pt);
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

bool operator<(const AssignmentAttempt & a, const AssignmentAttempt & b)
{
  // note that this is reversed here just because I'm lazy and don't want
  // to replace the priority queue type with a really long templated type name
  return (a._score > b._score);
}

}