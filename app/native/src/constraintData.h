#pragma once

#include "Image.h"
#include "util.h"

using namespace std;

namespace Comp {
  enum ConstraintType {
    COLOR = 0,
    HUE = 1,
    FIXED = 2
  };

  class ConstraintData {
  public:
    ConstraintData();
    ~ConstraintData();

    // updates the mask data for the specified layer name
    void setMaskLayer(string layerName, shared_ptr<Image> img, ConstraintType type);

    // Returns the input image for the specified layer
    // returned image is null if layer does not exist
    shared_ptr<Image> getRawInput(string layerName);

    // deletes the specified mask layer
    void deleteMaskLayer(string layerName);

    // deletes everything in the data object
    void deleteAllData();

    // if the constraint data is locked, no changes can occur
    // typically this will be locked down during a search
    bool _locked;
  private:
    map<string, shared_ptr<Image> > _rawInput;
    map<string, ConstraintType> _type;
  };
}