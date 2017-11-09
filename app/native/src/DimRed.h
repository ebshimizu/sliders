#include "Compositor.h"

namespace Comp {

// the important parts of a sample
struct Sample {
  Context _ctx;
  Eigen::VectorXd _x;
  shared_ptr<Image> _img;
};

// base class constaining some common elements of the available dimensionality reduction
// methods. expects subclasses to implement some details
// Compositor object not owned by this object, expects deletion for that somewhere else
class DimensionalityReduction {
public:
  DimensionalityReduction(Compositor *c);
  ~DimensionalityReduction();

  // projects a context into the reduction space
  virtual Eigen::VectorXd project(Context c) = 0;

  // reprojects a vector into a context
  virtual Context reproject(Eigen::VectorXd x) = 0;

  // renders and converts a context into the proper sample vector format
  // samples can't be deleted, just reset
  void setSamples(vector<Context> contexts);

  // computes the ssim similarity matrices on the current set of samples
  void computeSSIMSimilarity(float a, float b, float g, int patchSize, float scale = 1);

  vector<string> availableSimilarityMatrices();
  Eigen::MatrixXd getSimilarityMatrix(string name);
  void deleteSimilarityMatrix(string name);

protected:
  map<string, Eigen::MatrixXd> _similarityMatrices;

  vector<Sample> _samples;

  Compositor* _c;

  Eigen::VectorXd stdToEigen(vector<double>& x);
};

// possible embeddings:
// LLE
// Isomap
// GP-LVM
//class LLE : public DimensionalityReduction {
//
//};

}