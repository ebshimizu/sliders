/*
Layer.h - Container for bitmap data representing an editable layer
author: Evan Shimizu
*/

#pragma once

#include "Logger.h"
#include "Image.h"

namespace Comp {
  // matches photoshop blend modes and should function the same way
  enum BlendMode {
    NORMAL
  };
  
  class Layer {
  public:
    // blank layer, doesn't refer to much really
    Layer();

    // Layers should be associated with an image
    Layer(string name, shared_ptr<Image> img);

    Layer(const Layer& other);
    Layer& operator=(const Layer& other);

    ~Layer();

    // replace layer image
    void setImage(shared_ptr<Image> src);

    unsigned int getWidth();
    unsigned int getHeight();

    shared_ptr<Image> getImage();

    // resets layer adjustments
    void reset();

    // visibility toggle of the layer
    bool _visible;

    // layer transparency, [0-100]
    float _opacity;
    
    // blending mode
    BlendMode _mode;

    // Layer name
    string _name;

  private:
    // initializes default layer settings
    void init(shared_ptr<Image> source);

    // layer adjustment options?
    // float _hue;
    // float _sat;
    // float _brightness;

    // pointer to image data, stored in compositor
    shared_ptr<Image> _image;
  };
}