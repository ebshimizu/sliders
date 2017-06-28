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
  else if (mode == "paramShiftTest") {
    paramShiftTest();
  }
  else if (mode == "adjShiftTest") {
    adjShiftTest();
  }
  else if (mode == "random") {
    randomize();
  }
}

void App::setupOptimizer(string loadFrom, string saveTo)
{
  vector<vector<double> > layerValues;
  vector<vector<double> > targetColors;
  vector<double> weights;
  _outDir = saveTo;

  _data = loadCeresData(loadFrom, _allParams, layerValues, targetColors, weights);

  // levels constraints
  computeLtConstraints();

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

  //cout << "Solver used: " << summary.linear_solver_type_used << endl;
  //cout << "Minimizer iters: " << summary.iterations.size() << endl;

  //double iterationTotalTime = 0.0;
  //int totalLinearItereations = 0;
  //for (auto &i : summary.iterations)
  //{
  //  iterationTotalTime += i.iteration_time_in_seconds;
  //  totalLinearItereations += i.linear_solver_iterations;
  //  cout << "Iteration: " << i.linear_solver_iterations << " " << i.iteration_time_in_seconds * 1000.0 << "ms" << endl;
  //}

  //cout << "Total iteration time: " << iterationTotalTime << endl;
  //cout << "Cost per linear solver iteration: " << iterationTotalTime * 1000.0 / totalLinearItereations << "ms" << endl;

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
  vector<int> minimaCount;
  vector<double> minimaScores;
  double best = DBL_MAX;

  // 1000 random trials
  for (int n = 0; n < 1000; n++) {
    // randomize all parameters
    for (int i = 0; i < _allParams.size(); i++) {
      _allParams[i] = zeroOne(gen);
    }

    double startScore = eval();
    double score = runOptimizerOnce();

    cout << "[" << n << "/1000]\tScore: " << startScore << " -> " << score << "\n";

    scores.push_back(score);
    
    // check distances to existing minima
    bool addNewMinima = true;
    for (int i = 0; i < minima.size(); i++) {
      double dist = l2vector(_allParams, minima[i]);

      if (dist < 1) {
        addNewMinima = false;
        minimaCount[i] = minimaCount[i] + 1;
        break;
      }
    }

    if (addNewMinima) {
      minima.push_back(_allParams);
      minimaCount.push_back(1);
      minimaScores.push_back(score);

      if (score < best) {
        best = score;
        exportSolution("best_solution.json");
      }
      //exportSolution("minima_" + to_string(minima.size()) + ".json");
    }
  }

  // output scores n stuff
  nlohmann::json data;
  data["scores"] = scores;
  data["minima"] = minima;
  data["minimaCount"] = minimaCount;
  data["minimaScores"] = minimaScores;
  data["minimaFound"] = minima.size();

  ofstream outFile(_outDir + "randomization_summary.json");
  outFile << data.dump(4);
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

void main(int argc, char* argv[])
{
	App app;
	app.go(string(argv[1]), string(argv[2]), string(argv[3]));

	//cin.get();
}