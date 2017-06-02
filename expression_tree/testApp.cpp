
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
		ExpStep x1E = context.registerParam(0, "x1", x1D);

		//ExpStep vE = x0E + x1E;

		ExpStep vE = funcE(x0E, x1E);
		context.registerResult(vE, 0);
		//ExpStep vEOut2 = ExpStep(x0E, 1);
		//ExpStep vEOut3 = ExpStep(x1E, 2);

		if (testIndex == 0)
		{
			vector<string> sourceCode = context.toSourceCode(functionName, false);
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
