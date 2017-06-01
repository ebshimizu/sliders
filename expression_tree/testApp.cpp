
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

void TestApp::go()
{
	//testFunction2(function<ETree(ETree, ETree)>(f0<ETree>), function<double(double, double)>(f0<double>));
	//testFunction2(function<ETree(ETree, ETree)>(f1<ETree>), function<double(double, double)>(f1<double>));

	testFunction2(function<ExpStep(ExpStep, ExpStep)>(f0<ExpStep>), function<double(double, double)>(f0<double>), "f0");
	testFunction2(function<ExpStep(ExpStep, ExpStep)>(f1<ExpStep>), function<double(double, double)>(f1<double>), "f1");
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

		ExpStep x0E = ExpStep(context, x0D, "x0", 0);
		ExpStep x1E = ExpStep(context, x1D, "x1", 1);

		//ExpStep vE = x0E + x1E;

		ExpStep vE = funcE(x0E, x1E);
		ExpStep vEOut = ExpStep(vE, 0);
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

/*void TestApp::testFunction2(function<ETree(ETree, ETree)>& funcE, function<double(double, double)>& funcD)
{
	cout << "testing function2" << endl;
	for (int testIndex = 0; testIndex < 5; testIndex++)
	{
		double x0D = rand() % 100 - 50.0;
		double x1D = rand() % 100 - 50.0;

		double vD = funcD(x0D, x1D);

		ETree x0E = ETree(x0D, "x0", 0);
		ETree x1E = ETree(x1D, "x1", 1);

		ETree vE = funcE(x0E, x1E);
		double vEEval = vE.eval();

		if (testIndex == 0)
		{
			cout << vE.toString() << endl;
		}

		cout << "delta " << testIndex << " = " << vD - vEEval << endl;
	}
}
*/