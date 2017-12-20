#include "ClickMap.h"

namespace Comp {

PixelClickData::PixelClickData(int length)
{
  _activations.resize(length);

  for (int i = 0; i < _activations.size(); i++) {
    _activations[i] = false;
  }

  _count = 0;
}

PixelClickData::PixelClickData(const PixelClickData & other)
{
  _activations = other._activations;
  _count = other._count;
}

PixelClickData & PixelClickData::operator=(const PixelClickData & other)
{
  // self assignment check
  if (&other == this)
    return *this;

  _activations = other._activations;
  _count = other._count;
  return *this;
}

PixelClickData::~PixelClickData()
{
}

void PixelClickData::setLayerState(int index, bool active)
{
  if ((_activations[index] && active) || (!_activations[index] && !active)) {
    // this is a nop
    return;
  }

  _activations[index] = active;
  _count += (active) ? 1 : -1;
}

bool PixelClickData::isActivated(int index)
{
  return _activations[index];
}

void PixelClickData::merge(PixelClickData & other)
{
  // count reset
  _count = 0;

  // basically an or operation
  for (int i = 0; i < _activations.size(); i++) {
    _activations[i] = _activations[i] | other._activations[i];

    if (_activations[i])
      _count++;
  }
}

vector<bool> PixelClickData::getActivations()
{
  return _activations;
}

vector<int> PixelClickData::getActivationIds()
{
  vector<int> ret;

  for (int i = 0; i < _activations.size(); i++) {
    if (_activations[i]) {
      ret.push_back(i);
    }
  }

  return ret;
}

bool PixelClickData::sameActivations(shared_ptr<PixelClickData> other)
{
  for (int i = 0; i < _activations.size(); i++) {
    if (other->_activations[i] != _activations[i])
      return false;
  }

  return true;
}

int PixelClickData::getCount()
{
  return _count;
}

int PixelClickData::length()
{
  return _activations.size();
}

ClickMap::ClickMap(int w, int h, vector<string> order, map<string, shared_ptr<ImportanceMap>> maps, vector<bool> adjustments) :
  _w(w), _h(h), _layerOrder(order), _srcImportanceMaps(maps), _adjustmentLayers(adjustments)
{
  _clickMap.resize(_w * _h);
}

ClickMap::~ClickMap()
{
}

void ClickMap::init(float threshold, bool normalizeMaps, bool includeAdjustmentLayers)
{
  _threshold = threshold;

  for (auto& map : _srcImportanceMaps) {
    // duplicate
    _workingImportanceMaps[map.first] = shared_ptr<ImportanceMap>(new ImportanceMap(*(map.second.get())));

    if (normalizeMaps) {
      _workingImportanceMaps[map.first]->normalize();
    }
  }
  
  // ok so here we'll create the pixelclickdata for each pixel and initialize it with
  // the layers the are above the threshold
  // pixel data first, then layers in order
  for (int i = 0; i < _clickMap.size(); i++) {
    _clickMap[i] = shared_ptr<PixelClickData>(new PixelClickData(_layerOrder.size()));
  }

  // for each layer, check if the importance map is above threshold, if so, flip the
  // bit for the pixel
  for (int i = 0; i < _layerOrder.size(); i++) {
    // if it's an adjustment layer, compeltely skip it
    if (_adjustmentLayers[i])
      continue;

    string layerName = _layerOrder[i];
    shared_ptr<ImportanceMap> impMap = _workingImportanceMaps[layerName];

    getLogger()->log("[ClickMap] Initializing layer " + layerName + " (" + to_string(i) + ") ...");
    
    for (int p = 0; p < _clickMap.size(); p++) {
      if (impMap->_data[p] > _threshold) {
        _clickMap[p]->setLayerState(i, true);
      }
    }

    getLogger()->log("[ClickMap] Layer " + layerName + " (" + to_string(i) + ") Initialization Complete");
  }

  // check the max depth that we have
  _maxDepth = 0;
  for (int i = 0; i < _clickMap.size(); i++) {
    if (_clickMap[i]->getCount() > _maxDepth) {
      _maxDepth = _clickMap[i]->getCount();
    }
  }

  getLogger()->log("[ClickMap] Initialization complete. Max Depth: " + to_string(_maxDepth));
}

void ClickMap::compute(int targetDepth)
{
  // the main part of the click map, here we'll iteratively smooth and sparsify the map
  int currentDepth = _maxDepth;

  getLogger()->log("[ClickMap] Computation started. Current Depth: " + to_string(currentDepth) + ". Target Depth: " + to_string(targetDepth));

  while (currentDepth > targetDepth) {
    // reduce current depth
    currentDepth--;

    getLogger()->log("[ClickMap] Depth level " + to_string(currentDepth) + " of " + to_string(targetDepth));
    getLogger()->log("[ClickMap] Smooth computation start for depth level " + to_string(currentDepth));

    // merge
    smooth();

    getLogger()->log("[ClickMap] Smooth computation complete for depth level " + to_string(currentDepth));
    getLogger()->log("[ClickMap] Sparsify computation start for depth level " + to_string(currentDepth));

    // sparsify
    sparsify(currentDepth);

    getLogger()->log("[ClickMap] Sparsify computation complete for depth level " + to_string(currentDepth));
    getLogger()->log("[ClickMap] Depth level " + to_string(currentDepth) + " of " + to_string(targetDepth) + " computation complete.");
  }
}

Image* ClickMap::visualize(VisualizationType t)
{
  Image* ret = new Image(_w, _h);
  vector<unsigned char>& data = ret->getData();

  if (t == VisualizationType::CLUSTERS) {
    // need to index the number of unique objects then assign colors
    map<shared_ptr<PixelClickData>, float> clusters;

    for (int i = 0; i < _clickMap.size(); i++) {
      clusters[_clickMap[i]] = 0;
    }

    // color assignment, hue
    int i = 0;
    for (auto& c : clusters) {
      c.second = (360.0f / (clusters.size()) * i);
      i++;
    }

    // pixel assignment
    for (int i = 0; i < _clickMap.size(); i++) {
      int pxIndex = i * 4;

      float hue = clusters[_clickMap[i]];
      auto color = Utils<float>::HSLToRGB(hue, 1, 0.5);

      data[pxIndex] = color._r * 255;
      data[pxIndex + 1] = color._g * 255;
      data[pxIndex + 2] = color._b * 255;
      data[pxIndex + 3] = 255;
    }
  }
  else if (t == VisualizationType::UNIQUE_CLUSTERS) {
    // this time we work on the bitvectors
    map<vector<bool>, float> clusters;

    for (int i = 0; i < _clickMap.size(); i++) {
      clusters[_clickMap[i]->getActivations()] = 0;
    }

    // color assignment, hue
    int i = 0;
    for (auto& c : clusters) {
      c.second = (360.0f / (clusters.size()) * i);
      i++;
    }

    // pixel assignment
    for (int i = 0; i < _clickMap.size(); i++) {
      int pxIndex = i * 4;

      float hue = clusters[_clickMap[i]->getActivations()];
      auto color = Utils<float>::HSLToRGB(hue, 1, 0.5);

      data[pxIndex] = color._r * 255;
      data[pxIndex + 1] = color._g * 255;
      data[pxIndex + 2] = color._b * 255;
      data[pxIndex + 3] = 255;
    }
  }
  else if (t == VisualizationType::LAYER_DENSITY) {
    int vmax = 0;
    for (int i = 0; i < _clickMap.size(); i++) {
      if (_clickMap[i]->getCount() > vmax)
        vmax = _clickMap[i]->getCount();
    }

    for (int i = 0; i < _clickMap.size(); i++) {
      int pxIndex = i * 4;

      float color = (float)(_clickMap[i]->getCount()) / vmax;
      
      data[pxIndex] = color * 255;
      data[pxIndex + 1] = color * 255;
      data[pxIndex + 2] = color * 255;
      data[pxIndex + 3] = 255;
    }
  }

  return ret;
}

vector<string> ClickMap::activeLayers(int x, int y)
{
  // bounds check
  if (x < 0 || x >= _w || y < 0 || y >= _h)
    return vector<string>();

  vector<string> layers;
  int idx = index(x, y);

  // get layer names from the thing
  shared_ptr<PixelClickData> data = _clickMap[idx];
  vector<int> active = data->getActivationIds();

  for (int i = 0; i < active.size(); i++) {
    layers.push_back(_layerOrder[active[i]]);
  }

  return layers;
}

void ClickMap::smooth()
{
  // from top left to bottom right, check each pixel, see if it's literally identical
  // to the next one, and if so merge it
  // we should note here that this is hilariously bad and simplistic for merging so
  // it stands as a first implementation
  
  // 8 directional check, maybe redundant but eh
  vector<int> xdiff = { -1, 0, 1, -1, 1, -1, 0, 1 };
  vector<int> ydiff = { -1, -1, -1, 0, 0, 1, 1, 1 };

  for (int y = 0; y < _h; y++) {
    getLogger()->log("[ClickMap] smooth row: " + to_string(y) + "/" + to_string(_h), LogLevel::SILLY);

    for (int x = 0; x < _w; x++) {
      int current = index(x, y);

      for (int delta = 0; delta < xdiff.size(); delta++) {
        int dx = x + xdiff[delta];
        int dy = y + ydiff[delta];

        if (dx < 0 || dx >= _w || dy < 0 || dy >= _h)
          continue;

        int next = index(dx, dy);

        // if they're not literally the same pointer
        if (_clickMap[current] != _clickMap[next]) {
          // check for equality
          if (_clickMap[current]->sameActivations(_clickMap[next])) {
            // this should overwrite the pointer in next and it should be automatically destroyed
            _clickMap[next] = _clickMap[current];
          }
        }
      }
    }
  }
}

void ClickMap::sparsify(int currentDepth)
{
  getLogger()->log("[ClickMap] Sparsify depth " + to_string(currentDepth));

  // first, we need a list of the number of active pixels for each layer
  // this can be an array since everything is indexed by array id at this point
  vector<int> activePixelCount;
  activePixelCount.resize(_layerOrder.size());

  for (int i = 0; i < _clickMap.size(); i++) {
    for (int layerId = 0; layerId < _layerOrder.size(); layerId++) {
      if (_clickMap[i]->isActivated(layerId))
        activePixelCount[layerId] += 1;
    }
  }

  // log some data
  for (int i = 0; i < activePixelCount.size(); i++) {
    getLogger()->log("[ClickMap] layer " + _layerOrder[i] + " has " + to_string(activePixelCount[i]) + " active pixels");
  }

  // sparsify unique clusters
  set<shared_ptr<PixelClickData>> uniqueClusters = getUniqueClusters();

  int clusterNum = 0;
  for (auto& cluster : uniqueClusters) {
    getLogger()->log("[ClickMap] Sparsify cluster " + to_string(clusterNum) + "/" + to_string(uniqueClusters.size()), LogLevel::SILLY);

    // if we are above the current depth, we need to make sparse
    if (cluster->getCount() > currentDepth) {
      // of the affected layers, remove the one with the largest total count
      int max = 0;
      int id = -1;

      // logging string
      stringstream ss;
      ss << "[ClickMap] cluster " << cluster << " above max depth. culling...\n";
      ss << "gathering options:\n";

      vector<int> active = cluster->getActivationIds();
      for (auto& aid : active) {
        ss << _layerOrder[aid] << ": " << activePixelCount[aid] << "\n";

        if (activePixelCount[aid] > max) {
          max = activePixelCount[aid];
          id = aid;
        }
      }

      ss << _layerOrder[id] << " selected for removal with count " << activePixelCount[id] << "\n";
      cluster->setLayerState(id, false);

      getLogger()->log(ss.str(), LogLevel::SILLY);
      getLogger()->log("[ClickMap] recomputing layer activations...", LogLevel::SILLY);

      // recompute active pixel count
      activePixelCount.clear();
      activePixelCount.resize(_layerOrder.size());
      for (int i = 0; i < _clickMap.size(); i++) {
        for (int layerId = 0; layerId < _layerOrder.size(); layerId++) {
          if (_clickMap[i]->isActivated(layerId))
            activePixelCount[layerId] += 1;
        }
      }

      // log some data
      for (int i = 0; i < activePixelCount.size(); i++) {
        getLogger()->log("[ClickMap] layer " + _layerOrder[i] + " has " + to_string(activePixelCount[i]) + " active pixels", LogLevel::SILLY);
      }

      clusterNum++;
    }
  }
}

set<shared_ptr<PixelClickData>> ClickMap::getUniqueClusters()
{
  set<shared_ptr<PixelClickData>> uc;

  // so uh, basically just throw everything in like it's a set it'll do the thing
  for (int i = 0; i < _clickMap.size(); i++) {
    uc.insert(_clickMap[i]);
  }

  return uc;
}

}