#pragma once

class TestApp
{
public:
	void go();
	
	//void testFunction2(function<ETree(ETree, ETree)> &funcE, function<double(double, double)> &funcD);
	void testFunction2(function<ExpStep(ExpStep, ExpStep)> &funcE, function<double(double, double)> &funcD, const string &functionName);

	void testOptimizer();
};