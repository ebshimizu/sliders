/*
Layer.h - Container for compositing settings for each image in the composition 
author: Evan Shimizu
*/

#pragma once

#include "Logger.h"
#include "Image.h"
#include "util.h"
#include "third_party/json/src/json.hpp"

#include <map>
#include <unordered_map>
#include <set>

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
    COLOR = 10,
    LIGHTEN = 11,
    DARKEN = 12,
    PIN_LIGHT = 13,
    COLOR_BURN = 14,
    VIVID_LIGHT = 15,
    PASS_THROUGH = 16
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

    // layer mask
    void setMask(shared_ptr<Image> mask);
    bool hasMask();

    unsigned int getWidth();
    unsigned int getHeight();

    shared_ptr<Image> getImage();
    shared_ptr<Image> getMask();

    // opacity controls
    float getOpacity();
    void setOpacity(float val);

    // conditional blending
    void setConditionalBlend(string channel, map<string, float> settings);
    bool shouldConditionalBlend();
    const string& getConditionalBlendChannel();
    const map<string, float>& getConditionalBlendSettings();

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
    // if the layer is a precomp layer, this is automatically false
    bool isAdjustmentLayer();

    // adjustment settings
    map<string, float> getAdjustment(AdjustmentType type);
    void deleteAdjustment(AdjustmentType type);
    void deleteAllAdjustments();
    vector<AdjustmentType> getAdjustments();

    // adding adjustments can be done arbitrarily or with pre-defined functions
    // these functions overwrite existing values by default
    void addAdjustment(AdjustmentType type, string param, float val);
    void addAdjustment(AdjustmentType type, map<string, float> vals);
    void addHSLAdjustment(float hue, float sat, float light);
    void addLevelsAdjustment(float inMin, float inMax, float gamma = (1.0f / 10.0f), float outMin = 0, float outMax = 1);
    void addCurvesChannel(string channel, Curve curve);
    void deleteCurvesChannel(string channel);
    Curve getCurveChannel(string channel);
    void addExposureAdjustment(float exp, float offset, float gamma);
    void addGradientAdjustment(Gradient grad);
    void addSelectiveColorAdjustment(bool relative, map<string, map<string, float > > data);
    float getSelectiveColorChannel(string channel, string param);
    void setSelectiveColorChannel(string channel, string param, float val);
    void addColorBalanceAdjustment(bool preserveLuma, float shadowR, float shadowG, float shadowB,
      float midR, float midG, float midB,
      float highR, float highG, float highB);
    void addPhotoFilterAdjustment(bool preserveLuma, float r, float g, float b, float d);
    void addColorAdjustment(float r, float g, float b, float a);
    void addLighterColorAdjustment(float r, float g, float b, float a);
    void addOverwriteColorAdjustment(float r, float g, float b, float a);
    void addInvertAdjustment();
    void addBrightnessAdjustment(float b, float c);

    template <typename T>
    inline T evalCurve(string channel, T x) {
      if (_curves.count(channel) > 0) {
        return _curves[channel].eval(x);
      }

      return x;
    }

    template <typename T>
    inline typename Utils<T>::RGBColorT evalGradient(T x) {
      if (_adjustments.count(AdjustmentType::GRADIENT) > 0) {
        return _grad.eval(x);
      }

      return Utils<T>::RGBColorT();
    }

    map<string, map<string, float>> getSelectiveColor();
    Gradient& getGradient();

    // resets image to white with alpha 1
    void resetImage();

    // photoshop typestring
    string _psType;

    // By default we don't do anything with the _exp* fields. When generating the
    // expression tree this function should be called on all layers in the context
    // to convert all the adjustment values into expression variables
    // returns the next available variable index
    int prepExp(ExpContext& context, int start);

    // for the test harness, fill in params in the proper order
    void prepExpParams(vector<double>& params);

    // for ceres parameter export
    void addParams(nlohmann::json& paramList);

    // For the expression context
    map<AdjustmentType, map<string, ExpStep> > _expAdjustments;
    map<string, map<string, ExpStep> > _expSelectiveColor;
    ExpStep _expOpacity;

    void setOffset(float x, float y);
    pair<float, float> getOffset();
    void resetOffset();

    void setPrecompOrder(vector<string> order);
    vector<string> getPrecompOrder();
    bool isPrecomp();

    // note that local overrides don't have to use every single adjustment,
    // as in they can specify a partial order
    void setLocalAdjOverride(vector<AdjustmentType> order);
    vector<AdjustmentType> getLocalAdjOverride();
    bool hasLocalAdjOverride();

    void setLocalSelectionGroupOverride(vector<string> order);
    vector<string> getLocalSelectionGroupOverride();
    bool hasLocalSelectionGroupOverride();

  private:
    // initializes default layer settings
    void init(shared_ptr<Image> source);

    // layer transparency, [0-100]
    float _opacity;

    // Layer name.
    string _name;

    // Initialized on creation, true if is an adjustment layer,
    // false otherwise
    bool _adjustment;

    // adjustment settings
    // Note that if adjustment is true, this layer is an adjustment layer and applies to
    // the current composition. If adjustment is false, this layer itself has adjustments
    // and the adjustments apply to only this layer.
    map<AdjustmentType, map<string, float> > _adjustments;
    
    // curves yay
    map<string, Curve> _curves;

    // gradient yay
    Gradient _grad;

    // selective color yay?
    map<string, map<string, float> > _selectiveColor;

    // conditional blending
    string _cbChannel;
    map<string, float> _cbSettings;

    // pointer to image data, stored in compositor
    shared_ptr<Image> _image;

    // pointer to mask data, also stored in compositor
    shared_ptr<Image> _mask;

    float _offsetX;
    float _offsetY;

    // if the layer is a precomposed group, we'll need to render that.
    // this is just a recursive render call using the precomposition order
    // stored in this layer (same context)
    vector<string> _precompOrder;

    // local ordering
    vector<AdjustmentType> _localAdjOrderOverride;
    vector<string> _localSelectionGroupOverride;
  };

  typedef map<string, Layer> Context;
}