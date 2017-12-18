#pragma once

#include "util.h"
#include "Image.h"

using namespace std;

namespace Comp {
  class PixelClickData {
  public:
    PixelClickData(int length);
    PixelClickData(const PixelClickData& other);
    PixelClickData& operator=(const PixelClickData& other);
    ~PixelClickData();

    void setLayerState(int index, bool active);
    bool isActivated(int index);

    void merge(PixelClickData& other);

    vector<bool> getActivations();

    int getCount();

    int length();

  private:
    vector<bool> _activations;
    int _count;
  };

  class ClickMap {
  public:
    // for exporting images for debugging/visualization from the map
    enum VisualizationType {
      CLUSTERS,
      LAYER_DENSITY
    };

    ClickMap(int w, int h, vector<string> order, map<string, shared_ptr<ImportanceMap>> maps);
    ~ClickMap();

    // initializes the map
    void init(float threshold, bool normalizeMaps);

    // iteratively improves the initial map 
    void compute(int targetDepth);

    // dump a debug image
    Image* visualize(VisualizationType t);

  private:
    // merges pixels with their neighbors
    void smooth(int radius = 1);
  
    // eliminates layers in pixels that are too deep for the
    // target depth
    void sparsify(int currentDepth);

    // given to the click map from the compositor
    map<string, shared_ptr<ImportanceMap>> _importanceMaps;

    // layer order info
    vector<string> _layerOrder;

    // ideally each cluser is represented by the entry in this vector pointing to the
    // same PixelClickData object. the map should start out with all independent
    // clusters and then they get merged and deleted
    vector<shared_ptr<PixelClickData>> _activations;

    inline int index(int x, int y) { return y * _w + x; };

    // dimensions
    int _w;
    int _h;

    // used to threshold the importance maps
    float _threshold;
  };
}