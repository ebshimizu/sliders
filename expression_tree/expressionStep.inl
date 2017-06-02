#pragma once

//
// addition
//
inline ExpStep operator + (const ExpStep &a, const ExpStep &b)
{
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

inline ExpStepData ExpStepData::makeParameter(const string &paramName, double defaultValue, int _parameterIndex)
{
	ExpStepData result;
	result.type = ExpStepType::parameter;
	result.value = defaultValue;
	result.parameterIndex = _parameterIndex;
	return result;
}

inline ExpStepData ExpStepData::makeResult(int stepIndex, int resultIndex)
{
	ExpStepData result;
	result.type = ExpStepType::result;
	result.operand0Step = stepIndex;
	result.resultIndex = resultIndex;
	return result;
}