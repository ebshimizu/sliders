#pragma once

enum class ExpStepType
{
	constant,
	parameter,
	result,
	unaryOp,
	binaryOp,
	functionCall,
	functionOutput,
	invalid
};

enum class ExpOpType
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

inline string getOpName(ExpOpType op)
{
	switch (op)
	{
	case ExpOpType::sin: return "sin";
	case ExpOpType::cos: return "cos";
	case ExpOpType::tan: return "tan";
	case ExpOpType::negate: return "-";
	case ExpOpType::sqrt: return "sqrt";

	case ExpOpType::add: return " + ";
	case ExpOpType::subtract: return " - ";
	case ExpOpType::multiply: return " * ";
	case ExpOpType::divide: return " / ";
	case ExpOpType::pow: return " ^ ";
	}
	return "invalid";
}

struct ExpContext;

struct ExpStep
{
	ExpStep()
	{

	}

	// construct with context and step index
	ExpStep(ExpContext *_context, int _stepIndex)
	{
		init();
		context = _context;
		stepIndex = _stepIndex;
	}

	// constant constructor
	ExpStep(double constantValue)
	{
		init();
		value = constantValue;
	}

	ExpStep(int constantValue)
	{
		init();
		value = constantValue;
	}

	ExpStep(unsigned int constantValue)
	{
		init();
		value = constantValue;
	}

	ExpStep(float constantValue)
	{
		init();
		value = constantValue;
	}

	void init()
	{
		context = nullptr;
		value = numeric_limits<double>::max();
		stepIndex = -1;
	}

	// valid for empty contexts
	double value;

	ExpContext *context;
	int stepIndex;
};

// a step in the expression context
struct ExpStepData
{
	ExpStepData()
	{
		init();
	}

	// constant constructor
	explicit ExpStepData(double constantValue)
	{
		init();
		type = ExpStepType::constant;
		value = constantValue;
	}

	// binary operator constructor
	ExpStepData(ExpOpType _op, int _operand0Step, int _operand1Step);

	// unary operator constructor
	ExpStepData(ExpOpType _op, int _operand0Step);

	// parameter constructor
	static ExpStepData makeParameter(const string &paramName, double defaultValue, int _parameterIndex);

	// result constructor
	static ExpStepData makeResult(int stepIndex, int resultIndex);

	static ExpStepData makeFunctionCall(int functionIndex, const vector<ExpStep> &parameters);

	static ExpStepData makeFunctionOutput(int funcStepIndex, int outputIndex);

	void init()
	{
		type = ExpStepType::invalid;
		value = numeric_limits<double>::max();
		parameterIndex = -1;
		operand0Step = -1;
		operand1Step = -1;
		stepIndex = -1;
		resultIndex = -1;
	}
	
	double eval(const vector<double> &values) const
	{
		if (type == ExpStepType::constant || type == ExpStepType::parameter)
		{
			return value;
		}
		else if (type == ExpStepType::result)
		{
			return values[operand0Step];
		}
		else if (type == ExpStepType::unaryOp)
		{
			if (op == ExpOpType::sin) return sin(values[operand0Step]);
			if (op == ExpOpType::cos) return cos(values[operand0Step]);
			if (op == ExpOpType::tan) return tan(values[operand0Step]);
			if (op == ExpOpType::negate) return -values[operand0Step];
			if (op == ExpOpType::sqrt) return sqrt(values[operand0Step]);
			assert(false);
			cout << "unknown unary op" << endl;
			return 0.0;
		}
		else if (type == ExpStepType::binaryOp)
		{
			if (op == ExpOpType::add) return values[operand0Step] + values[operand1Step];
			if (op == ExpOpType::subtract) return values[operand0Step] - values[operand1Step];
			if (op == ExpOpType::multiply) return values[operand0Step] * values[operand1Step];
			if (op == ExpOpType::divide) return values[operand0Step] / values[operand1Step];
			if (op == ExpOpType::pow) return pow(values[operand0Step], values[operand1Step]);
			assert(false);
			cout << "unknown binary op" << endl;
			return 0.0;
		}
		else if (type == ExpStepType::functionCall)
		{
			//cout << "eval not supported for functions" << endl;
			return 0.0;
		}
		else if (type == ExpStepType::functionOutput)
		{
			//cout << "eval not supported for functions" << endl;
			return 0.0;
		}
		else
		{
			assert(false);
			cout << "unknown ExpStepType" << endl;
			return 0.0;
		}
	}

	string toSourceCode(bool useFloat) const
	{
		const string floatType = useFloat ? "float" : "double";
		const string assignment = "const " + floatType + " s" + to_string(stepIndex) + " = ";
		if (type == ExpStepType::constant)
		{
			return assignment + to_string(value);
		}
		else if (type == ExpStepType::parameter)
		{
			return assignment + "params[" + to_string(parameterIndex) + "]";
		}
		else if (type == ExpStepType::result)
		{
			return "result[" + to_string(resultIndex) + "] = s" + to_string(operand0Step);
		}
		else if (type == ExpStepType::unaryOp)
		{
			return assignment + getOpName(op) + "(s" + to_string(operand0Step) + ")";
		}
		else if (type == ExpStepType::binaryOp)
		{
			if (op == ExpOpType::pow)
			{
				return assignment + "pow(s" + to_string(operand0Step) + ", s" + to_string(operand1Step) + ")";
			}
			else
			{
				return assignment + "s" + to_string(operand0Step) + getOpName(op) + "s" + to_string(operand1Step);
			}
		}
		else if (type == ExpStepType::functionOutput)
		{
			return assignment + "s" + to_string(functionStepIndex) + "[" + to_string(functionOutputIndex) + "]";
		}
		return "invalid";
	}

	vector<string> toSourceCode(const ExpContext & parent, bool useFloat) const;

	ExpStepType type;
	
	// valid for constants and variables
	double value;

	// valid for variables
	int parameterIndex;
	int resultIndex;

	// valid for functions
	int functionIndex;
	vector<int> functionParamStepIndices;

	//valid for function outputs
	int functionStepIndex;
	int functionOutputIndex;

	// valid for unary and binary ops
	ExpOpType op;
	int operand0Step;
	int operand1Step;

	int stepIndex;
};
