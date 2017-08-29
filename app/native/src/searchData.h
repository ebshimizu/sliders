#pragma once

#include "Image.h"
#include "Layer.h"
#include "util.h"
#include "Histogram.h"

namespace Comp {

enum SearchGroupType {
  G_STRUCTURE = 0,
  G_COLOR = 1,
  G_FIXED = 2
};

enum StructDiffMode {
  GLOBAL_STRUCT = 0,
  AVG_STRUCT = 1,
  BIN_STRUCT = 2
};

struct SearchGroup {
  SearchGroupType _type;
  vector<string> _layerNames;
};

// an exploratory search result
// consists of a rendered image, and a specified context
class ExpSearchSample {
public:
  ExpSearchSample(shared_ptr<Image> img, Context ctx, vector<double> ctxVec);
  ExpSearchSample(const ExpSearchSample& other);

  ~ExpSearchSample();

  double brightnessDist(shared_ptr<ExpSearchSample> other);
  double hueDist(shared_ptr<ExpSearchSample> other);
  double satDist(shared_ptr<ExpSearchSample> other);
  double structDiff(shared_ptr<ExpSearchSample> other);

  vector<double> getContextVector();
  shared_ptr<Image> getImg();
  Context getContext();

  // sample ID, usually controlled by ExpSearchSet
  unsigned int _id;

  Histogram _brightness;
  Histogram _hue;
  Histogram _sat;

private:
  void preProcess();

  shared_ptr<Image> _render;
  Context _ctx;
  vector<double> _ctxVec;
};

// this class manages the set of results currently in the exploratory
// search result set. It should handle all the operations needed to add a new
// proposed sample.
class ExpSearchSet {
public:
  ExpSearchSet();

  // pull settings from settings map
  ExpSearchSet(map<string, float>& settings);
  ~ExpSearchSet();

  // sets the start config
  void setInitial(shared_ptr<ExpSearchSample> init);
  shared_ptr<ExpSearchSample> getInitial();

  // adds a result to the set if it expands the diversity of the set
  bool add(shared_ptr<ExpSearchSample> x, bool structural = false);

  // for now samples can be added but not removed, so array-like indexing is fine
  shared_ptr<ExpSearchSample> get(unsigned int id);

  // retrieves the reason why a sample was allowed in the set
  string getReason(unsigned int id);

  int size();

  // right now this function runs a few low-order tests to see if the sample 
  // meets some basic criteria for image quality
  bool isGood(shared_ptr<ExpSearchSample> x);

private:
  // since there are different struct difference mode, this handles that
  double structDiff(shared_ptr<ExpSearchSample> x, shared_ptr<ExpSearchSample> y);

  // bit different from the other struct diff functions, this one will check against
  // all samples and return a percentage of bins that are unique to the sample
  double binStructPct(shared_ptr<ExpSearchSample> x);

  // the samples
  map<unsigned int, shared_ptr<ExpSearchSample> > _samples;

  // explanations for why a sample was admitted
  map<unsigned int, string> _reasoning;

  // used for assigning ids
  unsigned int _idCounter;

  // settings/options
  // Required number of axes the sample needs to be different from
  // in order to be accepted
  int _axisReq;

  shared_ptr<ExpSearchSample> _init;

  double _brightThreshold;
  double _hueThreshold;
  double _satThreshold;
  double _structThreshold;

  // if these values are exceeded, the sample is rejected
  double _brightTolerance;
  double _hueTolerance;
  double _clipTolerance;

  // ssim params
  double _ssimA;
  double _ssimB;
  double _ssimG;

  StructDiffMode _structMode;
  int _structBinSize;
};

}