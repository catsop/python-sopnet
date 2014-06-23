#include <iostream>
#include <LinearSolverBackend.h>
#include <DefaultFactory.h>

#ifdef HAVE_GUROBI
#include "GurobiBackend.h"
#endif

int main(int, char**) {

	LinearSolverBackend* solver = 0;

	try {

		DefaultFactory solverFactory;
		solver = solverFactory.createLinearSolverBackend();

#ifdef HAVE_GUROBI
	} catch (GRBException& e) {

		std::cerr << "error: " << e.getMessage() << std::endl;
#endif

	} catch (std::exception& e) {

		std::cerr << "error: " << e.what() << std::endl;
	}

	if (solver)
		delete solver;
}
