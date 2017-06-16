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

  struct AssignmentAttempt {
    double _score;
    Pointi _pt;
  };

  bool operator<(const AssignmentAttempt &a, const AssignmentAttempt &b)
  {
    return (a._score < b._score);
  }

  class Superpixel {
  public:
    Superpixel(RGBAColor color, Pointi seed, int id);
    ~Superpixel();

    void addPixel(Pointi pt);
    double dist(RGBAColor color, Pointi pt);

    int getID();
    Pointi getSeed();

  private:
    vector<Pointi> _pts;
    Pointi _seed;
    RGBAColor _seedColor;
    int _id;
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
    // flattens the layers down such that each constraint type applies to exactly one pixel
    pair<Grid2D<ConstraintType>, shared_ptr<Image> > flatten();

    // computes connected components (i.e. regions that are adjacent, have same type, and same color if applicable)
    vector<ConnectedComponent> computeConnectedComponents(shared_ptr<Image> pixels, Grid2D<ConstraintType>& typeMap);

    // performs a dfs search from a specific point to determine the connected components
    void recordedDFS(int x, int y, Grid2D<int>& visited, shared_ptr<Image>& pxLog,
      shared_ptr<Image>& src, Grid2D<ConstraintType>& typeMap);

    // extracts superpixels from connected components
    vector<Superpixel> extractSuperpixels(ConnectedComponent component, int numSeeds);

    // adds pixels to the superpixels
    void growSuperpixels(vector<Superpixel>& superpixels, shared_ptr<Image>& src, Grid2D<int>& assignmentState);

    // assigns a single pixel to the superpixel
    void assignPixel(Superpixel& sp, shared_ptr<Image>& src, Grid2D<int>& assignmentState,
      priority_queue<AssignmentAttempt>& queue, Pointi& pt);

    //void computeConnectedComponents();
    // auto extractSuperpixels();

    map<string, shared_ptr<Image> > _rawInput;
    map<string, ConstraintType> _type;
  };
}