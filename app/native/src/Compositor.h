/*
Compositor.h - An image compositing library
author: Evan Shimizu
*/

#pragma once

#include <vector>
#include <map>

#include "Image.h"
#include "Layer.h"

using namespace std;

namespace Comp {
  typedef map<string, Layer> Context;

  // the compositor for now assumes that every layer it contains have the same dimensions.
  // having unequal layer sizes will likely lead to crashes or other undefined behavior
  class Compositor {
  public:
    Compositor();
    ~Compositor();

    // adds a layer, true on success, false if layer already exists
    bool addLayer(string name, string file);
    bool addLayer(string name, Image& img);
    
    bool copyLayer(string src, string dest);

    bool deleteLayer(string name);

    // changes the sort order of a layer
    void reorderLayer(int from, int to);

    Layer& getLayer(int id);
    Layer& getLayer(string name);

    // Returns a copy of the current context
    Context getNewContext();

    // returns a ref to the primary context
    Context& getPrimaryContext();

    vector<string> getLayerOrder();

    // number of layers present in the compositor
    int size();

    // Render the primary context
    Image render();

    // render with a given context
    Image render(Context c);

    // render directly to a string with the primary context
    string renderToBase64();

    // render directly to a string with a given context
    string renderToBase64(Context c);

  private:
    void addLayer(string name);

    inline unsigned char premult(unsigned char px, float a);

    // compositing order for layers
    vector<string> _layerOrder;

    // Primary rendering context. This is the persistent state of the compositor.
    // Keyed by IDs in the context.
    Context _primary;

    // image data referenced by layers
    map<string, shared_ptr<Image> > _imageData;

    // eventually this class will need to render things in parallel

  };
}