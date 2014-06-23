#include <iostream>
#include <LinearSolverBackend.h>
#include <DefaultFactory.h>

int main(int, char**) {

	DefaultFactory solverFactory;
	LinearSolverBackend* solver = solverFactory.createLinearSolverBackend();
	delete solver;
}
