
#include "main.h"
#include "ceresFunctions.h"

using namespace Comp;
#include <ceresFunc.h>
#include "util.cpp"

class App
{
public:
  // available modes:
  // - single (runs optimizer exactly once)
  // - jitter (random jitter around found solutions)
  // - eval (evaluates the given scene and returns the same scene with a score)
	void go(string mode, string loadFrom, string saveTo);

	void setupOptimizer(string loadFrom, string saveTo);
  double runOptimizerOnce();

  // evaluates the problem objective function once and outputs a file
  // containing the same input scene with a score field
  double eval();

  void exportSolution(string filename);

  // runs the optimization multiple times, tracking solution quality and randomizing
  // parameters to try to find better solutions
  void randomReinit();

  vector<double> _allParams;
  Problem _problem;
  nlohmann::json _data;
  string _outDir;
};

void App::go(string mode, string loadFrom, string saveTo)
{
	setupOptimizer(loadFrom, saveTo);
  
  if (mode == "single") {
    runOptimizerOnce();
    exportSolution("ceres_result.json");
  }
  else if (mode == "jitter") {
    randomReinit();
  }
  else if (mode == "eval") {
    double score = eval();
    _data["score"] = score;
    exportSolution("ceres_score.json");
  }
}

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

	CostTerm(const vector<double> &_layerValues, float _weight, const cRGBColor &_targetColor)
		: layerValues(_layerValues), weight(_weight), targetColor(_targetColor) {}

	template <typename T>
	bool operator()(const T* const params, T* residuals) const
	{
    // color is RGBA, right now compare vs premult RGB
		vector<T> color = evalLayerColor(params, layerValues);
		residuals[0] = (color[0] * color[3] - T(targetColor.x)) * T(weight);
		residuals[1] = (color[1] * color[3] - T(targetColor.y)) * T(weight);
		residuals[2] = (color[2] * color[3] - T(targetColor.z)) * T(weight);

    //if (!residuals[0].v.allFinite() || !residuals[1].v.allFinite() || !residuals[2].v.allFinite()) {
      //cout << "Nan";
      //while (true) {
      //  vector<T> color2 = evalLayerColor(params, layerValues);
      //}
    //}

		return true;
	}

  template<>
  bool operator()<double>(const double* const params, double* residuals) const
  {
    // color is RGBA, right now compare vs premult RGB
    vector<double> color = evalLayerColor(params, layerValues);
    residuals[0] = (color[0] * color[3] - (double)(targetColor.x)) * (weight);
    residuals[1] = (color[1] * color[3] - (double)(targetColor.y)) * (weight);
    residuals[2] = (color[2] * color[3] - (double)(targetColor.z)) * (weight);

    return true;
  }

	static ceres::CostFunction* Create(const vector<double> &layerValues, float weight, const cRGBColor &targetColor)
	{
		return (new ceres::AutoDiffCostFunction<CostTerm, CostTerm::ResidualCount, ceresFunc_paramACount>(
			new CostTerm(layerValues, weight, targetColor)));
	}

	vector<double> layerValues;
	cRGBColor targetColor;
	float weight;
};

void App::setupOptimizer(string loadFrom, string saveTo)
{
  vector<vector<double> > layerValues;
  vector<vector<double> > targetColors;
  vector<double> weights;
  _outDir = saveTo;

  _data = loadCeresData(loadFrom, _allParams, layerValues, targetColors, weights);

	// add all fit constraints
	//if (mask(i, j) == 0 && constaints(i, j).u >= 0 && constaints(i, j).v >= 0)
	//    fit = (x(i, j) - constraints(i, j)) * w_fitSqrt
  for (int i = 0; i < targetColors.size(); i++) {
    cRGBColor target(targetColors[i][0], targetColors[i][1], targetColors[i][2]);
    ceres::CostFunction* costFunction = CostTerm::Create(layerValues[i], weights[i], target);
    _problem.AddResidualBlock(costFunction, NULL, _allParams.data());
  }

  for (int i = 0; i < _allParams.size(); i++) {
    if (_data["params"][i]["adjustmentType"] == Comp::AdjustmentType::HSL &&
      _data["params"][i]["adjustmentName"] == "hue")
      continue;

    _problem.SetParameterLowerBound(_allParams.data(), i, 0);
    _problem.SetParameterUpperBound(_allParams.data(), i, 1);
  }
}

double App::runOptimizerOnce()
{
  cout << "Solving..." << endl;

  Solver::Options options;
  Solver::Summary summary;

  const bool performanceTest = false;
  options.minimizer_progress_to_stdout = !performanceTest;

  //faster methods
  options.num_threads = 7;
  options.num_linear_solver_threads = 7;
  //options.linear_solver_type = ceres::LinearSolverType::SPARSE_NORMAL_CHOLESKY; //7.2s
  //options.trust_region_strategy_type = ceres::TrustRegionStrategyType::DOGLEG;
  //options.linear_solver_type = ceres::LinearSolverType::SPARSE_SCHUR; //10.0s

  //slower methods
  //options.linear_solver_type = ceres::LinearSolverType::ITERATIVE_SCHUR; //40.6s
  //options.linear_solver_type = ceres::LinearSolverType::CGNR; //46.9s

  //options.min_linear_solver_iterations = linearIterationMin;
  options.max_num_iterations = 10000;
  options.function_tolerance = 0.1;
  options.gradient_tolerance = 1e-4 * options.function_tolerance;

  //options.min_lm_diagonal = 1.0f;
  //options.min_lm_diagonal = options.max_lm_diagonal;
  //options.max_lm_diagonal = 10000000.0;

  //problem.Evaluate(Problem::EvaluateOptions(), &cost, nullptr, nullptr, nullptr);
  //cout << "Cost*2 start: " << cost << endl;

  Solve(options, &_problem, &summary);

  cout << "Solver used: " << summary.linear_solver_type_used << endl;
  cout << "Minimizer iters: " << summary.iterations.size() << endl;

  double iterationTotalTime = 0.0;
  int totalLinearItereations = 0;
  for (auto &i : summary.iterations)
  {
    iterationTotalTime += i.iteration_time_in_seconds;
    totalLinearItereations += i.linear_solver_iterations;
    cout << "Iteration: " << i.linear_solver_iterations << " " << i.iteration_time_in_seconds * 1000.0 << "ms" << endl;
  }

  cout << "Total iteration time: " << iterationTotalTime << endl;
  cout << "Cost per linear solver iteration: " << iterationTotalTime * 1000.0 / totalLinearItereations << "ms" << endl;

  double cost = -1.0;
  _problem.Evaluate(Problem::EvaluateOptions(), &cost, nullptr, nullptr, nullptr);
  cout << "Cost*2 end: " << cost * 2 << endl;

  // params
  //for (int i = 0; i < _allParams.size(); i++) {
  //  cout << "p[" << i << "]: " << _allParams[i] << "\n";
  //}

  _data["score"] = cost;
  //cout << summary.FullReport() << endl;

  return cost;
}

double App::eval()
{
  double cost;
  _problem.Evaluate(Problem::EvaluateOptions(), &cost, nullptr, nullptr, nullptr);

  return cost;
}

void App::exportSolution(string filename)
{
  // re-export
  for (int i = 0; i < _allParams.size(); i++) {
    _data["params"][i]["value"] = _allParams[i];
  }

  string file(_outDir + filename);
  cout << "Writing file to " << file << "\n";

  //remove(file.c_str());
  ofstream out(_outDir + filename);
  out << _data.dump(4);
}

void App::randomReinit()
{
  // threshold for being best global solution
  float minEps = 1e-3;
  int maxIters = 100;
  float pctParams = 0.2;
  float sigma = 0.25;
  int paramsToJitter = (int) (pctParams * _allParams.size());

  // rng things
  random_device rd;
  mt19937 gen(rd());
  normal_distribution<double> dist(0, sigma);

  vector<int> paramIndices;
  for (int i = 0; i < _allParams.size(); i++)
    paramIndices.push_back(i);

  // store initial state
  vector<double> initParams = _allParams;

  // run problem from initial state
  double bestSoFar = runOptimizerOnce();
  vector<double> bestParams = _allParams;
  exportSolution("ceresSolution_0.json");

  for (int i = 0; i < maxIters; i++) {
    // randomize x% of params from previous solution and re-run
    // everything is clamped between 0 and 1 except hue right now
    shuffle(paramIndices.begin(), paramIndices.end(), gen);

    for (int j = 0; j < paramsToJitter; j++) {
      int index = paramIndices[j];

      _allParams[index] += dist(gen);
      
      if (_data["params"][index]["adjustmentType"] == Comp::AdjustmentType::HSL &&
        _data["params"][index]["adjustmentName"] == "hue") {
        continue;
      }
      else {
        _allParams[index] = clamp(0.0, 1.0, _allParams[index]);
      }
    }

    // re-run optimizer
    double newScore = runOptimizerOnce();

    if (newScore < bestSoFar) {
      bestSoFar = newScore;
      bestParams = _allParams;
      exportSolution("ceresSolution_" + to_string(i) + ".json");
    }
    else {
      // if not better, do a reset
      _allParams = bestParams;
    }
  }

  cout << "Best found: " << bestSoFar;
}

void main(int argc, char* argv[])
{
	App app;
	app.go(string(argv[1]), string(argv[2]), string(argv[3]));

	//cin.get();
}