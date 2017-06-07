
#include "main.h"

template<class T>
T f0(T v0, T v1)
{
	//return T(0.0);
	return (v0 + v1) * 0.5;
}

template<class T>
T f1(T v0, T v1)
{
	T v2 = v0 * 0.1 * v1 / -0.5;
	T v3 = cos(sin(0.6 / -v0 + v1 * v1 * 0.7));
	T v4 = 0.9 * sqrt(v3 * v3) + 1.0;
	T v5 = 0.8 + pow(v4, v4);
	return v5;
}

vector<double> RGToHSV(double v0, double v1)
{
	vector<double> result;
	result.push_back(v0 + v1);
	result.push_back(v0 - v1);
	result.push_back(v0 * v1);
	return result;
}

vector<ExpStep> RGToHSV(ExpStep v0, ExpStep v1)
{
	return v0.context->callFunc("RGToHSV", v0, v1);
}

template<class T>
T f2(T v0, T v1)
{
	T v2 = v0 * v1;
	T v3 = v2 + v0;
	vector<T> results = RGToHSV(v2, v3);
	T v4 = results[0] + v0;
	T v5 = results[1] + v1;
	T v6 = results[2] + v2;
	return v4 * v5 * v6;
}

void TestApp::go()
{
	//testFunction2(function<ETree(ETree, ETree)>(f0<ETree>), function<double(double, double)>(f0<double>));
	//testFunction2(function<ETree(ETree, ETree)>(f1<ETree>), function<double(double, double)>(f1<double>));

	testFunction2(function<ExpStep(ExpStep, ExpStep)>(f0<ExpStep>), function<double(double, double)>(f0<double>), "f0");
	testFunction2(function<ExpStep(ExpStep, ExpStep)>(f1<ExpStep>), function<double(double, double)>(f1<double>), "f1");
	testFunction2(function<ExpStep(ExpStep, ExpStep)>(f2<ExpStep>), function<double(double, double)>(f2<double>), "f2");

	testOptimizer();
}

void TestApp::testFunction2(function<ExpStep(ExpStep, ExpStep)>& funcE, function<double(double, double)>& funcD, const string &functionName)
{
	cout << "testing function2" << endl;
	for (int testIndex = 0; testIndex < 5; testIndex++)
	{
		double x0D = rand() % 100 - 50.0;
		double x1D = rand() % 100 - 50.0;

		double vD = funcD(x0D, x1D);

		ExpContext context;
		context.registerFunc("RGToHSV", 2, 3);

		ExpStep x0E = context.registerParam(0, "x0", x0D);
		ExpStep x1E = context.registerParam(1, "x1", x1D);

		//ExpStep vE = x0E + x1E;

		ExpStep vE = funcE(x0E, x1E);
		context.registerResult(vE, 0, "output0");
		//ExpStep vEOut2 = ExpStep(x0E, 1);
		//ExpStep vEOut3 = ExpStep(x1E, 2);

		if (testIndex == 0)
		{
			vector<string> sourceCode = context.toSourceCode(functionName);
			ofstream file(functionName + ".txt");
			for (auto &s : sourceCode)
			{
				file << s << endl;
				cout << s << endl;
			}
		}

		double vEEval = context.eval();

		cout << "delta " << testIndex << " = " << vD - vEEval << endl;
	}
}

template<class T>
struct RGBColorT
{
	RGBColorT() {}
	RGBColorT(T _x, T _y, T _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	T sqMagintude()
	{
		return x * x + y * y + z * z;
	}
	RGBColorT operator * (T v)
	{
		return RGBColorT(v * x, v * y, v * z);
	}
	RGBColorT operator + (const RGBColorT &v)
	{
		return RGBColorT(x + v.x, y + v.y, z + v.z);
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

typedef RGBColorT<double> RGBColor;

template<class T>
RGBColorT<T> evalLayerColor(const T* const params, const vector<float> &layerValues)
{
	return RGBColorT<T>(params[0] * params[3], params[1] * params[3], params[2] * params[3]);
}

struct CostTerm
{
	static const int ResidualCount = 3;
	static const int ParameterCount = 4;

	CostTerm(const vector<float> &_layerValues, float _weight, const RGBColor &_targetColor)
		: layerValues(_layerValues), weight(_weight), targetColor(_targetColor) {}

	template <typename T>
	bool operator()(const T* const params, T* residuals) const
	{
		RGBColorT<T> color = evalLayerColor(params, layerValues);
		residuals[0] = (color[0] - T(targetColor.x)) * T(weight);
		residuals[1] = (color[1] - T(targetColor.y)) * T(weight);
		residuals[2] = (color[2] - T(targetColor.z)) * T(weight);
		return true;
	}

	static ceres::CostFunction* Create(const vector<float> &layerValues, float weight, const RGBColor &targetColor)
	{
		return (new ceres::AutoDiffCostFunction<CostTerm, CostTerm::ResidualCount, CostTerm::ParameterCount>(
			new CostTerm(layerValues, weight, targetColor)));
	}

	vector<float> layerValues;
	RGBColor targetColor;
	float weight;
};

void TestApp::testOptimizer()
{
	Problem problem;

	vector<double> allParams(CostTerm::ParameterCount);

	// add all fit constraints
	//if (mask(i, j) == 0 && constaints(i, j).u >= 0 && constaints(i, j).v >= 0)
	//    fit = (x(i, j) - constraints(i, j)) * w_fitSqrt
	for (int pixel = 0; pixel < 5; pixel++)
	{
		vector<float> layerValues(3); // not currently used
		const float pixelWeight = 1.0f;
		RGBColor targetColor(0.5f, 0.5f, 0.5f);
		ceres::CostFunction* costFunction = CostTerm::Create(layerValues, pixelWeight, targetColor);
		problem.AddResidualBlock(costFunction, NULL, allParams.data());
	}

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

	cout << summary.FullReport() << endl;

}
