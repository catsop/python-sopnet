#include <iostream>
#include <LinearSolverBackend.h>
#include <DefaultFactory.h>

int main(int, char**) {

	LinearSolverBackend* solver = 0;

	try {

		DefaultFactory solverFactory;
		solver = solverFactory.createLinearSolverBackend();

	} catch (std::exception& e) {

		std::cerr << "error: " << e.what() << std::endl;
	}

	if (solver)
		delete solver;
}
