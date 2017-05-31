#pragma once

//
// deprecated, uses expression trees instead of expression contexts
//

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

struct ETreeData
{
	ETreeData()
	{
		init();
	}

	explicit ETreeData(double constantValue)
	{
		init();
		type = ETreeType::constant;
		value = constantValue;
	}
	ETreeData(ETreeOpType _op, const ETreeData *c0, const ETreeData *c1)
	{
		init();
		type = ETreeType::binaryOp;
		op = _op;
		child0 = c0;
		child1 = c1;
	}
	ETreeData(ETreeOpType _op, const ETreeData *c0)
	{
		init();
		type = ETreeType::unaryOp;
		op = _op;
		child0 = c0;
	}
	explicit ETreeData(double defaultValue, const string &_variableName, int _variableIndex)
	{
		init();
		type = ETreeType::variable;
		value = defaultValue;
		variableIndex = _variableIndex;
	}

	ETreeType type;
	ETreeOpType op;

	// valid for constants and variables
	double value;

	// valid for variables
	int variableIndex;

	// valid for unary and binary ops
	const ETreeData *child0;

	// valid forbinary ops
	const ETreeData *child1;

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
			return getOpName(op) + "(" + child0->toString() + ")";
		}
		else if (type == ETreeType::binaryOp)
		{
			return "(" + child0->toString() + getOpName(op) + child1->toString() + ")";
		}
	}

	double eval() const
	{
		if (type == ETreeType::constant || type == ETreeType::variable)
		{
			return value;
		}
		else if (type == ETreeType::unaryOp)
		{
			if (op == ETreeOpType::sin) return sin(child0->eval());
			if (op == ETreeOpType::cos) return cos(child0->eval());
			if (op == ETreeOpType::tan) return tan(child0->eval());
			if (op == ETreeOpType::negate) return -(child0->eval());
			if (op == ETreeOpType::sqrt) return sqrt(child0->eval());
			assert(false);
			cout << "unknown unary op" << endl;
			return 0.0;
		}
		else if (type == ETreeType::binaryOp)
		{
			if (op == ETreeOpType::add) return child0->eval() + child1->eval();
			if (op == ETreeOpType::subtract) return child0->eval() - child1->eval();
			if (op == ETreeOpType::multiply) return child0->eval() * child1->eval();
			if (op == ETreeOpType::divide) return child0->eval() / child1->eval();
			if (op == ETreeOpType::pow) return pow(child0->eval(), child1->eval());
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

private:
	void init()
	{
		type = ETreeType::invalid;
		op = ETreeOpType::invalid;
		value = numeric_limits<float>::max();
		variableIndex = -1;
		child0 = nullptr;
		child1 = nullptr;
	}
};

struct ETree
{
	ETree()
	{
		data = nullptr;
	}
	explicit ETree(const ETreeData *_data)
	{
		data = _data;
	}
	explicit ETree(double constantValue)
	{
		data = new ETreeData(constantValue);
	}
	ETree(ETreeOpType op, const ETree &left)
	{
		data = new ETreeData(op, left.data);
	}
	ETree(ETreeOpType op, const ETree &left, const ETree &right)
	{
		data = new ETreeData(op, left.data, right.data);
	}
	ETree(double defaultValue, const string &variableName, int variableIndex)
	{
		data = new ETreeData(defaultValue, variableName, variableIndex);
	}

	double eval() const
	{
		return data->eval();
	}

	string toString() const
	{
		return data->toString();
	}

	const ETreeData *data;
};

inline ETree operator + (const ETree &a, const ETree &b)
{
	return ETree(ETreeOpType::add, a, b);
}

inline ETree operator * (const ETree &a, const ETree &b)
{
	return ETree(ETreeOpType::multiply, a, b);
}

inline ETree operator - (const ETree &a, const ETree &b)
{
	return ETree(ETreeOpType::subtract, a, b);
}

inline ETree operator / (const ETree &a, const ETree &b)
{
	return ETree(ETreeOpType::divide, a, b);
}

inline ETree operator - (const ETree &a)
{
	return ETree(ETreeOpType::negate, a);
}

inline ETree sin(const ETree &a)
{
	return ETree(ETreeOpType::sin, a);
}

inline ETree cos(const ETree &a)
{
	return ETree(ETreeOpType::cos, a);
}

inline ETree tan(const ETree &a)
{
	return ETree(ETreeOpType::tan, a);
}

inline ETree sqrt(const ETree &a)
{
	return ETree(ETreeOpType::sqrt, a);
}

inline ETree pow(const ETree &a, const ETree &b)
{
	return ETree(ETreeOpType::pow, a, b);
}