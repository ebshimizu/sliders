#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <cassert>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <random>
#include <chrono>

#include "ceres/ceres.h"
#include "glog/logging.h"

using ceres::DynamicAutoDiffCostFunction;
using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;
using namespace std;

#include "ceresFunctions.h"

using namespace Comp;
#include <ceresFunc.h>

enum GroupEvalFunction {
  ZERO_COUNT,
  FLOORED_SUM,
  SQUARED_TARGET,
  SQRT_TARGET
};

// information for handling a group of semantically similar layers.
// Used to compute additional objective functions related to the parameters of these layers.
class LayerGroup {
public: 
  // the layer group stores indexes to the opacity parameters of the layer groups.
  // each layer has to have opacity, and generally the effects we deal with here 
  LayerGroup(vector<int> opacityParams, GroupEvalFunction type = SQUARED_TARGET);

  // not all of these are used at once
  GroupEvalFunction _type;
  double _target;
  double _floor;
  string _name;
  vector<int> _group;

  // basically this class just computes various objective functions for the
  // given layer groups. Hopefully this presents enough flexibiliy to test various cases.
  // it will also provide some functions for generating distributions of the group itself.

  double eval(vector<double>& params);

  // will turn on a proportion of the group (100% opacity)
  void distributeVisible(vector<double>& params, double targetVisible);

  // does distribute visible and also sets opacities according to a gaussian distribution
  void distributeOpacity(vector<double>& params, double targetVisible, double opacityMean, double opacitySigma);

private:
  // returns the percentage of layers above 0% opacity (minimizing)
  double discreteZeroCount(vector<double>& params);

  // returns a sum of the opacity values up to a specified floor (default 0)
  double flooredSum(vector<double>& params);

  // returns the sum of squared distances to a specified target opacity
  double squaredTarget(vector<double>& params);

  // returns the sqrt of the abs distance to the target
  double sqrtTarget(vector<double>& params);
};

// a population element for the evolutionary search method
class PopElem {
public:
  PopElem(vector<double> g) : _g(g) { }

  // the genotype (here that's the layer parameters vector)
  vector<double> _g;

  // the phenoytpe (here that's the color of the pixels produced by the layer parameters vector)
  vector<double> _x;

  // fitness score
  double _fitness;

  // objective function scores
  vector<double> _f;

  bool operator<(const PopElem& b) const;
};

class Evo;

class App
{
public:
  // available modes:
  // - single (runs optimizer exactly once)
  // - jitter (random jitter around found solutions)
  // - eval (evaluates the given scene and returns the same scene with a score)
  // - paramStats (parameter statitics computation)
  // - minimaStats (parameter statistics computation around the first found local minima)
  // - paramShiftTest (statistics computed by adjusting groups of parameters to see if adjusting a certain number is better)
  // - random (randomly samples the space of parameters and tracks the frequency of each minima encountered.
  void go(string mode, string loadFrom, string saveTo);

  // loads a config file and then runs the normal go function
  // the only thing absolutely necessary in the file is the mode, loadFrom, and saveTo params
  void goFromConfig(string file);

  void setupOptimizer(string loadFrom, string saveTo);
  double runOptimizerOnce(bool printCeres = false);

  // evaluates the problem objective function once and outputs a file
  // containing the same input scene with a score field
  double eval();

  void exportSolution(string filename);

  // runs the optimization multiple times, tracking solution quality and randomizing
  // parameters to try to find better solutions
  void randomReinit();

  // computes some statistics for each parameter around the _initial configuration_ of the solver
  nlohmann::json exportParamStats(string filename);

  // computes some statistics for randomized values around the given local minima (assumed to be the state of _allParams)
  void exportMinimaStats(string filename);

  // outputs some stats about how effective adjusting groups of parameters is to escape local minima
  void paramShiftTest();

  // outputs some stats about of effective adjusting adjustments is to escape local minima
  void adjShiftTest();

  // randomizes the parameters and tracks which minima are found and how often
  // Intended to give a sense of how common various solutions are and how rare
  // stumbling into the proper location is
  void randomize();

  // runs the evolutionary algorithm
  void evo();

  // levels has a requirement that min < max, figure out if we have any here
  void computeLtConstraints();

  // looks at the json data and returns groups of up to 4 parameters that make up
  // an "adjustment group". Typically these are the entire adjustment, but some
  // such as selective color have too many to keep as one group and are broken up
  // into multiple sets
  vector<vector<int> > getAdjGroups();

  // helper for stats
  void computePopStats(vector<double>& vals, double& sum, double& mean, double& var, double& stdDev);

  // l2 distance between two vectors
  double l2vector(vector<double>& x1, vector<double>& x2);

  // Average Lab L2 distance between two rendered images (or at least, the pixels of the image we're looking 
  // at in this program)
  double pixelDist(vector<double>& x1, vector<double>& x2);

  // Gets the pixel color vector from a parameter vector
  vector<double> getLabVector(vector<double>& x);

  vector<double> _allParams;
  Problem _problem;
  nlohmann::json _data;
  string _outDir;
  nlohmann::json _config;

  // indicates that first should always be less than second
  map<int, int> _ltConstraints;

  // group constraints should be loaded from the json exchange file
  vector<LayerGroup> _groups;

  // cached layer pixel values for computing a perceptual distance function
  vector<vector<double> > _layerValues;
};

enum ObjectiveFunctionType {
  CERES = 0,
  DISTANCE_FROM_START = 1,
  GROUPS = 2
};

enum ComparisonMethod {
  PARETO = 0,
  WEIGHTED_SUM = 1
};

enum FitnessMethod {
  FIRST_OBJECTIVE_NO_SCALING = 0,
  VARIETY_PRESERVING = 1,
  PARETO_ORDERING = 2
};

enum EvoLogLevel {
  ABSURD = 0,
  ALL = 1,
  VERBOSE = 2,
  INFO = 3,
  CRITICAL = 4
};

enum ReturnSortOrder {
  CERES_ORDER = 0,
  FITNESS_ORDER = 1
};

// evolutionary search class (mostly for grouping settings together instead of duplicating
// things in the main class)
class Evo {
public:
  Evo(App* parent);
  ~Evo();

  // constructs necessary objects, updates settings, and runs
  void init();

  // runs the search
  void run();

  // stops the search
  void stop();

  // gets the results
  vector<PopElem> getResults();

  // peeks at the _config object to get some settings
  // if the optimization is running this won't actually do anything
  void updateSettings();

private:
  // functions to actually run the search
  vector<PopElem> createPop();

  // fills in the _f field for each population element
  void computeObjectives(vector<PopElem>& pop);

  // depending on the fitness assignment method, this redirects to one of a few functions
  void assignFitness(vector<PopElem>& pop);

  // simply takes the first objective function value (typically the ceres objective)
  // and uses that as the fitness score
  void firstObjectiveNoScalingFitness(vector<PopElem>& pop);

  // Implements algorithm 2.4 from http://www.it-weise.de/projects/book.pdf
  // Designed to balance exploration and exploitation
  void varietyPreservingFitness(vector<PopElem>& pop);

  int paretoCmp(PopElem& x1, PopElem& x2);
  int weightedSumCmp(PopElem& x1, PopElem& x2);

  double shareExp(double val, double sigma, double p);

  // selection method for picking a mating pool
  vector<PopElem> select(vector<PopElem>& pop, vector<PopElem>& arc);

  // mutate and recombine the mating pool to make a new population
  vector<PopElem> reproducePop(vector<PopElem>& mate, vector<PopElem>& elite);

  // run ceres on the population
  void optimizePop(vector<PopElem>& pop);

  void updateOptimalSet(vector<PopElem>& arc, vector<PopElem>& pop);

  // exports things 
  void exportPopElems(string prefix, vector<PopElem>& x);

  // gets top elements by ceres score at the end
  vector<PopElem> getBestCeresScores(vector<PopElem>& pop);

  // gets the top elements by fitness score at the end, assumes assign fitness has been called
  vector<PopElem> getBestFitnessScores(vector<PopElem>& pop);

  // checks that the element is valid (satisfies lt constraints specifically, at the moment)
  bool isValid(PopElem& p);

  App* _parent;

  bool _searchRunning;

  // generation counter
  int _t;

  // used for computing a secondary objective function
  // the secondary function measures how many parameters were changed to achieve the goal.
  // the intent is that if two solutions are basically identical, the better one is the one that
  // moves fewer parameters
  vector<double> _initialConfig;

  // settings everywhere
  // mutation rate
  double _mr;

  // crossover rate
  double _cr;

  // chance that a particular element gets chosen for crossover
  double _ce;

  // archive size
  int _arcSize;

  // Population size
  int _popSize;

  // mating pool size
  int _poolSize;

  // includes the starting configuration in the initial population
  bool _includeStartConfig;

  // if true will run ceres on every population element before doing fitness calculations
  bool _optimizeBeforeFitness;

  // if true, then crossovers will only happen between the archive and the selected
  // element in the population.
  bool _elitistRepro;

  // maximum iteration time
  int _maxIters;

  // ordered selection k parameter
  // roughly: the number of expected offspring from the best individual
  int _selectK;

  // tolerance for something to be considered equal
  double _equalityTolerance;

  // set of objective functions to use in this optimization
  set<ObjectiveFunctionType> _activeObjFuncs;

  // comparison function to use for fitness scoring the results
  ComparisonMethod _cmp;

  // how to compute the fitness values for the population elements
  FitnessMethod _v;

  // logging info  
  EvoLogLevel _logLevel;

  // generic log data storage
  // its json because we can just dump it ez if we need to (which we probably do)
  nlohmann::json _logData;

  // a bit of a dangerous option for your hard drive
  // will dump every generation of the resultsto json files
  // exports the population and the archive separately
  bool _exportPopulations;

  // dumps each generation of the archive to json files
  // less data than export populations
  bool _exportArchives;

  // run an optimization pass on the final elements
  // can be used independently of _optimizeBeforeFitness
  bool _optimizeFinal;

  // chance that ceres will be run as a mutation
  double _ceresRate;

  // optimizes the archive when updating the optimal set
  bool _optimizeArc;

  // number of results to return in the end
  int _returnSize;

  // used to determine which values are used for selecting the final results
  ReturnSortOrder _order;

  // chance that a group element gets set to 0. Only applies if there are groups in the optimizer
  double _knockoutRate;

  vector<PopElem> _finalArc;
  vector<PopElem> _finalPop;
};

template<class T>
struct cRGBColorT
{
  cRGBColorT() {}
  cRGBColorT(T _x, T _y, T _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }
  T sqMagintude()
  {
    return x * x + y * y + z * z;
  }
  cRGBColorT operator * (T v)
  {
    return RGBColorT(v * x, v * y, v * z);
  }
  cRGBColorT operator + (const cRGBColorT &v)
  {
    return cRGBColorT(x + v.x, y + v.y, z + v.z);
  }
  const T& operator [](int k) const
  {
    if (k == 0) return x;
    if (k == 1) return y;
    if (k == 2) return z;
    return x;
  }
  T x, y, z;
};

typedef cRGBColorT<double> cRGBColor;

template<class T>
vector<T> evalLayerColor(const T* const params, const vector<double> &layerValues)
{
  return ceresFunc<T>(params, layerValues);
}

struct CostTerm
{
  static const int ResidualCount = 3;

  CostTerm(const vector<double> &_layerValues, float _weight, const cRGBColor &_targetColor, map<int, int> ltc)
    : layerValues(_layerValues), weight(_weight), targetColor(_targetColor), _ltConstraints(ltc) {
    targetLabColor = Utils<float>::RGBToLab(targetColor.x, targetColor.y, targetColor.z);
  }

  template <typename T>
  bool operator()(const T* const params, T* residuals) const
  {
    // check lt constraints (key < value)
    for (auto& k : _ltConstraints) {
      if (params[k.first] >= params[k.second])
        return false;
    }

    // color is RGBA, right now compare vs premult RGB
    vector<T> color = evalLayerColor(params, layerValues);
    //residuals[0] = (color[0] * color[3] - T(targetColor.x)) * (T)weight;
    //residuals[1] = (color[1] * color[3] - T(targetColor.y)) * (T)weight;
    //residuals[2] = (color[2] * color[3] - T(targetColor.z)) * (T)weight;

    // lab space, CIE76
    Utils<T>::LabColorT labColor = Utils<T>::RGBToLab(color[0] * color[3], color[1] * color[3], color[2] * color[3]);
    residuals[0] = (labColor._L - (T)targetLabColor._L) * (T)weight;
    residuals[1] = (labColor._a - (T)targetLabColor._a) * (T)weight;
    residuals[2] = (labColor._b - (T)targetLabColor._b) * (T)weight;

    return true;
  }

  static ceres::CostFunction* Create(const vector<double> &layerValues, float weight, const cRGBColor &targetColor, map<int, int> ltc)
  {
    return (new ceres::AutoDiffCostFunction<CostTerm, CostTerm::ResidualCount, ceresFunc_paramACount>(
      new CostTerm(layerValues, weight, targetColor, ltc)));
  }

  vector<double> layerValues;
  cRGBColor targetColor;
  float weight;
  Utils<float>::LabColorT targetLabColor;
  map<int, int> _ltConstraints;
};
