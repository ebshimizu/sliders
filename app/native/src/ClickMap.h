#pragma once

#include "util.h"
#include "Image.h"
#include <set>

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

    // returns a copy of the current state
    vector<bool> getActivations();

    // returns a list of the active layers
    vector<int> getActivationIds();

    // check activation status
    bool sameActivations(shared_ptr<PixelClickData> other);

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
      LAYER_DENSITY,
      UNIQUE_CLUSTERS
    };

    ClickMap(int w, int h, vector<string> order, map<string, shared_ptr<ImportanceMap>> maps, vector<bool> adjustments);
    ~ClickMap();

    // initializes the map
    void init(float threshold, bool normalizeMaps, bool includeAdjustmentLayers = false);

    // iteratively improves the initial map 
    void compute(int targetDepth);

    // dump a debug image
    Image* visualize(VisualizationType t);

    // layer list
    vector<string> activeLayers(int x, int y);

  private:
    // merges pixels with their neighbors
    void smooth();
  
    // eliminates layers in pixels that are too deep for the
    // target depth
    void sparsify(int currentDepth);

    // returns a set of the unique clusters in the map
    set<shared_ptr<PixelClickData>> getUniqueClusters();

    // given to the click map from the compositor, these are the original unmodified maps
    map<string, shared_ptr<ImportanceMap>> _srcImportanceMaps;

    // working set of maps after processing (if needed)
    map<string, shared_ptr<ImportanceMap>> _workingImportanceMaps;

    // if true indicates the layer in question is a pure adjustment layer
    vector<bool> _adjustmentLayers;

    // layer order info
    vector<string> _layerOrder;

    // ideally each cluser is represented by the entry in this vector pointing to the
    // same PixelClickData object. the map should start out with all independent
    // clusters and then they get merged and deleted
    vector<shared_ptr<PixelClickData>> _clickMap;

    inline int index(int x, int y) { return y * _w + x; };

    // dimensions
    int _w;
    int _h;
    
    // maximum depth on initialization
    int _maxDepth;

    // used to threshold the importance maps
    float _threshold;
  };
}