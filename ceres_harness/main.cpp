
#include "main.h"
#include "ceresFunctions.h"

using namespace Comp;
#include "ceresFunc.h"
#include "util.cpp"

class App
{
public:
	void go(string loadFrom, string saveTo);

	void testOptimizer(string loadFrom, string saveTo);
};

void App::go(string loadFrom, string saveTo)
{
	testOptimizer(loadFrom, saveTo);
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

void App::testOptimizer(string loadFrom, string saveTo)
{
	Problem problem;

	vector<double> allParams;
  vector<vector<double> > layerValues;
  vector<vector<double> > targetColors;
  vector<double> weights;

  nlohmann::json data = loadCeresData(loadFrom, allParams, layerValues, targetColors, weights);

	// add all fit constraints
	//if (mask(i, j) == 0 && constaints(i, j).u >= 0 && constaints(i, j).v >= 0)
	//    fit = (x(i, j) - constraints(i, j)) * w_fitSqrt
  for (int i = 0; i < targetColors.size(); i++) {
    cRGBColor target(targetColors[i][0], targetColors[i][1], targetColors[i][2]);
    ceres::CostFunction* costFunction = CostTerm::Create(layerValues[i], weights[i], target);
    problem.AddResidualBlock(costFunction, NULL, allParams.data());
  }

  /*
	for (int pixel = 0; pixel < 1; pixel++)
	{
    vector<double> layerValues = { 0, 174.0 / 255.0, 239.0 / 255.0, 1.0, 0, 0, 0, 0, 0, 0, 0, 0 }; // not currently used
		const float pixelWeight = 1.0f;
		cRGBColor targetColor(1.0, 0, 0);
		ceres::CostFunction* costFunction = CostTerm::Create(layerValues, pixelWeight, targetColor);
		problem.AddResidualBlock(costFunction, NULL, allParams.data());
	}
  */

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

	Solve(options, &problem, &summary);

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
	problem.Evaluate(Problem::EvaluateOptions(), &cost, nullptr, nullptr, nullptr);
	cout << "Cost*2 end: " << cost * 2 << endl;

  // params
  for (int i = 0; i < allParams.size(); i++) {
    cout << "p[" << i << "]: " << allParams[i] << "\n";
  }

	cout << summary.FullReport() << endl;

  // re-export
  for (int i = 0; i < allParams.size(); i++) {
    data["params"][i]["value"] = allParams[i];
  }

  ofstream out(saveTo);
  out << data.dump(4);
}

void main(int argc, char* argv[])
{
	App app;
	app.go(string(argv[1]), string(argv[2]));

	cin.get();
}