#pragma once

#include "Image.h"
#include "util.h"
#include "Layer.h"

using namespace std;

namespace Comp {
  enum ConstraintType {
    TARGET_COLOR = 0,
    HUE = 1,
    FIXED = 2,
    NO_CONSTRAINT_DEFINED = 3   // internal type (mostly)
  };

  struct PixelConstraint {
    Point _pt;
    RGBColor _color;
  };

  struct ConnectedComponent {
    int _id;
    ConstraintType _type;
    shared_ptr<Image> _pixels;
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

    // computes the pixel constraints from the current state of the raw input and specified constraint types
    vector<PixelConstraint> getPixelConstraints(Context& c);

    // if the constraint data is locked, no changes can occur
    // typically this will be locked down during a search
    bool _locked;

    // If enabled, this will output a bunch of intermediate files for debugging.
    // should probably disable for most runs of this program
    bool _verboseDebugMode;
  private:
    // TODO: implement functions
    // flattens the layers down such that each constraint type applies to exactly one pixel
    pair<Grid2D<ConstraintType>, shared_ptr<Image> > flatten();
    vector<ConnectedComponent> computeConnectedComponents(shared_ptr<Image> pixels, Grid2D<ConstraintType>& typeMap);

    void recordedDFS(int x, int y, Grid2D<int>& visited, shared_ptr<Image>& pxLog,
      shared_ptr<Image>& src, Grid2D<ConstraintType>& typeMap);
    // vector<Superpixels> extractSuperpixels(vector<ConnectedComponent>, float pixelDensity);

    //void computeConnectedComponents();
    // auto extractSuperpixels();

    map<string, shared_ptr<Image> > _rawInput;
    map<string, ConstraintType> _type;
  };
}