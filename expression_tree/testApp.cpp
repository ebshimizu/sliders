
#include "main.h"

template<class T>
T f0(T v0, T v1)
{
	return (v0 + v1) * T(0.5);
}

template<class T>
T f1(T v0, T v1)
{
	T v2 = v0 * T(0.1) * v1 / -T(0.5);
	T v3 = cos(sin(-v0 + v1 * v1));
	T v4 = sqrt(v3 * v3) + T(1.0);
	T v5 = pow(v4, v4);
	return v5;
}

void TestApp::go()
{
	testFunction2(function<ETree(ETree, ETree)>(f0<ETree>), function<double(double, double)>(f0<double>));
	testFunction2(function<ETree(ETree, ETree)>(f1<ETree>), function<double(double, double)>(f1<double>));
}

void TestApp::testFunction2(function<ETree(ETree, ETree)>& funcE, function<double(double, double)>& funcD)
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
