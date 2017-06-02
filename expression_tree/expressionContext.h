#pragma once

#include "expressionStep.h"

// the context in which a set of expressions is executed
struct ExpContext
{
	struct FunctionInfo
	{
		FunctionInfo()
		{
			globalIndex = -1;
			paramCount = -1;
			resultCount = -1;
		}
		FunctionInfo(int _globalIndex, int _paramCount, int _resultCount)
		{
			globalIndex = _globalIndex;
			paramCount = _paramCount;
			resultCount = _resultCount;
		}
		int globalIndex;
		int paramCount;
		int resultCount;
	};

	ExpContext()
	{
		parameterCount = 0;
		resultCount = 0;
	}

	ExpStep registerConstant(double value)
	{
		ExpStepData newStep(value);
		addStep(newStep);
		return ExpStep(this, newStep.stepIndex);
	}

	ExpStep registerParam(int paramIndex, string unusedParamName = "", double unusedDefaultValue = numeric_limits<double>::max())
	{
		ExpStepData step = ExpStepData::makeParameter(unusedParamName, unusedDefaultValue, parameterCount);
		addStep(step);
		parameterCount = max(parameterCount, paramIndex + 1);
		return ExpStep(this, step.stepIndex);
	}

	void registerResult(const ExpStep &step, int resultIndex)
	{
		ExpStepData stepData = ExpStepData::makeResult(step.stepIndex, resultIndex);
		addStep(stepData);
		resultCount = max(resultCount, resultIndex + 1);
	}

	ExpStep addStep(ExpStepData &step)
	{
		step.stepIndex = (int)steps.size();
		steps.push_back(step);
		return ExpStep(this, step.stepIndex);
	}

	void registerFunc(const string &functionName, int parameterCount, int resultCount)
	{
		int functionIndex = (int)functionList.size();
		functionList.push_back(functionName);
		functions[functionName] = FunctionInfo(functionIndex, parameterCount, resultCount);
	}

	vector<ExpStep> callFunc(const string &functionName, const ExpStep &p0)
	{
		vector<ExpStep> params;
		params.push_back(p0);
		return callFunc(functionName, params);
	}

	vector<ExpStep> callFunc(const string &functionName, const ExpStep &p0, const ExpStep &p1)
	{
		vector<ExpStep> params;
		params.push_back(p0);
		params.push_back(p1);
		return callFunc(functionName, params);
	}

	vector<ExpStep> callFunc(const string &functionName, const ExpStep &p0, const ExpStep &p1, const ExpStep &p2)
	{
		vector<ExpStep> params;
		params.push_back(p0);
		params.push_back(p1);
		params.push_back(p2);
		return callFunc(functionName, params);
	}

	vector<ExpStep> callFunc(const string &functionName, const ExpStep &p0, const ExpStep &p1, const ExpStep &p2, const ExpStep &p3)
	{
		vector<ExpStep> params;
		params.push_back(p0);
		params.push_back(p1);
		params.push_back(p2);
		params.push_back(p3);
		return callFunc(functionName, params);
	}

	vector<ExpStep> callFunc(const string &functionName, const vector<ExpStep> &params)
	{
		if (functions.count(functionName) == 0)
		{
			cout << "Function not found: " << functionName << endl;
		}
		const FunctionInfo &info = functions[functionName];
		ExpStepData callStepData = ExpStepData::makeFunctionCall(info.globalIndex, params);
		addStep(callStepData);

		vector<ExpStep> result;
		for (int outputIndex = 0; outputIndex < info.resultCount; outputIndex++)
		{
			ExpStepData outputStepData = ExpStepData::makeFunctionOutput(callStepData.stepIndex, outputIndex);
			result.push_back(addStep(outputStepData));
		}
		return result;
	}

	double eval() const
	{
		vector<double> values;
		for (int i = 0; i < (int)steps.size(); i++)
		{
			values.push_back(steps[i].eval(values));
		}
		return values.back();
	}

	vector<string> toSourceCode(const string &functionName, bool useFloat) const
	{
		const string floatType = useFloat ? "float" : "double";
		const string indent = "    ";
		const string vectorType = "vector<" + floatType + ">";
		vector<string> result;
		result.push_back(vectorType + " " + functionName + "(const " + vectorType + " &params)");
		result.push_back("{");
		result.push_back(indent + vectorType + " result(" + to_string(resultCount) + ");");
		result.push_back(indent);
		for (const ExpStepData &s : steps)
		{
			if (s.type == ExpStepType::functionCall)
			{
				for(const string &line : s.toSourceCode(*this, useFloat))
					result.push_back(indent + line + ";");
			}
			else
			{
				result.push_back(indent + s.toSourceCode(useFloat) + ";");
			}
		}
		result.push_back(indent);
		result.push_back(indent + "return result;");
		result.push_back("}");
		return result;
	}

	map<string, FunctionInfo> functions;
	vector<string> functionList;
	vector<ExpStepData> steps;
	int parameterCount, resultCount;
};

#include "expressionStep.inl"