#pragma once

#include "Image.h"
#include "util.h"
#include "Layer.h"
#include <random>
#include <set>
#include <queue>

using namespace std;

namespace Comp {
  enum ConstraintType {
    TARGET_COLOR = 0,
    TARGET_HUE = 1,
    FIXED = 2,
    NO_CONSTRAINT_DEFINED = 3,  // internal type (mostly)
    TARGET_HUE_SAT = 4,
    TARGET_BRIGHTNESS = 6
  };

  struct PixelConstraint {
    Point _pt;
    RGBColor _color;
    float _weight;
    ConstraintType _type;
  };

  struct ConnectedComponent {
    int _id;
    ConstraintType _type;
    shared_ptr<Image> _pixels;
    int _pixelCount;
  };

  struct AssignmentAttempt {
    double _score;
    Pointi _pt;
    int _superpixelID;
  };

  bool operator<(const AssignmentAttempt &a, const AssignmentAttempt &b);

  class Superpixel {
  public:
    Superpixel(RGBAColor color, Pointi seed, int id);
    ~Superpixel();

    void addPixel(Pointi pt);
    double dist(RGBAColor color, Pointi pt);
    void setScale(float xy, int w, int h);
    void recenter();
    void reset(shared_ptr<Image>& src);

    int getID();
    void setID(int id);
    Pointi getSeed();
    vector<Pointi> getPoints() { return _pts; }
    int _constraintID;

  private:
    vector<Pointi> _pts;
    Pointi _seed;
    RGBAColor _seedColor;
    int _id;

    float _xyScale;
    int _w;
    int _h;
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
    vector<PixelConstraint> getPixelConstraints(Context& c, shared_ptr<Image>& currentRender);

    // saves a debug image that computes the error for each constraint and renders it in a superpixel
    // user should ensure that constraints are updated before calling this, though the function will try
    // its best to not crash
    void computeErrorMap(shared_ptr<Image>& currentRender, string filename);

    // if the constraint data is locked, no changes can occur
    // typically this will be locked down during a search
    bool _locked;

    // If enabled, this will output a bunch of intermediate files for debugging.
    // should probably disable for most runs of this program
    bool _verboseDebugMode;

    // number of iterations to use for superpixel extraction
    // defaults to 5
    int _superpixelIters;

    // superpixel budgets, density is pixels per sample point
    int _unconstDensity;
    int _constDensity;

    // weights for different groups of pixels
    float _totalUnconstrainedWeight;
    float _totalConstrainedWeight;

  private:
    // flattens the layers down such that each constraint type applies to exactly one pixel
    pair<Grid2D<ConstraintType>, shared_ptr<Image> > flatten();

    // computes connected components (i.e. regions that are adjacent, have same type, and same color if applicable)
    vector<ConnectedComponent> computeConnectedComponents(shared_ptr<Image> pixels, Grid2D<ConstraintType>& typeMap);

    // performs a dfs search from a specific point to determine the connected components
    void recordedDFS(int x, int y, Grid2D<int>& visited, shared_ptr<Image>& pxLog,
      shared_ptr<Image>& src, Grid2D<ConstraintType>& typeMap);

    // extracts superpixels from connected components
    vector<Superpixel> extractSuperpixels(ConnectedComponent& component, int numSeeds, shared_ptr<Image>& px);

    // adds pixels to the superpixels
    void growSuperpixels(vector<Superpixel>& superpixels, shared_ptr<Image>& src, Grid2D<int>& assignmentState);

    // assigns a single pixel to the superpixel
    void assignPixel(Superpixel& sp, shared_ptr<Image>& src, Grid2D<int>& assignmentState,
      priority_queue<AssignmentAttempt>& queue, Pointi& pt);

    bool isValid(int x, int y, int width, int height);

    map<string, shared_ptr<Image> > _rawInput;
    map<string, ConstraintType> _type;

    vector<Superpixel> _extractedSuperpixels;
    vector<PixelConstraint> _extractedConstraints;
  };
}