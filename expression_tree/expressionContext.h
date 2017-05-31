#pragma once

enum class ETreeType
{
	constant,
	variable,
	unaryOp,
	binaryOp,
	invalid
};

enum class ETreeOpType
{
	// unary ops
	sin,
	cos,
	tan,
	negate,
	sqrt,

	// binary ops
	add,
	subtract,
	multiply,
	divide,
	pow,

	invalid
};

inline string getOpName(ETreeOpType op)
{
	switch (op)
	{
	case ETreeOpType::sin: return "sin";
	case ETreeOpType::cos: return "cos";
	case ETreeOpType::tan: return "tan";
	case ETreeOpType::negate: return "-";
	case ETreeOpType::sqrt: return "sqrt";
	case ETreeOpType::add: return " + ";
	case ETreeOpType::subtract: return " - ";
	case ETreeOpType::multiply: return " * ";
	case ETreeOpType::divide: return " / ";
	case ETreeOpType::pow: return " ^ ";
	}
	return "invalid";
}

struct ExpContext;

// a step in the expression context
struct ExpStep
{
	ExpStep()
	{
		init();
	}
	explicit ExpStep(double constantValue)
	{
		init();
		type = ETreeType::constant;
		value = constantValue;
	}
	ExpStep(ETreeOpType _op, const ExpStep &c0, const ExpStep &c1);
	ExpStep(ETreeOpType _op, double c0, const ExpStep &c1);
	ExpStep(ETreeOpType _op, const ExpStep &c0, double c1);
	ExpStep(ETreeOpType _op, const ExpStep &c0);
	ExpStep(ExpContext &_context, double defaultValue, const string &_variableName, int _variableIndex);

	void init()
	{
		type = ETreeType::invalid;
		value = numeric_limits<double>::max();
		variableIndex = -1;
		operand0Step = -1;
		operand1Step = -1;
		context = nullptr;
		contextStepIndex = -1;
	}

	double eval(const vector<double> &values) const
	{
		if (type == ETreeType::constant || type == ETreeType::variable)
		{
			return value;
		}
		else if (type == ETreeType::unaryOp)
		{
			if (op == ETreeOpType::sin) return sin(values[operand0Step]);
			if (op == ETreeOpType::cos) return cos(values[operand0Step]);
			if (op == ETreeOpType::tan) return tan(values[operand0Step]);
			if (op == ETreeOpType::negate) return -values[operand0Step];
			if (op == ETreeOpType::sqrt) return sqrt(values[operand0Step]);
			assert(false);
			cout << "unknown unary op" << endl;
			return 0.0;
		}
		else if (type == ETreeType::binaryOp)
		{
			if (op == ETreeOpType::add) return values[operand0Step] + values[operand1Step];
			if (op == ETreeOpType::subtract) return values[operand0Step] - values[operand1Step];
			if (op == ETreeOpType::multiply) return values[operand0Step] * values[operand1Step];
			if (op == ETreeOpType::divide) return values[operand0Step] / values[operand1Step];
			if (op == ETreeOpType::pow) return pow(values[operand0Step], values[operand1Step]);
			assert(false);
			cout << "unknown binary op" << endl;
			return 0.0;
		}
		else
		{
			assert(false);
			cout << "unknown ETreeType" << endl;
			return 0.0;
		}
	}

	string toString() const
	{
		if (type == ETreeType::constant)
		{
			return to_string(value);
		}
		else if (type == ETreeType::variable)
		{
			return "v" + to_string(variableIndex);
		}
		else if (type == ETreeType::unaryOp)
		{
			return getOpName(op) + "(r" + to_string(operand0Step) + ")";
		}
		else if (type == ETreeType::binaryOp)
		{
			return "(r" + to_string(operand0Step) + getOpName(op) + "r" + to_string(operand1Step) + ")";
		}
		return "invalid";
	}

	ETreeType type;
	
	// valid for constants and variables
	double value;

	// valid for variables
	int variableIndex;

	// valid for unary and binary ops
	ETreeOpType op;
	int operand0Step;
	int operand1Step;

	ExpContext *context;
	int contextStepIndex;
};

// the context in which a set of expressions is executed
struct ExpContext
{
	int registerConstant(double value)
	{
		ExpStep newStep(value);
		addStep(newStep);
		return newStep.contextStepIndex;
	}

	void addStep(ExpStep &step)
	{
		step.context = this;
		step.contextStepIndex = steps.size();
		steps.push_back(step);
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

	string toString() const
	{
		string result;
		for (int i = 0; i < (int)steps.size(); i++)
		{
			result += "r" + to_string(i) + " = " + steps[i].toString() + "\n";
		}
		return result;
	}

	vector<ExpStep> steps;
};

//
// addition
//
inline ExpStep operator + (const ExpStep &a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::add, a, b);
}

inline ExpStep operator + (const ExpStep &a, double b)
{
	return ExpStep(ETreeOpType::add, a, b);
}

inline ExpStep operator + (double a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::add, a, b);
}

//
// multiplication
//
inline ExpStep operator * (const ExpStep &a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::multiply, a, b);
}

inline ExpStep operator * (const ExpStep &a, double b)
{
	return ExpStep(ETreeOpType::multiply, a, b);
}

inline ExpStep operator * (double a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::multiply, a, b);
}

//
// subtraction
//
inline ExpStep operator - (const ExpStep &a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::subtract, a, b);
}

inline ExpStep operator - (const ExpStep &a, double b)
{
	return ExpStep(ETreeOpType::subtract, a, b);
}

inline ExpStep operator - (double a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::subtract, a, b);
}

//
// division
//
inline ExpStep operator / (const ExpStep &a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::divide, a, b);
}

inline ExpStep operator / (const ExpStep &a, double b)
{
	return ExpStep(ETreeOpType::divide, a, b);
}

inline ExpStep operator / (double a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::divide, a, b);
}

//
// unary operators
//
inline ExpStep operator - (const ExpStep &a)
{
	return ExpStep(ETreeOpType::negate, a);
}

inline ExpStep sin(const ExpStep &a)
{
	return ExpStep(ETreeOpType::sin, a);
}

inline ExpStep cos(const ExpStep &a)
{
	return ExpStep(ETreeOpType::cos, a);
}

inline ExpStep tan(const ExpStep &a)
{
	return ExpStep(ETreeOpType::tan, a);
}

inline ExpStep sqrt(const ExpStep &a)
{
	return ExpStep(ETreeOpType::sqrt, a);
}

inline ExpStep pow(const ExpStep &a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::pow, a, b);
}

inline ExpStep pow(double a, const ExpStep &b)
{
	return ExpStep(ETreeOpType::pow, a, b);
}

inline ExpStep pow(const ExpStep &a, double b)
{
	return ExpStep(ETreeOpType::pow, a, b);
}

inline ExpStep::ExpStep(ETreeOpType _op, const ExpStep &c0, const ExpStep &c1)
{
	init();
	type = ETreeType::binaryOp;
	op = _op;
	operand0Step = c0.contextStepIndex;
	operand1Step = c1.contextStepIndex;
	c0.context->addStep(*this);
}

inline ExpStep::ExpStep(ETreeOpType _op, double c0, const ExpStep &c1)
{
	init();
	type = ETreeType::binaryOp;
	op = _op;
	operand0Step = c1.context->registerConstant(c0);
	operand1Step = c1.contextStepIndex;
	c1.context->addStep(*this);
}

inline ExpStep::ExpStep(ETreeOpType _op, const ExpStep &c0, double c1)
{
	init();
	type = ETreeType::binaryOp;
	op = _op;
	operand0Step = c0.context->registerConstant(c1);
	operand1Step = c0.contextStepIndex;
	c0.context->addStep(*this);
}

inline ExpStep::ExpStep(ETreeOpType _op, const ExpStep &c0)
{
	init();
	type = ETreeType::unaryOp;
	op = _op;
	operand0Step = c0.contextStepIndex;
	c0.context->addStep(*this);
}

inline ExpStep::ExpStep(ExpContext &_context, double defaultValue, const string &_variableName, int _variableIndex)
{
	init();
	context = &_context;
	type = ETreeType::variable;
	value = defaultValue;
	variableIndex = _variableIndex;
	context->addStep(*this);
}