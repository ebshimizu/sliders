#include "DimRed.h"

namespace Comp {

DimensionalityReduction::DimensionalityReduction(Compositor* c) : _c(c) {

}

DimensionalityReduction::~DimensionalityReduction() {
  // don't delete c! this object doesn't really manage that, it just needs A Compositor to work
}

void DimensionalityReduction::setSamples(vector<Context> contexts)
{
  _samples.clear();

  for (auto& ctx : contexts) {
    Sample s;
    s._ctx = ctx;

    // render at full size (can scale down if needed later)
    s._img = shared_ptr<Image>(_c->render(s._ctx));
    s._x = stdToEigen(_c->contextToVector(s._ctx));

    _samples.push_back(s);

    getLogger()->log("Added sample. Current count: " + to_string(_samples.size()));
  }
}

void DimensionalityReduction::computeSSIMSimilarity(float a, float b, float g, int patchSize, float scale)
{
  // computes the ssim on the current set of samples
  Eigen::MatrixXd ssim;
  ssim.resize(_samples.size(), _samples.size());

  // image cache for resizing
  vector<shared_ptr<Image>> scaledImages;

  for (int i = 0; i < _samples.size(); i++) {
    if (scale != 1) {
      scaledImages.push_back(_samples[i]._img->resize(scale));
    }
    else {
      scaledImages.push_back(_samples[i]._img);
    }
  }

  for (int r = 0; r < _samples.size(); r++) {
    for (int c = 0; c < _samples.size(); c++) {
      if (r == c) {
        ssim(r, c) = 0;
        continue;
      }
      
      ssim(r, c) = scaledImages[r]->MSSIM(scaledImages[c].get(), patchSize, a, b, g);
    }
  }

  _similarityMatrices["pairwiseSSIM"] = ssim;
}

vector<string> DimensionalityReduction::availableSimilarityMatrices()
{
  vector<string> names;
  for (auto& m : _similarityMatrices) {
    names.push_back(m.first);
  }

  return names;
}

Eigen::MatrixXd DimensionalityReduction::getSimilarityMatrix(string name)
{
  if (_similarityMatrices.count(name) > 0) {
    return _similarityMatrices[name];
  }

  getLogger()->log("No similarity matrix with key " + name, Comp::WARN);
  return Eigen::MatrixXd();
}

void DimensionalityReduction::deleteSimilarityMatrix(string name)
{
  _similarityMatrices.erase(name);
  getLogger()->log("Deleted matrix " + name);
}

Eigen::VectorXd DimensionalityReduction::stdToEigen(vector<double>& x)
{
  Eigen::VectorXd ex;
  ex.resize(x.size());

  for (int i = 0; i < x.size(); i++) {
    ex[i] = x[i];
  }

  return ex;
}

}