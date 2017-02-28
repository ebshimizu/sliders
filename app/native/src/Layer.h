/*
Layer.h - Container for compositing settings for each image in the composition 
author: Evan Shimizu
*/

#pragma once

#include "Logger.h"
#include "Image.h"
#include "util.h"

#include <map>

namespace Comp {
  // matches photoshop blend modes and should function the same way
  enum BlendMode {
    NORMAL = 0,
    MULTIPLY = 1,
    SCREEN = 2,
    OVERLAY = 3,
    HARD_LIGHT = 4,
    SOFT_LIGHT = 5,
    LINEAR_DODGE = 6,
    COLOR_DODGE = 7,
    LINEAR_BURN = 8,
    LINEAR_LIGHT = 9,
    COLOR = 10
  };

  enum AdjustmentType {
    HSL = 0,      // expected params: hue [-180, 180], sat [-100, 100], light [-100, 100]
    LEVELS = 1,   // expected params: inMin, inMax, gamma, outMin, outMax (all optional, [0-255])
    CURVES = 2,   // Curves need more data, so the default map just contains a list of present channels
    EXPOSURE = 3, // params: exposure [-20, 20], offset [-0.5, 0.5], gamma [0.01, 9.99]
    GRADIENT = 4  // params: a gradient
  };

  class Layer {
  public:
    // blank layer, doesn't refer to much really
    Layer();

    // Layers should be associated with an image
    Layer(string name, shared_ptr<Image> img);

    // but some are adjustment layers, which also have transparency properties and blend modes...
    Layer(string name);

    Layer(const Layer& other);
    Layer& operator=(const Layer& other);

    ~Layer();

    // replace layer image
    void setImage(shared_ptr<Image> src);

    unsigned int getWidth();
    unsigned int getHeight();

    shared_ptr<Image> getImage();

    // opacity controls
    float getOpacity();
    void setOpacity(float val);

    string getName();

    // set name for layers, should only really be called from compositor
    void setName(string name);

    // blending mode
    BlendMode _mode;

    // resets layer adjustments
    void reset();

    // visibility toggle of the layer
    bool _visible;

    // Returns true if this is an adjustment layer
    bool isAdjustmentLayer();

    // adjustment settings
    map<string, float> getAdjustment(AdjustmentType type);
    void deleteAdjustment(AdjustmentType type);
    void deleteAllAdjustments();
    vector<AdjustmentType> getAdjustments();

    // adding adjustments can be done arbitrarily or with pre-defined functions
    // these functions overwrite existing values by default
    void addAdjustment(AdjustmentType type, string param, float val);
    void addHSLAdjustment(float hue, float sat, float light);
    void addLevelsAdjustment(float inMin, float inMax, float gamma = 1, float outMin = 0, float outMax = 255);
    void addCurvesChannel(string channel, Curve curve);
    void deleteCurvesChannel(string channel);
    Curve getCurveChannel(string channel);
    void addExposureAdjustment(float exp, float offset, float gamma);
    void addGradientAdjustment(Gradient grad);

    // somewhat of a debug function
    float evalCurve(string channel, float x);
    RGBColor evalGradient(float x);

  private:
    // initializes default layer settings
    void init(shared_ptr<Image> source);

    // layer transparency, [0-100]
    float _opacity;

    // Layer name.
    // Immutable due to how layers are modified in the compositor
    string _name;

    // Initialized on creation, true if is an adjustment layer,
    // false otherwise
    bool _adjustment;

    // layer adju    // adjustment settings
    // Note that if adjustment is true, this layer is an adjustment layer and applies to
    // the current composition. If adjustment is false, this layer itself has adjustments
    // and the adjustments apply to only this layer.
    map<AdjustmentType, map<string, float> > _adjustments;

    // curves yay
    map<string, Curve> _curves;

    // gradient yay
    Gradient _grad;

    // pointer to image data, stored in compositor
    shared_ptr<Image> _image;
  };
}