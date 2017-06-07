#include "expressionStep.h"
#pragma once

//
// addition
//
inline ExpStep operator + (const ExpStep &a, const ExpStep &b)
{
  if (a.context == nullptr && b.context == nullptr) {
    return ExpStep(a.value + b.value);
  }
  if (a.context == nullptr) {
    ExpStep aStep = b.context->registerConstant(a.value);
    return aStep + b;
  }
  if (b.context == nullptr) {
    ExpStep bStep = a.context->registerConstant(b.value);
    return a + bStep;
  }

	return a.context->addStep( ExpStepData(ExpOpType::add, a.stepIndex, b.stepIndex) );
}

inline ExpStep operator + (const ExpStep &a, double b)
{
	ExpStep bStep = a.context->registerConstant(b);
	return a + bStep;
}

inline ExpStep operator + (double a, const ExpStep &b)
{
	ExpStep aStep = b.context->registerConstant(a);
	return aStep + b;
}

//
// multiplication
//
inline ExpStep operator * (const ExpStep &a, const ExpStep &b)
{
  if (a.context == nullptr && b.context == nullptr) {
    return ExpStep(a.value * b.value);
  }
  if (a.context == nullptr) {
    ExpStep aStep = b.context->registerConstant(a.value);
    return aStep * b;
  }
  if (b.context == nullptr) {
    ExpStep bStep = a.context->registerConstant(b.value);
    return a * bStep;
  }

  return a.context->addStep(ExpStepData(ExpOpType::multiply, a.stepIndex, b.stepIndex));
}

inline ExpStep operator * (const ExpStep &a, double b)
{
	ExpStep bStep = a.context->registerConstant(b);
	return a * bStep;
}

inline ExpStep operator * (double a, const ExpStep &b)
{
	ExpStep aStep = b.context->registerConstant(a);
	return aStep * b;
}

//
// subtraction
//
inline ExpStep operator - (const ExpStep &a, const ExpStep &b)
{
  if (a.context == nullptr && b.context == nullptr) {
    return ExpStep(a.value - b.value);
  }
  if (a.context == nullptr) {
    ExpStep aStep = b.context->registerConstant(a.value);
    return aStep - b;
  }
  if (b.context == nullptr) {
    ExpStep bStep = a.context->registerConstant(b.value);
    return a - bStep;
  }

	return a.context->addStep(ExpStepData(ExpOpType::subtract, a.stepIndex, b.stepIndex));
}

inline ExpStep operator - (const ExpStep &a, double b)
{
	ExpStep bStep = a.context->registerConstant(b);
	return a - bStep;
}

inline ExpStep operator - (double a, const ExpStep &b)
{
	ExpStep aStep = b.context->registerConstant(a);
	return aStep - b;
}

//
// division
//
inline ExpStep operator / (const ExpStep &a, const ExpStep &b)
{
  if (a.context == nullptr && b.context == nullptr) {
    return ExpStep(a.value / b.value);
  }
  if (a.context == nullptr) {
    ExpStep aStep = b.context->registerConstant(a.value);
    return aStep / b;
  }
  if (b.context == nullptr) {
    ExpStep bStep = a.context->registerConstant(b.value);
    return a / bStep;
  }

	return a.context->addStep(ExpStepData(ExpOpType::divide, a.stepIndex, b.stepIndex));
}

inline ExpStep operator / (const ExpStep &a, double b)
{
	ExpStep bStep = a.context->registerConstant(b);
	return a / bStep;
}

inline ExpStep operator / (double a, const ExpStep &b)
{
	ExpStep aStep = b.context->registerConstant(a);
	return aStep / b;
}

//
// unary operators
//
inline ExpStep operator - (const ExpStep &a)
{
	return a.context->addStep( ExpStepData(ExpOpType::negate, a.stepIndex) );
}

inline ExpStep sin(const ExpStep &a)
{
	return a.context->addStep(ExpStepData(ExpOpType::sin, a.stepIndex));
}

inline ExpStep cos(const ExpStep &a)
{
	return a.context->addStep(ExpStepData(ExpOpType::cos, a.stepIndex));
}

inline ExpStep tan(const ExpStep &a)
{
	return a.context->addStep(ExpStepData(ExpOpType::tan, a.stepIndex));
}

inline ExpStep sqrt(const ExpStep &a)
{
	return a.context->addStep(ExpStepData(ExpOpType::sqrt, a.stepIndex));
}

inline ExpStep pow(const ExpStep &a, const ExpStep &b)
{
	return a.context->addStep(ExpStepData(ExpOpType::pow, a.stepIndex, b.stepIndex));
}

inline ExpStep pow(double a, const ExpStep &b)
{
	ExpStep aStep = b.context->registerConstant(a);
	return b.context->addStep(ExpStepData(ExpOpType::pow, aStep.stepIndex, b.stepIndex));
}

inline ExpStep pow(const ExpStep &a, double b)
{
	ExpStep bStep = a.context->registerConstant(b);
	return a.context->addStep(ExpStepData(ExpOpType::pow, a.stepIndex, bStep.stepIndex));
}

inline ExpStepData::ExpStepData(ExpOpType _op, int _operand0Step, int _operand1Step)
{
	init();
	type = ExpStepType::binaryOp;
	op = _op;
	operand0Step = _operand0Step;
	operand1Step = _operand1Step;
}

inline ExpStepData::ExpStepData(ExpOpType _op, int _operand0Step)
{
	init();
	type = ExpStepType::unaryOp;
	op = _op;
	operand0Step = _operand0Step;
}

inline ExpStepData ExpStepData::makeParameter(const string &paramName, double defaultValue, int _parameterSlot, int _parameterIndex)
{
	ExpStepData result;
	result.type = ExpStepType::parameter;
	result.value = defaultValue;
	result.parameterSlot = _parameterSlot;
	result.parameterIndex = _parameterIndex;
	result.name = paramName;
	return result;
}

inline ExpStepData ExpStepData::makeResult(const string &resultName, int stepIndex, int resultIndex)
{
	ExpStepData result;
	result.type = ExpStepType::result;
	result.operand0Step = stepIndex;
	result.resultIndex = resultIndex;
	result.name = resultName;
	return result;
}

inline ExpStepData ExpStepData::makeFunctionCall(int functionIndex, const vector<ExpStep> &parameters)
{
	ExpStepData result;
	result.type = ExpStepType::functionCall;
	result.functionIndex = functionIndex;
	for (auto &it : parameters)
		result.functionParamStepIndices.push_back(it.stepIndex);
	return result;
}

inline ExpStepData ExpStepData::makeFunctionOutput(int funcStepIndex, int outputIndex)
{
	ExpStepData result;
	result.type = ExpStepType::functionOutput;
	result.functionStepIndex = funcStepIndex;
	result.functionOutputIndex = outputIndex;
	return result;
}

inline vector<string> ExpStepData::toSourceCode(const ExpContext &parent) const
{
	//const string floatType = useFloat ? "float" : "double";
	const string floatType = "T";
	vector<string> result;
	if (type == ExpStepType::functionCall)
	{
		string paramList = "";
		for (int i = 0; i < (int)functionParamStepIndices.size(); i++)
		{
			paramList += "s" + to_string(functionParamStepIndices[i]);
			if (i != functionParamStepIndices.size() - 1)
				paramList += ", ";
		}

		string vectorLine = "vector<" + floatType + "> v" + to_string(stepIndex) + " = { " + paramList + " }";
		string callLine = "auto s" + to_string(stepIndex) + " = " + parent.functionList[functionIndex] + "(v" + to_string(stepIndex) + ")";
		result.push_back(vectorLine);
		result.push_back(callLine);
		return result;
	}
	result.push_back("invalid");
	return result;
}