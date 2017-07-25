#include "main.h"
#include "util.cpp"
#include <algorithm>

bool PopElem::operator<(const PopElem & b) const
{
  return _fitness < b._fitness;
}

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
  else if (mode == "paramShiftTest") {
    paramShiftTest();
  }
  else if (mode == "adjShiftTest") {
    adjShiftTest();
  }
  else if (mode == "random") {
    randomize();
  }
  else if (mode == "evo") {
    evo();
  }
}

void App::goFromConfig(string file)
{
  // load and go
  ifstream config(file);

  if (config.is_open()) {
    config >> _config;

    config.close();

    // some required fields
    if (_config.count("mode") > 0 && _config.count("loadFrom") > 0 && _config.count("saveTo") > 0) {
      go(_config["mode"].get<string>(), _config["loadFrom"].get<string>(), _config["saveTo"].get<string>());
    }
    else {
      cout << "Missing required fields. Config file must contain mode, loadFrom, and saveTo fields.\n";
    }
  }
  else {
    cout << "Couldn't load config file from " << file << "\n";
  }
}

void App::setupOptimizer(string loadFrom, string saveTo)
{
  vector<vector<double> > targetColors;
  vector<double> weights;
  _outDir = saveTo;
  _layerValues.clear();

  _data = loadCeresData(loadFrom, _allParams, _layerValues, targetColors, weights);

  // levels constraints
  computeLtConstraints();

	// add all fit constraints
	//if (mask(i, j) == 0 && constaints(i, j).u >= 0 && constaints(i, j).v >= 0)
	//    fit = (x(i, j) - constraints(i, j)) * w_fitSqrt
  for (int i = 0; i < targetColors.size(); i++) {
    cRGBColor target(targetColors[i][0], targetColors[i][1], targetColors[i][2]);
    ceres::CostFunction* costFunction = CostTerm::Create(_layerValues[i], weights[i], target, _ltConstraints);
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

double App::runOptimizerOnce(bool printCeres)
{
  //cout << "Solving..." << endl;

  Solver::Options options;
  Solver::Summary summary;

  const bool performanceTest = false;
  options.minimizer_progress_to_stdout = false;
  //options.minimizer_progress_to_stdout = !performanceTest;

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

  if (printCeres) {
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
  }

  //double cost = -1.0;
  //_problem.Evaluate(Problem::EvaluateOptions(), &cost, nullptr, nullptr, nullptr);
  //cout << "Cost*2 end: " << cost * 2 << endl;

  // params
  //for (int i = 0; i < _allParams.size(); i++) {
  //  cout << "p[" << i << "]: " << _allParams[i] << "\n";
  //}

  _data["score"] = summary.final_cost;
  //cout << summary.FullReport() << endl;

  return summary.final_cost;
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
  // defaults
  // threshold for being best global solution
  float minEps = 1e-3;
  int maxIters = 100;
  float pctParams = 0.2;
  float sigma = 0.25;
  int paramsToJitter = (int) (pctParams * _allParams.size());

  // load settings if they exist
  if (_config.count("randomReinit") > 0) {
    nlohmann::json localSettings = _config["random"];

    minEps = (_config.count("minEps") > 0) ? _config["minEps"] : minEps;
    maxIters = (_config.count("maxIters") > 0) ? _config["maxIters"] : maxIters;
    pctParams = (_config.count("pctParams") > 0) ? _config["pctParams"] : pctParams;
    sigma = (_config.count("sigma") > 0) ? _config["sigma"] : sigma;
    paramsToJitter = (int) (pctParams * _allParams.size());
  }

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

nlohmann::json App::exportParamStats(string filename)
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
    double currentScore = eval();
    double minLoc = 0;

    // with 100 trials we can just sort of scan the space real quick
    for (int j = 0; j <= 100; j++) {
      _allParams[i] = j / 100.0;
      double score = eval();

      if (score < min) {
        min = score;
        minLoc = j / 100.0;
      }
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
    varData["bestImprove"] = (min - currentScore);
    varData["distToImprove"] = (minLoc - originalValue);

    // maximum useful range?
    out[i] = varData;

    // reset
    _allParams[i] = originalValue;
  }

  cout << "Writing file to " << filename << "\n";

  // single adjustments?

  ofstream outFile(filename);
  outFile << out.dump(4);

  return out;
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
  double initScore = eval();

  int numParams = _allParams.size() * n;

  nlohmann::json method1;

  double min = DBL_MAX;
  double max = DBL_MIN;
  double sum = 0;
  vector<double> scores;
  int success = 0;  // number of times we were better than the minima

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

    if (score < initScore)
      success++;

    // reset
    _allParams = initParams;
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

void App::paramShiftTest()
{
  cout << "Starting parameter shift test\n";

  // first determine how each of the parameters generally affect the objective
  auto sens = exportParamStats(_outDir + "prelim_stats.json");
  nlohmann::json trialResults;

  // runs the solver once
  double localMin = runOptimizerOnce();
  exportSolution("initial_solution.json");

  // rng things
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<double> zeroOne(0, 1);

  vector<int> paramIndices;
  for (int i = 0; i < _allParams.size(); i++)
    paramIndices.push_back(i);

  // store local min start state
  vector<double> initParams = _allParams;

  cout << "Initial minima found, starting randomization tests to escape with k=[1,10]\n";
  cout << "Score: " << localMin << "\n";

  // now we're in a local min
  // we want to see how we can get out of here. This function will try out
  // combinations of individual parameters and report the results.
  // TODO: this needs a config file badly
  for (int k = 1; k <= (int)(_allParams.size() / 2); k++) {
    cout << "\nStarting trial for k = " << k << "\n";

    nlohmann::json trialData;
    trialData["rndBetter"] = 0;
    trialData["ceresBetter"] = 0;
    trialData["timesNewMinimaFound"] = 0;
    trialData["bestScore"] = localMin;
    trialData["k"] = k;
    trialData["n"] = 100;
    vector<double> scores;
    vector<double> ceresScores;
    vector<double> distToStartOptima;
    vector<double> bestParams = initParams;
    vector<double> improvedDiffs;

    for (int n = 0; n < 100; n++) {
      // pick k random parameters to adjust
      shuffle(paramIndices.begin(), paramIndices.end(), gen);

      for (int i = 0; i < k; i++) {
        // using the sensitivity stats, determine how to adjust them with a given random distribution
        int paramId = paramIndices[i];

        double maxImprovement = sens[paramId]["bestImprove"];
        double improvementDist = sens[paramId]["distToImprove"];

        // if the parameter exhibits no possible benefits, just adjust it a little since
        // it's not expected to help at all
        //if (abs(maxImprovement) < 0.01) {
        //  normal_distribution<double> dist(0, 0.05);

        //  _allParams[i] += dist(gen);
        //}
        // otherwise vary the param with a gaussian based on expected vairance in value
        //else {
        //  normal_distribution<double> dist(0, abs(improvementDist) / 2);

        //  _allParams[i] += dist(gen);
        //}

        _allParams[i] = zeroOne(gen); // clamp(_allParams[i], 0.0, 1.0);
      }

      // check score
      double rndScore = eval();

      // run ceres, check score
      double ceresScore = runOptimizerOnce();

      // determine difference from this minima to starting minima
      double dist = l2vector(_allParams, initParams);

      cout << "[k=" << k << "][" << n << "/100]\t Score: " << rndScore << ", Opt. Score: " << ceresScore << ", Dist: " << dist << "\n";

      // determine if we did better
      if (abs(rndScore - localMin) > 0.01 && rndScore < localMin) {
        trialData["rndBetter"] = trialData["rndBetter"] + 1;
      }

      if (abs(ceresScore - localMin) > 0.01 && ceresScore < localMin) {
        trialData["ceresBetter"] = trialData["ceresBetter"] + 1;
        improvedDiffs.push_back(ceresScore - localMin);

        // determine if we're different than the old minima
        // TODO: no idea what would be a good threshold, maybe just like set it to 0.25?
        if (dist > 0.25) {
          trialData["timesNewMinimaFound"] = trialData["timesNewMinimaFound"] + 1;
        }
      }

      // track best score found
      if (ceresScore < trialData["bestScore"]) {
        trialData["bestScore"] = ceresScore;
        bestParams = _allParams;
        exportSolution("best_solution_" + to_string(k) + ".json");
      }

      // other stats
      scores.push_back(rndScore);
      ceresScores.push_back(ceresScore);
      distToStartOptima.push_back(dist);

      // reset for next run
      _allParams = initParams;
    }

    // json object update
    trialData["rndScores"] = scores;
    trialData["ceresScores"] = ceresScores;
    trialData["distToFirstOptima"] = distToStartOptima;
    trialData["bestParams"] = bestParams;

    double avgDiff = 0;
    for (auto& d : improvedDiffs) {
      avgDiff += d;
    }
    trialData["improvedScores"] = improvedDiffs;
    trialData["averageImprovement"] = avgDiff / improvedDiffs.size();

    trialResults.push_back(trialData);
  }

  ofstream outFile(_outDir + "full_result_summary.json");
  outFile << trialResults.dump(4);
}

void App::adjShiftTest()
{
  // this is very similar to param shift test but instead we group the parameters by which adjustments
  // they belong to
  cout << "Starting parameter shift test\n";

  // first determine how each of the adjustments generally affect the objective
  auto sens = exportParamStats(_outDir + "prelim_stats.json");
  nlohmann::json trialResults;

  // runs the solver once
  double localMin = runOptimizerOnce();
  exportSolution("initial_solution.json");

  // rng things
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<double> zeroOne(0, 1);

  // store local min start state
  vector<double> initParams = _allParams;

  cout << "Initial minima found\n";
  cout << "Score: " << localMin << "\n";

  // collect parameter groups
  vector<vector<int>> groups = getAdjGroups();

  vector<int> groupIndices;
  // organize
  for (int i = 0; i < groups.size(); i++) {
    groupIndices.push_back(i);
  }

  cout << "Identified " << groups.size() << " parameter groups\n";
  cout << "Beginning adjustment shift test...\n";

  for (int k = 1; k <= (int)(groups.size() / 2); k++) {
    cout << "\nStarting trial for k = " << k << "\n";

    nlohmann::json trialData;
    trialData["rndBetter"] = 0;
    trialData["ceresBetter"] = 0;
    trialData["timesNewMinimaFound"] = 0;
    trialData["bestScore"] = localMin;
    trialData["k"] = k;
    trialData["n"] = 100;
    vector<double> scores;
    vector<double> ceresScores;
    vector<double> distToStartOptima;
    vector<double> bestParams = initParams;
    vector<double> improvedDiffs;

    for (int n = 0; n < 100; n++) {
      // pick k random parameter groups to adjust
      shuffle(groupIndices.begin(), groupIndices.end(), gen);

      for (int i = 0; i < k; i++) {
        // using the sensitivity stats, determine how to adjust them with a given random distribution
        for (auto& paramId : groups[groupIndices[i]]) {
          double maxImprovement = sens[paramId]["bestImprove"];
          double improvementDist = sens[paramId]["distToImprove"];

          // if the parameter exhibits no possible benefits, just adjust it a little since
          // it's not expected to help at all
          if (abs(maxImprovement) < 0.01) {
            normal_distribution<double> dist(0, 0.05);

            _allParams[i] += dist(gen);
          }
          // otherwise vary the param with a gaussian based on expected vairance in value
          else {
            normal_distribution<double> dist(0, abs(improvementDist) / 2);

            _allParams[i] += dist(gen);
          }

          _allParams[i] = clamp(_allParams[i], 0.0, 1.0);
        }
      }

      // check score
      double rndScore = eval();

      // run ceres, check score
      double ceresScore = runOptimizerOnce();

      // determine difference from this minima to starting minima
      double dist = l2vector(_allParams, initParams);

      cout << "[k=" << k << "][" << n << "/100]\t Score: " << rndScore << ", Opt. Score: " << ceresScore << ", Dist: " << dist << "\n";

      // determine if we did better
      if (abs(rndScore - localMin) > 0.01 && rndScore < localMin) {
        trialData["rndBetter"] = trialData["rndBetter"] + 1;
      }

      if (abs(ceresScore - localMin) > 0.01 && ceresScore < localMin) {
        trialData["ceresBetter"] = trialData["ceresBetter"] + 1;
        improvedDiffs.push_back(ceresScore - localMin);

        // determine if we're different than the old minima
        // TODO: no idea what would be a good threshold, maybe just like set it to 0.25?
        if (dist > 0.25) {
          trialData["timesNewMinimaFound"] = trialData["timesNewMinimaFound"] + 1;
        }
      }

      // track best score found
      if (ceresScore < trialData["bestScore"]) {
        trialData["bestScore"] = ceresScore;
        bestParams = _allParams;
        exportSolution("best_solution_" + to_string(k) + ".json");
      }

      // other stats
      scores.push_back(rndScore);
      ceresScores.push_back(ceresScore);
      distToStartOptima.push_back(dist);

      // reset for next run
      _allParams = initParams;
    }

    // json object update
    trialData["rndScores"] = scores;
    trialData["ceresScores"] = ceresScores;
    trialData["distToFirstOptima"] = distToStartOptima;
    trialData["bestParams"] = bestParams;

    double avgDiff = 0;
    for (auto& d : improvedDiffs) {
      avgDiff += d;
    }
    trialData["improvedScores"] = improvedDiffs;
    trialData["averageImprovement"] = avgDiff / improvedDiffs.size();

    trialResults.push_back(trialData);
  }

  ofstream outFile(_outDir + "full_result_summary.json");
  outFile << trialResults.dump(4);
}

void App::randomize()
{
  // just randomize the parameters and start tracking which minima we encounter
  // rng things
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<double> zeroOne(0, 1);

  vector<double> scores;
  vector<vector<double> > minima;
  vector<vector<double> > pixelVals;
  vector<int> minimaCount;
  vector<double> minimaScores;
  double best = DBL_MAX;

  // 1000 random trials by default
  int trials = 1000;
  double tolerance = 1;
  bool exportAll = false;
  float histInterval = 50;

  // distance at which we stop caring about how far away things are in the histogram
  float histMax = 1000;

  if (_config.count("random") > 0) {
    nlohmann::json localConfig = _config["random"];

    trials = (localConfig.count("trials") > 0) ? localConfig["trials"] : trials;
    tolerance = (localConfig.count("tolerance") > 0) ? localConfig["tolerance"] : tolerance;
    exportAll = (localConfig.count("outputAllResults") > 0) ? localConfig["outputAllResults"] : exportAll;
    histInterval = (localConfig.count("histInterval") > 0) ? localConfig["histInterval"] : histInterval;
    histMax = (localConfig.count("histMax") > 0) ? localConfig["histMax"] : histMax;
  }

  cout << "Starting randomization test.\n";
  cout << "Trials: " << trials << "\n";
  cout << "Tolerance: " << tolerance << "\n";

  int bestIndex = -1;

  for (int n = 0; n < trials; n++) {
    // randomize all parameters
    for (int i = 0; i < _allParams.size(); i++) {
      _allParams[i] = zeroOne(gen);
    }

    double startScore = eval();
    double score = runOptimizerOnce();

    cout << "[" << n + 1 << "/" << trials << "]\tScore: " << startScore << " -> " << score;

    scores.push_back(score);
    pixelVals.push_back(getLabVector(_allParams));
    
    // check distances to existing minima and find the closest (this is slow
    // but might be useful info)
    bool addNewMinima = true;
    double minDist = DBL_MAX;
    int closest = -1;

    for (int i = 0; i < minima.size(); i++) {
      double dist = pixelDist(_allParams, minima[i]);

      if (dist < tolerance) {
        //cout << " [OLD (" << i << ") Dist: " << dist << "]\n";
        addNewMinima = false;
        //minimaCount[i] = minimaCount[i] + 1;
        //continue;
      }
      
      if (dist < minDist) {
        minDist = dist;
        closest = i;
      }
    }

    // auto-accept a new best score
    if (score < best) {
      addNewMinima = true;
    }

    if (!addNewMinima) {
      cout << " [OLD (" << closest << ") Dist: " << minDist << "]\n";
      minimaCount[closest] = minimaCount[closest] + 1;
    }
    else {
      cout << " [NEW (" << closest << ": " << minDist << ")]\n";

      minima.push_back(_allParams);
      minimaCount.push_back(1);
      minimaScores.push_back(score);

      if (score < best) {
        best = score;
        bestIndex = minima.size() - 1;

        exportSolution("best_solution.json");
      }

      if (exportAll) {
        _data["internalID"] = minima.size() - 1;
        exportSolution("minima_" + to_string(minima.size() - 1) + ".json");
      }
    }
  }

  // little bit of analysis in output file
  // histogram tracking how many elements were close to the found global
  map<int, int> distHist;

  for (int i = 0; i < scores.size(); i++) {
    double dist = scores[i] - best;

    // each bracket contains elements less than or equal to the bracket number
    int bracket = (int) (((int)ceil(dist / histInterval)) * histInterval);

    if (bracket > histMax) {
      bracket = (int)histMax;
    }

    if (distHist.count(bracket) == 0)
      distHist[bracket] = 0;

    distHist[bracket] += 1;
  }

  // output scores n stuff
  nlohmann::json data;
  data["scores"] = scores;
  data["minima"] = minima;
  data["minimaCount"] = minimaCount;
  data["minimaScores"] = minimaScores;
  data["minimaFound"] = minima.size();
  data["pixelVals"] = pixelVals;

  nlohmann::json histogramLabels;
  nlohmann::json histogramValues;
  for (auto& kvp : distHist) {
    histogramLabels.push_back(kvp.first);
    histogramValues.push_back(kvp.second);
  }
  data["distanceHistogram"] = nlohmann::json({ { "labels", histogramLabels}, {"values", histogramValues} });

  nlohmann::json bestStats;
  bestStats["score"] = best;
  bestStats["count"] = minimaCount[bestIndex];
  bestStats["params"] = minima[bestIndex];
  bestStats["id"] = bestIndex;

  data["best"] = bestStats;

  ofstream outFile(_outDir + "randomization_summary.json");
  outFile << data.dump(4);
}

void App::evo()
{
  Evo evoObj(this);

  evoObj.init();
  evoObj.run();

  auto results = evoObj.getResults();

  for (int i = 0; i < results.size(); i++) {
    _allParams = results[i]._g;

    // typically ceres will always be first, if not just note this does NOT report
    // fitness values and instead reports the first objective function value
    _data["score"] = eval();

    exportSolution("evo_final_" + to_string(i) + ".json");
  }
}

void App::computeLtConstraints()
{
  // slow linear search here, there aren't that many times levels comesup (that I am aware of)
  auto params = _data["params"];

  for (int i = 0; i < params.size(); i++) {
    // we are looking for inMin and outMin
    if (params[i]["adjustmentName"] == "inMin" || params[i]["adjustmentName"] == "outMin") {
      string layer = params[i]["layerName"];
      string target = (params[i]["adjustmentName"] == "inMin") ? "inMax" : "outMax";

      // search for the corresponding element.
      for (int j = 0; j < params.size(); j++) {
        if (params[j]["layerName"] == layer && params[j]["adjustmentName"] == target) {
          _ltConstraints[i] = j;
        }
      }
    }
  }
}

vector<vector<int>> App::getAdjGroups()
{
  // theoretically all the adjustments in the json file are roughly grouped together
  // we want to group parameters of same adjustment type and same layer name together
  auto params = _data["params"];

  vector<vector<int>> paramGroups;
  string currentLayer = "";
  int currentType = -1;
  vector<int> currentGroup;

  for (int i = 0; i < params.size(); i++) {
    // need to check that params have same name, type, and then need to create a group
    if (currentType == params[i]["adjustmentType"] && currentLayer == params[i]["layerName"]) {
      currentGroup.push_back(params[i]["paramID"]);
    }
    else {
      if (currentGroup.size() > 0) {
        paramGroups.push_back(currentGroup);
      }

      // reset
      currentGroup.clear();
      currentLayer = params[i]["layerName"].get<string>();
      currentType = params[i]["adjustmentType"];
      currentGroup.push_back(params[i]["paramID"]);
    }
  }

  return paramGroups;
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

double App::l2vector(vector<double>& x1, vector<double>& x2)
{
  double sum = 0;
  for (int i = 0; i < x1.size(); i++) {
    double diff = x1[i] - x2[i];
    sum += diff * diff;
  }

  return sqrt(sum);
}

double App::pixelDist(vector<double>& x1, vector<double>& x2)
{
  // render each pixel and compare, could be slow dunno
  double sum = 0;

  for (int i = 0; i < _layerValues.size(); i++) {
    vector<double> v1 = ceresFunc(x1.data(), _layerValues[i]);
    vector<double> v2 = ceresFunc(x2.data(), _layerValues[i]);

    Utils<double>::LabColorT c1 = Utils<double>::RGBToLab(v1[0] * v1[3], v1[1] * v1[3], v1[2] * v1[3]);
    Utils<double>::LabColorT c2 = Utils<double>::RGBToLab(v2[0] * v2[3], v2[1] * v2[3], v2[2] * v2[3]);

    double diff = sqrt(pow(c1._L - c2._L, 2) + pow(c1._a - c2._a, 2) + pow(c1._b - c2._b, 2));

    sum += diff;
  }

  return sum / _layerValues.size();
}

vector<double> App::getLabVector(vector<double>& x)
{
  vector<double> colors;

  for (int i = 0; i < _layerValues.size(); i++) {
    vector<double> v = ceresFunc(x.data(), _layerValues[i]);
    Utils<double>::LabColorT c = Utils<double>::RGBToLab(v[0] * v[3], v[1] * v[3], v[2] * v[3]);
    colors.push_back(c._L);
    colors.push_back(c._a);
    colors.push_back(c._b);
  }

  return colors;
}

Evo::Evo(App* parent) : _parent(parent)
{
  init();
}

Evo::~Evo()
{
  stop();
}

void Evo::init()
{
  if (_searchRunning) {
    cout << "Can't change settings while search is running\n";
    return;
  }

  _searchRunning = false;
  _initialConfig = _parent->_allParams;

  // other objects n stuff

  // settings
  updateSettings();
}

void Evo::run()
{
  chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

  if (_searchRunning) {
    cout << "Stop search before calling run again.\n";
    return;
  }

  // init should have been called by this point
  _t = 0;

  vector<PopElem> pop = createPop();

  // archive of best elements so far
  vector<PopElem> arc;

  // for now we just have a max iteration termination criteria, this may change later
  while (_t < _maxIters) {
    if (_logLevel <= EvoLogLevel::CRITICAL) {
      cout << "Generation " << _t << "\n";
      cout << "================================================================\n";
    }

    if (_logLevel <= ABSURD) {
      _logData[_t] = nlohmann::json::object();
    }

    // optimize if setting is on
    if (_optimizeBeforeFitness) {
      optimizePop(pop);
    }

    computeObjectives(pop);
    
    if (_exportPopulations) {
      cout << "Exporting Population\n";
      exportPopElems("population", pop);
    }

    if (_exportArchives) {
      cout << "Exporting Archive\n";
      exportPopElems("archive", arc);
    }

    // combine the population and archive now
    pop.insert(pop.end(), arc.begin(), arc.end());

    // logging objective function values
    if (_logLevel <= ABSURD) {
      // objective functions
      nlohmann::json objVals;
      for (int i = 0; i < pop.size(); i++) {
        for (int j = 0; j < pop[i]._f.size(); j++) {
          objVals[i][j] = pop[i]._f[j];
        }
      }

      _logData[_t]["objectives"] = objVals;
    }

    assignFitness(pop);

    // preserve best things found so far according to fitness score
    updateOptimalSet(arc, pop);
    //pruneOptimalSet(arc);

    vector<PopElem> mate = select(pop, arc);

    // the archive is also always accessible to mate, we insert one copy of each archive
    // element here. multiple elements of arc may be in the mating pool, which should be ok
    mate.insert(mate.end(), arc.begin(), arc.end());

    pop = reproducePop(mate, arc);

    _t++;
  }

  if (_optimizeBeforeFitness || _optimizeFinal) {
    // final optimization run
    optimizePop(pop);
  }

  // combine the population and archive for final selection
  pop.insert(pop.end(), arc.begin(), arc.end());

  // final objectives and fitness
  // this time we just want to get the best ceres scored things
  if (_order == CERES_ORDER) {
    _finalArc = getBestCeresScores(pop);
  }
  else if (_order == FITNESS_ORDER) {
    assignFitness(pop);
    _finalArc = getBestFitnessScores(pop);
  }
  _finalPop = pop;

  chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();

  double runtime = chrono::duration_cast<chrono::duration<double>>(end - start).count();

  if (_logLevel <= ABSURD) {
    _logData[_t]["duration"] = runtime;
  }

  if (_logLevel <= ALL) {
    cout << "Runtime: " << runtime << "s\n";
  }

  if (_logLevel <= ABSURD) {
    ofstream outFile(_parent->_outDir + "evo_log.json");
    outFile << _logData.dump(2);
  }
}

void Evo::stop()
{
  if (_searchRunning) {
    _searchRunning = false;
    
    // might need to wait for threads eventually.
    // also this function is a bit useless right now without threads, as
    // run will terminate on its own

    _parent->_allParams = _initialConfig;
  }
}

vector<PopElem> Evo::getResults()
{
  return _finalArc;
}

void Evo::updateSettings()
{
  // defaults then load
  _mr = 0.3;
  _cr = 0.3;
  _ce = 0.25;
  _popSize = 100;
  _includeStartConfig = true;
  _maxIters = 50;
  _activeObjFuncs = { CERES, DISTANCE_FROM_START };
  _cmp = PARETO;
  _v = VARIETY_PRESERVING;
  _optimizeBeforeFitness = true;
  _equalityTolerance = 0.25;
  _arcSize = 0.1 * _popSize;
  _selectK = 2;
  _poolSize = _popSize / 2;
  _logLevel = ALL;
  _exportPopulations = false;
  _elitistRepro = false;
  _exportArchives = false;
  _optimizeFinal = false;
  _ceresRate = 0;
  _optimizeArc = false;
  _returnSize = 10;
  _order = CERES_ORDER;

  if (_parent->_config.count("evo") > 0) {
    nlohmann::json localConfig = _parent->_config["evo"];

    _mr = (localConfig.count("mutationRate") > 0) ? localConfig["mutationRate"] : _mr;
    _cr = (localConfig.count("crossoverRate") > 0) ? localConfig["crossoverRate"] : _cr;
    _ce = (localConfig.count("crossoverChance") > 0) ? localConfig["crossoverChance"] : _ce;
    _popSize = (localConfig.count("popSize") > 0) ? localConfig["popSize"] : _popSize;
    _includeStartConfig = (localConfig.count("includeStartConfig") > 0) ? localConfig["includeStartConfig"] : _includeStartConfig;
    _maxIters = (localConfig.count("maxIters") > 0) ? localConfig["maxIters"] : _maxIters;
    _cmp = (localConfig.count("comparisonMethod") > 0) ? localConfig["comparisonMethod"] : _cmp;
    _v = (localConfig.count("fitnessMethod") > 0) ? localConfig["fitnessMethod"] : _v;
    _optimizeBeforeFitness = (localConfig.count("optimizeBeforeFitness") > 0) ? localConfig["optimizeBeforeFitness"] : _optimizeBeforeFitness;
    _equalityTolerance = (localConfig.count("equalityTolerance") > 0) ? localConfig["equalityTolerance"] : _equalityTolerance;
    _arcSize = (localConfig.count("archiveSize") > 0) ? localConfig["archiveSize"] : _arcSize;
    _selectK = (localConfig.count("selectK") > 0) ? localConfig["selectK"] : _selectK;
    _poolSize = (localConfig.count("poolSize") > 0) ? localConfig["poolSize"] : _poolSize;
    _logLevel = (localConfig.count("logLevel") > 0) ? localConfig["logLevel"] : _logLevel;
    _exportPopulations = (localConfig.count("exportPopulations") > 0) ? localConfig["exportPopulations"] : _exportPopulations;
    _elitistRepro = (localConfig.count("elitistReproduction") > 0) ? localConfig["elitistReproduction"] : _elitistRepro;
    _exportArchives = (localConfig.count("exportArchives") > 0) ? localConfig["exportArchives"] : _exportArchives;
    _optimizeFinal = (localConfig.count("optimizeFinal") > 0) ? localConfig["optimizeFinal"] : _optimizeFinal;
    _ceresRate = (localConfig.count("ceresRate") > 0) ? localConfig["ceresRate"] : _ceresRate;
    _optimizeArc = (localConfig.count("optimizeArc") > 0) ? localConfig["optimizeArc"] : _optimizeArc;
    _returnSize = (localConfig.count("returnSize") > 0) ? localConfig["returnSize"] : _returnSize;
    _order = (localConfig.count("returnOrder") > 0) ? localConfig["returnOrder"] : _order;

    if (localConfig.count("objectives") > 0) {
      _activeObjFuncs.clear();
      for (int i = 0; i < localConfig["objectives"].size(); i++) {
        _activeObjFuncs.insert((ObjectiveFunctionType)(localConfig["objectives"][i].get<int>()));
      }
    }
  }
}

vector<PopElem> Evo::createPop()
{
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<double> zeroOne(0, 1);

  vector<PopElem> pop;
  int i = 0;

  if (_includeStartConfig) {
    pop.push_back(PopElem(_parent->_allParams));
    i++;
  }

  // i is initialized and used earlier
  for ( ; i < _popSize; i++) {
    vector<double> x;

    for (int j = 0; j < _parent->_allParams.size(); j++) {
      x.push_back(zeroOne(gen));
    }

    pop.push_back(PopElem(x));
  }

  return pop;
}

void Evo::computeObjectives(vector<PopElem>& pop)
{
  if (_logLevel <= VERBOSE) {
    cout << "Computing objective functions\n";
  }

  int i = 0;
  for (auto& p : pop) {
    if (_logLevel <= ALL) {
      cout << "[" << i + 1 << "/" << pop.size() << "]";
    }

    p._f.clear();

    for (auto& funcType : _activeObjFuncs) {
      if (funcType == CERES) {
        if (_logLevel <= ALL) 
          cout << " CERES";

        // run the ceres function
        _parent->_allParams = p._g;
        double score = _parent->eval();
        p._f.push_back(score);
      }
      else if (funcType == DISTANCE_FROM_START) {
        if (_logLevel <= ALL) 
          cout << " DISTANCE_FROM_START";

        // compare the current vector with the start vector
        double score = _parent->l2vector(p._g, _initialConfig);
        p._f.push_back(score);
      }
    }

    if (_logLevel <= ALL)
      cout << endl;

    i++;
  }
}

void Evo::assignFitness(vector<PopElem>& pop)
{
  switch (_v) {
  case FIRST_OBJECTIVE_NO_SCALING:
    firstObjectiveNoScalingFitness(pop);
    return;
  case VARIETY_PRESERVING:
    varietyPreservingFitness(pop);
    return;
  case PARETO_ORDERING:
    //paretoRankFitness(pop);
    return;
  default:
    return;
  }
}

void Evo::firstObjectiveNoScalingFitness(vector<PopElem>& pop)
{
  for (auto& p : pop) {
    // if the vector of objective functions is 0 this will crash
    // but honestly if you're running an optimization with 0 objectives you deserve it
    p._fitness = p._f[0];
  }
}

void Evo::varietyPreservingFitness(vector<PopElem>& pop)
{
  if (_logLevel <= VERBOSE)
    cout << "Computing Fitness Scores\n";

  // assign the comparison function
  function<int(PopElem&, PopElem&)> cmp;

  switch (_cmp) {
  case PARETO:
    cmp = [this](PopElem& x1, PopElem& x2) { return paretoCmp(x1, x2); };
    break;
  case WEIGHTED_SUM:
    cmp = [this](PopElem& x1, PopElem& x2) { return weightedSumCmp(x1, x2); };
    break;
  default:
    cmp = [this](PopElem& x1, PopElem& x2) { return paretoCmp(x1, x2); };
  }

  // gonna be a long one
  vector<int> ranks;
  ranks.resize(pop.size());
  int maxRank = 0;
  int objectives = pop[0]._f.size();

  // compute pareto ranks
  for (int i = 0; i < pop.size(); i++) {
    for (int j = i + 1; j < pop.size(); j++) {
      int k = cmp(pop[i], pop[j]);

      if (k < 0)
        ranks[j]++;
      else if (k > 0)
        ranks[i]++;
    }

    if (ranks[i] > maxRank)
      maxRank = ranks[i];
  }

  if (_logLevel <= ABSURD) {
    _logData[_t]["ranks"] = ranks;
  }

  // determine objective ranges
  vector<double> minf(objectives, DBL_MAX);
  vector<double> maxf(objectives, DBL_MIN);

  for (auto& p : pop) {
    for (int i = 0; i < objectives; i++) {
      if (p._f[i] < minf[i])
        minf[i] = p._f[i];

      if (p._f[i] > maxf[i])
        maxf[i] = p._f[i];
    }
  }

  vector<double> scale(objectives, 1);
  for (int i = 0; i < objectives; i++) {
    if (maxf[i] > minf[i])
      scale[i] = 1 / (maxf[i] - minf[i]);
  }

  // calculate the sharing value (for diversity)
  vector<double> shares(pop.size(), 0);
  double minShare = DBL_MAX;
  double maxShare = DBL_MIN;

  for (int i = 0; i < pop.size(); i++) {
    double current = shares[i];

    for (int j = i + 1; j < pop.size(); j++) {
      double dist = 0;

      for (int k = 0; k < objectives; k++) {
        dist += pow((pop[i]._f[k] - pop[j]._f[k]) * scale[k], 2);
      }

      double s = shareExp(dist, sqrt(objectives), 16);
      current += s;
      shares[j] += s;
    }
 
    shares[i] = current;
    if (current < minShare)
      minShare = current;

    if (current > maxShare)
      maxShare = current;
  }

  if (_logLevel <= ABSURD) {
    _logData[_t]["shares"] = shares;
  }

  // compute the actual fitness value
  double shareScale = (maxShare > minShare) ? 1 / (maxShare - minShare) : 1;
  vector<double> fitnessLog;

  for (int i = 0; i < pop.size(); i++) {
    if (ranks[i] > 0) {
      pop[i]._fitness = ranks[i] + sqrt(maxRank) * shareScale * (shares[i] - minShare);
    }
    else {
      pop[i]._fitness = shareScale * (shares[i] - minShare);
    }

    fitnessLog.push_back(pop[i]._fitness);
  }

  if (_logLevel <= ABSURD) {
    _logData[_t]["fitness"] = fitnessLog;
  }
}

int Evo::paretoCmp(PopElem & x1, PopElem & x2)
{
  // automatically have invalid elements be dominated by literally anything
  if (!isValid(x1) && !isValid(x2))
    return 0;
  if (!isValid(x1))
    return 1;
  if (!isValid(x2))
    return -1;

  // x1 dominates (<) x2 if at least one function value is < x2
  // and if everythign is <= x2's function values. <= here has a tolerance
  bool atLeastOneLess = false;
  bool allLeq = true;

  for (int i = 0; i < x1._f.size(); i++) {
    if (x1._f[i] < x2._f[i]) {
      // also satisfied condition 2 automatically
      atLeastOneLess = true;
    }
    // if not <, then check equality
    else if (abs(x1._f[i] - x2._f[i]) > _equalityTolerance) {
      allLeq = false;
    }
  }

  if (atLeastOneLess && allLeq)
    return -1; // x1 < x2

  // check other direction
  atLeastOneLess = false;
  allLeq = true;

  for (int i = 0; i < x1._f.size(); i++) {
    if (x2._f[i] < x1._f[i]) {
      // also satisfied condition 2 automatically
      atLeastOneLess = true;
    }
    // if not <, then check equality
    else if (abs(x1._f[i] - x2._f[i]) > _equalityTolerance) {
      allLeq = false;
    }
  }

  if (atLeastOneLess && allLeq)
    return 1; // x2 < x1

  // otherwise return 0
  return 0; // x1 == x2
}

int Evo::weightedSumCmp(PopElem & x1, PopElem & x2)
{
  double sum1 = 0;
  double sum2 = 0;
  
  for (int i = 0; i < x1._f.size(); i++) {
    sum1 += x1._f[i];
    sum2 += x2._f[i];
  }

  if (sum1 < sum2)
    return 1;
  else if (sum2 < sum1)
    return -1;
  else
    return 0;
}

double Evo::shareExp(double val, double sigma, double p)
{
  if (val <= 0)
    return 1;

  if (val >= sigma)
    return 0;

  return (exp(-(p * val) / sigma) - exp(-p)) / (1 - exp(-p));
}

vector<PopElem> Evo::select(vector<PopElem>& pop, vector<PopElem>& arc)
{
  // select here uses Ordered Selection, where the list is sorted then random
  // elements are selected biased towards the top.
  // ordered selection also preserves some of the lower end of the population to encourage
  // diversity
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<double> zeroOne(0, 1);

  double q = 1 / (1 - (log(_selectK) / log(_poolSize)));
  sort(pop.begin(), pop.end());
  vector<PopElem> mate;

  for (int i = 0; i < _poolSize; i++) {
    mate.push_back(pop[(int)(pow(zeroOne(gen), q) * pop.size())]);
  }

  return mate;
}

vector<PopElem> Evo::reproducePop(vector<PopElem>& mate, vector<PopElem>& elite)
{
  if (_logLevel <= VERBOSE) {
    cout << "Reproducing population\n";
  }

  // couple operations here: mutate, recombine, duplicate.
  // duplicate will not be used too much here, every element is assumed to be 
  // mutable so the chance that it passes through without mutations is vanishingly small
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<double> zeroOne(0, 1);

  vector<PopElem> newPop;
  for (int i = 0; i < _popSize; i++) {
    if (_logLevel <= ALL)
      cout << "[" << i + 1 << "/" << _popSize << "]";

    // pick an element, duplicate it to start
    PopElem p1 = PopElem(mate[(int)(zeroOne(gen) * mate.size())]._g);
    
    // crossover chance
    if (zeroOne(gen) < _cr) {
      if (_logLevel <= ALL)
        cout << " Crossover";

      // pick a random thing to crossover with
      PopElem p2 = mate[(int)(zeroOne(gen) * mate.size())];

      // if we're only crossing over from the elite pool, overwrite the previous selection
      // (slight performance hit but should be ok)
      if (_elitistRepro) {
        p2 = elite[(int)(zeroOne(gen) * elite.size())];
      }
      
      // do the crossover (randomly pick things to swap)
      for (int j = 0; j < p1._g.size(); j++) {
        if (zeroOne(gen) < _ce) {
          p1._g[j] = p2._g[j];
        }
      }
    }

    // mutation
    for (int j = 0; j < p1._g.size(); j++) {
      if (zeroOne(gen) < _mr) {
        if (_logLevel <= ALL)
          cout << " Mutation [" << j << "]";

        // randomize parameter value (for now, may do something more specific later)
        p1._g[j] = zeroOne(gen);
      }
    }

    // ceres happens after everything, if it happens
    if (zeroOne(gen) < _ceresRate) {
      if (_logLevel <= ALL) {
        cout << " Ceres";
      }

      // optimize the thing
      _parent->_allParams = p1._g;
      _parent->runOptimizerOnce(_logLevel <= ABSURD);
      p1._g = _parent->_allParams;
    }

    if (_logLevel <= ALL)
      cout << endl;

    newPop.push_back(p1);
  }

  return newPop;
}

void Evo::optimizePop(vector<PopElem>& pop)
{
  if (_logLevel <= VERBOSE) {
    cout << "Optimizing Population\n";
  }

  // runs the optimizer on each element
  for (int i = 0; i < pop.size(); i++) {
    if (_logLevel <= ALL) {
      cout << "[" << i + 1 << "/" << pop.size() << "]\n";
    }

    _parent->_allParams = pop[i]._g;
    _parent->runOptimizerOnce(_logLevel <= ABSURD);
    pop[i]._g = _parent->_allParams;
  }
}

void Evo::updateOptimalSet(vector<PopElem>& arc, vector<PopElem>& pop)
{
  // actually just take the fitness values and drop them into
  // the top n results of archive. During fitness value calculation, the
  // archive is added to the population for obtain a fitness ranking
  // this should ensure that an optimal set is preserved through each generation
  // and can be extracted at the end
  arc.clear();
  sort(pop.begin(), pop.end());

  for (int i = 0; i < pop.size(); i++) {
    // only add valid elements to the archive
    if (isValid(pop[i])) {
      if (_optimizeArc) {
        _parent->_allParams = pop[i]._g;
        _parent->runOptimizerOnce(_logLevel <= ABSURD);
        pop[i]._g = _parent->_allParams;
      }

      arc.push_back(pop[i]);

      if (arc.size() >= _arcSize)
        break;
    }
  }

}

void Evo::exportPopElems(string prefix, vector<PopElem>& x)
{
  for (int i = 0; i < x.size(); i++) {
    _parent->_allParams = x[i]._g;
    _parent->_data["score"] = x[i]._f[0];
    _parent->exportSolution(prefix + "_" + to_string(_t) + "_" + to_string(i) + ".json");
  }
}

vector<PopElem> Evo::getBestCeresScores(vector<PopElem>& pop)
{
  for (auto& p : pop) {
    _parent->_allParams = p._g;
    double score = _parent->eval();

    p._fitness = score;
  }

  return getBestFitnessScores(pop);
}

vector<PopElem> Evo::getBestFitnessScores(vector<PopElem>& pop)
{
  sort(pop.begin(), pop.end());

  vector<PopElem> ret;
  for (int i = 0; i < pop.size(); i++) {
    // only consider valid elements
    if (isValid(pop[i])) {
      ret.push_back(pop[i]);

      if (ret.size() >= _returnSize)
        break;
    }
  }

  return ret;
}

bool Evo::isValid(PopElem & p)
{
  // check lt constraints (key < value)
  for (auto& k : _parent->_ltConstraints) {
    if (p._g[k.first] >= p._g[k.second])
      return false;
  }

  return true;
}

void main(int argc, char* argv[])
{
	App app;

  if (string(argv[1]) == "config") {
    // load a config file instead
    app.goFromConfig(string(argv[2]));
  }
  else {
    app.go(string(argv[1]), string(argv[2]), string(argv[3]));
  }

	//cin.get();
}