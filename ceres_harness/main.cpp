#include "main.h"
#include "util.cpp"

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
  else if (mode == "paramStats") {
    exportParamStats(_outDir);
  }
}

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
      _data["params"][i]["adjustmentName"] == "hue") {
      _problem.SetParameterLowerBound(_allParams.data(), i, -1);
      _problem.SetParameterUpperBound(_allParams.data(), i, 2);
    }
    //else if (_data["params"][i]["adjustmentType"] == Comp::AdjustmentType::OPACITY) {
    //  _problem.SetParameterLowerBound(_allParams.data(), i, 0.99);
    //  _problem.SetParameterUpperBound(_allParams.data(), i, 1);
    //}
    else {
      _problem.SetParameterLowerBound(_allParams.data(), i, 0);
      _problem.SetParameterUpperBound(_allParams.data(), i, 1);
    }
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

      Stats stats = Stats(initParams, bestParams, _data);
      cout << stats.detailedSummary();
    }
    else {
      // if not better, do a reset
      _allParams = bestParams;
    }
  }

  cout << "Best found: " << bestSoFar;
}

void App::exportParamStats(string filename)
{
  // compute a few stats about each parameter involved in the solver
  // what we're trying to find out:
  // - which parameters are most "influential" / "sensitive"
  // - broad view of the parameter space
  nlohmann::json out;

  // single params
  for (int i = 0; i < _allParams.size(); i++) {
    cout << "Running stats for parameter " << i + 1 << " / " << _allParams.size() << "\n";

    vector<double> scores;
    double originalValue = _allParams[i];
    double min = DBL_MAX;
    double max = DBL_MIN;
    double sum = 0;

    // with 100 trials we can just sort of scan the space real quick
    for (int j = 0; j < 100; j++) {
      _allParams[i] = j / 100.0;
      double score = eval();

      if (score < min)
        min = score;
      if (score > max)
        max = score;

      scores.push_back(score);
      sum += score;
    }

    // compute stats
    double mean, var, stdDev;
    computePopStats(scores, sum, mean, var, stdDev);

    // output
    nlohmann::json varData;
    varData["adjustmentData"] = _data["params"][i];

    varData["scores"] = scores;
    varData["avg"] = mean;
    varData["var"] = var;
    varData["stdDev"] = stdDev;
    varData["min"] = min;
    varData["max"] = max;

    // maximum useful range?
    out[i] = varData;

    // reset
    _allParams[i] = originalValue;
  }

  cout << "Writing file to " << filename << "\n";

  // single adjustments?

  ofstream outFile(filename);
  outFile << out.dump(4);
}

void App::exportMinimaStats(string filename)
{
  // computes some stats for various adjustment algorithms around a local minima
  nlohmann::json out;

  // stats to compute:
  // - typical: avg, mean, stdDev, min, max
  // - times jump was strictly better
  // - distance from found minima (compare feature vectors)
  // - some sort of stat representing the distance from minima + found score
  // - how many times we escaped the attraction basin of this minima (expensive, runs ceres each time)
  // - derivatives?
  
  float sigma = 0.25;
  float n = 0.15;

  // rng things
  random_device rd;
  mt19937 gen(rd());
  normal_distribution<double> dist(0, sigma);
  uniform_real_distribution<double> zeroOne(0, 1);

  vector<int> paramIndices;
  for (int i = 0; i < _allParams.size(); i++)
    paramIndices.push_back(i);

  // store initial state
  vector<double> initParams = _allParams;

  int numParams = _allParams.size() * n;

  nlohmann::json method1;

  double min = DBL_MAX;
  double max = DBL_MIN;
  double sum = 0;
  vector<double> scores;

  // method 1
  // uniform 0-1 randomization on ~15% of all parameters
  for (int runs = 0; runs < 100; runs++) {
    // select parameters
    shuffle(paramIndices.begin(), paramIndices.end(), gen);

    for (int i = 0; i < numParams; i++) {
      int index = paramIndices[i];

      _allParams[index] = zeroOne(gen);
    }

    // eval the result
    double score = eval();

    if (score < min)
      min = score;
    if (score > max)
      max = score;

    sum += score;

    scores.push_back(score);
  }

  // compute stats
  double mean, var, stdDev;
  computePopStats(scores, sum, mean, var, stdDev);

  // method 2
  // gaussian randomization on ~15% of all parameters, fixed sigma

  // method 3
  // uniform 0-1 randomization on 1 adjustment (group of parameters)

  // method 4
  // gaussian randomization on 1 adjustment, fixed sigma (group of parameters)

  // method 5
  // biased uniform 0-1 randomization on selected parameters
  // this one would take info computed earlier and bias parameter selection towards parameters
  // known to have a large influence on the overall image (determined by attribute score)

  // method 6
  // gaussian randomization on selected parameters (see method 5)

  // method 7
  // 
}

void App::computePopStats(vector<double>& vals, double& sum, double& mean, double& var, double& stdDev)
{
  // compute stats
  mean = sum / vals.size();

  // variance
  var = 0;
  for (auto& s : vals) {
    var += (mean - s) * (mean - s);
  }

  var /= vals.size();
  stdDev = sqrt(var);
}

void main(int argc, char* argv[])
{
	App app;
	app.go(string(argv[1]), string(argv[2]), string(argv[3]));

	//cin.get();
}