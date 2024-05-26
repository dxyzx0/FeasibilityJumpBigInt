/*
 * XPress integration for the Feasibility Jump heuristic.
 */

extern "C"
//{
//#include "xprs.h"
//}

#include <cstdio>
#include <string>
#include <chrono>
#include <vector>
#include <cassert>
#include <mutex>
#include <thread>
#include <functional>
#include <atomic>
#include <cmath>
#include <climits>
#include <chrono>
#include "parser/AbcCallback.h"
#include "parser/SimpleParser.h"
#include "feasibilityjump.hh"
#include "type.h"
#include <iostream>

using namespace std;
const int NUM_THREADS = 1;

std::atomic_size_t totalNumSolutionsFound(0);
std::atomic_size_t totalNumSolutionsAdded(0);
std::atomic_bool presolveFinished(false);
std::atomic_bool heuristicFinished(false);
std::chrono::steady_clock::time_point startTime;

struct Solution
{
	std::vector< IntegerType > assignment;
	bool includesContinuous;
};

std::vector< Solution > heuristicSolutions;
std::mutex heuristicSolutions_mutex;

std::mutex presolvedProblem_mutex;
std::mutex nonPresolvedProblem_mutex;

struct ProblemInstance
{
	int numCols;
	std::vector< char > varTypes;
	std::vector< IntegerType > lb;
	std::vector< IntegerType > ub;
	std::vector< IntegerType > objCoeffs;

	int numRows;
	int numNonZeros;
	std::vector< char > rowtypes;
	std::vector< IntegerType > rhs;
	//std::vector<IntegerType> rhsrange;
	std::vector< long > rowStart;
	std::vector< long > colIdxs;
	std::vector< IntegerType > colCoeffs;
	std::vector< tuple< long, string>> rowRelOp;
};

struct FJData
{
	std::vector< int > originalIntegerCols;
//    XPRSprob originalProblemCopy = nullptr;
//    XPRSprob presolvedProblemCopy = nullptr;
//    AbcCallback& originalProblemCopy;
//    AbcCallback& presolvedProblemCopy;
	ProblemInstance originalData;
	ProblemInstance presolvedData;
};

FJData gFJData;

// A function that receives the result of any solution added with XPRSaddmipsol.
//void XPRS_CC userSolNotify(XPRSprob problem, void *_data, const char *solName, int status)
//{
//    if (status == 0)
//        printf(FJ_LOG_PREFIX "XPress received solution: An error occurred while processing the solution.\n");
//    if (status == 1)
//        printf(FJ_LOG_PREFIX "XPress received solution: Solution is feasible.\n");
//    if (status == 2)
//        printf(FJ_LOG_PREFIX "XPress received solution: Solution is feasible after reoptimizing with fixed globals.\n");
//    if (status == 3)
//        printf(FJ_LOG_PREFIX "XPress received solution: A local search heuristic was applied and a feasible solution discovered.\n");
//    if (status == 4)
//        printf(FJ_LOG_PREFIX "XPress received solution: A local search heuristic was applied but a feasible solution was not found.\n");
//    if (status == 5)
//        printf(FJ_LOG_PREFIX "XPress received solution: Solution is infeasible and a local search could not be applied.\n");
//    if (status == 6)
//        printf(FJ_LOG_PREFIX "XPress received solution: Solution is partial and a local search could not be applied.\n");
//    if (status == 7)
//        printf(FJ_LOG_PREFIX "XPress received solution: Failed to reoptimize the problem with globals fixed to the provided solution. Likely because a time or iteration limit was reached.\n");
//    if (status == 8)
//        printf(FJ_LOG_PREFIX "XPress received solution: Solution is dropped. This can happen if the MIP problem is changed or solved to completion before the solution could be processed.\n");
//}

std::string inputFilename;
std::string outDir;
//
//void XPRS_CC intsol(XPRSprob problem, void *data)
//{
//    double time = std::chrono::duration_cast<std::chrono::milliseconds>(
//                      std::chrono::steady_clock::now() - startTime)
//                      .count() /
//                  1000.0;
//
//    double objVal;
//    XPRSgetdblattrib(problem, XPRS_MIPOBJVAL, &objVal);
//    printf("XPRESS %g %g\n", time, objVal);
//
//    if (outDir.size() > 0)
//    {
//        char path[512];
//        sprintf(path, "%s/%.2f_%g_%s.sol", outDir.c_str(), time, objVal, inputFilename.c_str());
//
//        printf("Writing to filename '%s'\n", path);
//        XPRSwriteslxsol(problem, path, "");
//    }
//}
//
//int XPRS_CC checktime(XPRSprob problem, void *_data)
//{
//    if (presolveFinished && totalNumSolutionsFound != totalNumSolutionsAdded)
//    {
//        std::lock_guard<std::mutex> guard(heuristicSolutions_mutex);
//
//        for (auto &sol : heuristicSolutions)
//        {
//
//            std::vector<double> values;
//            values.reserve(gFJData.originalIntegerCols.size());
//            for (auto &idx : gFJData.originalIntegerCols)
//                values.push_back(sol.assignment[idx]);
//            assert(values.size() == gFJData.originalIntegerCols.size());
//
//            // Add only the integer values.
//            XPRSaddmipsol(problem, values.size(), values.data(),
//                          gFJData.originalIntegerCols.data(), nullptr);
//
//            // We could also have added all values and let the solver handle it, like this:
//            //   auto data = new std::vector<double>(sol.assignment);
//            //   XPRSaddmipsol(problem, data->size(), data->data(), nullptr, nullptr);
//        }
//        totalNumSolutionsAdded += heuristicSolutions.size();
//        heuristicSolutions.clear();
//    }
//    if (heuristicFinished)
//    {
//        printf(FJ_LOG_PREFIX "all threads terminated. Removing MIP solver callback.\n");
//        XPRSremovecbchecktime(problem, nullptr, nullptr);
//    }
//
//    return 0;
//}
//
//void XPRS_CC presolve_callback(XPRSprob problem, void *_data)
//{
//    presolveFinished = true;
//    checktime(problem, nullptr);
//}

ProblemInstance getXPRSProblemData(AbcCallback& abcCallback)//XPRSprob problem,
{
	// TODO: CHANGE IT
	ProblemInstance data;
	//XPRSgetintattrib(problem, XPRS_COLS, &data.numCols);
	data.numCols = abcCallback.getNVar();

	//data.varTypes = std::vector<char>(data.numCols);
	//XPRSgetcoltype(problem, data.varTypes.data(), 0, data.numCols - 1);
	data.varTypes = std::vector< char >(data.numCols);
	std::fill(data.varTypes.begin(), data.varTypes.end(), 'B');

	data.lb = std::vector< IntegerType >(data.numCols, 0);
	data.ub = std::vector< IntegerType >(data.numCols, 1);
	//data.objCoeffs = std::vector<double>(data.numCols);
	//XPRSgetobj(problem, data.objCoeffs.data(), 0, data.numCols - 1);
	data.objCoeffs = std::vector< IntegerType >(data.numCols);

	for (int i = 0; i < abcCallback.getC().size(); i++)
	{
		auto idx = std::get< 0 >(abcCallback.getC()[i]);
		auto val = std::get< 1 >(abcCallback.getC()[i]);
		data.objCoeffs[idx] = val;
	}
	//XPRSgetintattrib(problem, XPRS_ROWS, &data.numRows);
	data.numRows = abcCallback.getNCons();

	//data.rowtypes = std::vector<char>(data.numRows);
	//XPRSgetrowtype(problem, data.rowtypes.data(), 0, data.numRows - 1);
	data.rowtypes = std::vector< char >(data.numRows);
	data.rowRelOp = abcCallback.getRelOp();
	for (int i = 0; i < data.numRows; i++)
	{
		auto idx = std::get< 0 >(data.rowRelOp[i]);
		assert(idx == i);
		std::string relOp = std::get< 1 >(data.rowRelOp[i]);
		if (relOp == "=") data.rowtypes[i] = 'E';
		else if (relOp == ">=") data.rowtypes[i] = 'G';
		else
		{ throw std::runtime_error("Unsupported relation operator"); }
	}

	//data.rhs = std::vector<double>(data.numRows);
	//XPRSgetrhs(problem, data.rhs.data(), 0, data.numRows - 1);
	data.rhs = std::vector< IntegerType >(data.numRows);
	for (int i = 0; i < data.numRows; i++)
	{
		auto idx = std::get< 0 >(abcCallback.getB()[i]);
		assert(idx == i);
		data.rhs[i] = std::get< 1 >(abcCallback.getB()[i]);
	}

	//data.rhsrange = std::vector<double>(data.numRows);
	//XPRSgetrhsrange(problem, data.rhsrange.data(), 0, data.numRows - 1);

	//XPRSgetrows(problem, nullptr, nullptr, nullptr, 0, &data.numNonZeros, 0, data.numRows - 1);
	data.numNonZeros = abcCallback.getA().size();
	printf(FJ_LOG_PREFIX "copying %d x %d matrix with %d nonzeros.\n",
		data.numCols, data.numRows, data.numNonZeros);

	data.rowStart = std::vector< long >(data.numRows + 1);
	data.colIdxs = std::vector< long >(data.numNonZeros);
	data.colCoeffs = std::vector< IntegerType >(data.numNonZeros);
	data.rowStart[0] = 0;

	// add assert to make sure abcCallback.getA() is sorted by row


	int i = 0;
	long row = 0;
	for (const auto& t : abcCallback.getA())
	{
		data.colIdxs[i] = get< 1 >(t);
		data.colCoeffs[i] = get< 2 >(t);
		auto row_i = get< 0 >(t);
		if (row_i != row)
		{
			data.rowStart[row + 1] = i;
			row = row_i;
		}
		i++;
	}
	data.rowStart[data.numRows] = data.numNonZeros;
//    for(int j = 0; j < data.numRows; j++)
//    {
//        data.rowStart[j + 1] = data.rowStart[j] + std::count_if(data.colIdxs.begin(), data.colIdxs.end(), [j](int x) { return x == j; });
//    }
	cout << "*****************\n" << endl;
	cout << "numCols= \n" << data.numCols << endl;
	cout << "numRows= \n" << data.numRows << endl;
	cout << "numNonZeros= \n" << data.numNonZeros << endl;
	cout << " varTypes: " << endl;
	for (const auto& t : data.varTypes)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " lb: " << endl;
	for (const auto& t : data.lb)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " ub: " << endl;
	for (const auto& t : data.ub)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " objCoeffs: " << endl;
	for (const auto& t : data.objCoeffs)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " rowtypes: " << endl;
	for (const auto& t : data.rowtypes)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " rhs:" << endl;
	for (const auto& t : data.rhs)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " rowStart:" << endl;
	for (const auto& t : data.rowStart)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " colIdxs:" << endl;
	for (const auto& t : data.colIdxs)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;
	cout << " colCoeffs:" << endl;
	for (const auto& t : data.colCoeffs)
	{
		std::cout << t << " ";
	}
	std::cout << std::endl;

	cout << "*****************\n" << endl;


	//XPRSgetrows(problem,
	//            data.rowStart.data(),
	//            data.colIdxs.data(),
	//            data.colCoeffs.data(),
	//            data.numNonZeros,
	//            &data.numNonZeros,
	//            0,
	//            data.numRows - 1);

	return data;
}

bool copyDataToHeuristicSolver(FeasibilityJumpSolver& solver, ProblemInstance& data, int relaxContinuous)
{
	printf("initializing FJ with %d vars %d constraints\n", data.numCols, data.numRows);
	for (long colIdx = 0; colIdx < data.numCols; colIdx += 1)
	{
		VarType vartype = VarType::Continuous;
		if (data.varTypes[colIdx] == 'B')
		{
			vartype = VarType::Integer;
		}
		else
		{
			printf(FJ_LOG_PREFIX "unsupported variable type '%c' (%d).\n",
				data.varTypes[colIdx], data.varTypes[colIdx]);
			return false;
		}

		solver.addVar(vartype, data.lb[colIdx], data.ub[colIdx], data.objCoeffs[colIdx]);
	}

	for (long rowIdx = 0; rowIdx < data.numRows; rowIdx += 1)
	{
		assert(data.rowtypes[rowIdx] == 'G' || data.rowtypes[rowIdx] == 'E');
		RowType rowtype;
		if (data.rowtypes[rowIdx] == 'G')
		{
			rowtype = RowType::Gte;
		}
		else if (data.rowtypes[rowIdx] == 'E')
		{
			rowtype = RowType::Equal;
		}
		else
		{
			printf(FJ_LOG_PREFIX "unsupported constraint type '%c'. Ignoring constraint.\n", data.rowtypes[rowIdx]);
			return false;
		}

		solver.addConstraint(rowtype,
			data.rhs[rowIdx],
			data.rowStart[rowIdx + 1] - data.rowStart[rowIdx],
			&data.colIdxs[data.rowStart[rowIdx]],
			&data.colCoeffs[data.rowStart[rowIdx]],
			relaxContinuous);
	}

	return true;
}

// An object containing a function to be executed when the object is destructed.
struct Defer
{
	std::function< void(void) > func;

	Defer(std::function< void(void) > pFunc) : func(pFunc)
	{
	};

	~Defer()
	{
		func();
	}
};

void mapHeuristicSolution(FJStatus& status)
{

	Solution s;
	bool conversionOk = false;
	{
		printf(FJ_LOG_PREFIX "received a solution from non-presolved instance.\n");
		s.assignment = std::vector< IntegerType >(status.solution, status.solution + status.numVars);
		s.includesContinuous = true;
		conversionOk = true;
	}

	if (conversionOk)
	{
		{
			std::lock_guard< std::mutex > guard(heuristicSolutions_mutex);
			heuristicSolutions.push_back(s);
			totalNumSolutionsFound += 1;
		}
	}
}

const int maxEffort = 100000000;

// Starts background threads running the Feasibility Jump heuristic.
// Also installs the check-time callback to report any feasible solutions
// back to the MIP solver.
void start_feasibility_jump_heuristic(AbcCallback& abcCallback, size_t maxTotalSolutions, bool heuristicOnly,
	bool relaxContinuous = false, bool exponentialDecay = false, int verbose = 0)
{

	// Copy the problem to the heuristic.
//    XPRScreateprob(&gFJData.originalProblemCopy);
//    XPRScopyprob(gFJData.originalProblemCopy, problem, "");

	{
		auto allThreadsTerminated = std::make_shared< Defer >([]()
		{ heuristicFinished = true; });

		for (int thread_idx = 0; thread_idx < NUM_THREADS; thread_idx += 1)
		{
			auto seed = thread_idx;
			bool usePresolved = thread_idx % 2 == 1;
			double decayFactor = (!exponentialDecay) ? 1.0 : 0.9999;
			auto f = [verbose, maxTotalSolutions, usePresolved, seed,
				relaxContinuous, decayFactor, allThreadsTerminated, &abcCallback]()
			{
			  // Prepare data for the non-presolved version.
			  {
				  std::lock_guard< std::mutex > guard(nonPresolvedProblem_mutex);
				  if (gFJData.originalData.numCols == 0)
				  {
					  gFJData.originalData = getXPRSProblemData(abcCallback);
				  }
			  }

			  // Produce the presolved solution
//                    if (usePresolved)
//                    {
//                        std::lock_guard<std::mutex> guard(presolvedProblem_mutex);
//                        if (gFJData.presolvedProblemCopy == nullptr)
//                        {
//                            XPRScreateprob(&gFJData.presolvedProblemCopy);
//                            XPRSsetlogfile(gFJData.presolvedProblemCopy, "presolve.log");
//                            XPRScopyprob(gFJData.presolvedProblemCopy, gFJData.originalProblemCopy, "");
//                            XPRSsetintcontrol(gFJData.presolvedProblemCopy, XPRS_LPITERLIMIT, 0);
//
//                            XPRSmipoptimize(gFJData.presolvedProblemCopy, "");
//                            gFJData.presolvedData = getXPRSProblemData(gFJData.presolvedProblemCopy);
//                        }
//                    }

			  ProblemInstance& data = gFJData.originalData;
			  FeasibilityJumpSolver solver(seed, verbose, decayFactor);
			  bool copyOk = copyDataToHeuristicSolver(solver, data, relaxContinuous);
			  if (!copyOk)
			  {
				  cout << "error here copyOk!" << endl;
				  return;
			  }

			  solver.solve(
				  nullptr, [maxTotalSolutions, usePresolved](FJStatus status) -> CallbackControlFlow
				  {

					double time = std::chrono::duration_cast< std::chrono::milliseconds >(
						std::chrono::steady_clock::now() - startTime).count() / 1000.0;

					// If we received a solution, put it on the queue.
					if (status.solution != nullptr)
					{
						char* solutionObjectiveValueStr =
							mpz_get_str(nullptr, 10, status.solutionObjectiveValue.get_mpz_t());
//
						printf("FJSOL %g %s\n", time, solutionObjectiveValueStr);
						free(solutionObjectiveValueStr);
						mapHeuristicSolution(status);
					}

					// If we have enough solutions or spent enough time, quit.
					auto quitNumSol = totalNumSolutionsFound >= maxTotalSolutions;
					if (quitNumSol)
						printf(FJ_LOG_PREFIX "quitting because number of solutions %zd >= %zd.\n",
							totalNumSolutionsFound.load(), maxTotalSolutions);
					auto quitEffort = status.effortSinceLastImprovement > maxEffort;
					if (quitEffort)
						printf(FJ_LOG_PREFIX "quitting because effort %d > %d.\n",
							status.effortSinceLastImprovement, maxEffort);

					auto quit = quitNumSol || quitEffort || heuristicFinished;
					if (quit)
						printf(FJ_LOG_PREFIX "effort rate: %g Mops/sec\n", status.totalEffort / time / 1.0e6);
					return quit ? CallbackControlFlow::Terminate : CallbackControlFlow::Continue;
				  });
			};
			f();
//            std::thread(
//                [verbose, maxTotalSolutions, usePresolved, seed,
//                 relaxContinuous, decayFactor, allThreadsTerminated, &abcCallback]()
//                {
//                    // Prepare data for the non-presolved version.
//                    {
//                        std::lock_guard<std::mutex> guard(nonPresolvedProblem_mutex);
//                        if (gFJData.originalData.numCols == 0)
//                        {
//                            gFJData.originalData = getXPRSProblemData(abcCallback);                        }
//                    }
//
//                    // Produce the presolved solution
////                    if (usePresolved)
////                    {
////                        std::lock_guard<std::mutex> guard(presolvedProblem_mutex);
////                        if (gFJData.presolvedProblemCopy == nullptr)
////                        {
////                            XPRScreateprob(&gFJData.presolvedProblemCopy);
////                            XPRSsetlogfile(gFJData.presolvedProblemCopy, "presolve.log");
////                            XPRScopyprob(gFJData.presolvedProblemCopy, gFJData.originalProblemCopy, "");
////                            XPRSsetintcontrol(gFJData.presolvedProblemCopy, XPRS_LPITERLIMIT, 0);
////
////                            XPRSmipoptimize(gFJData.presolvedProblemCopy, "");
////                            gFJData.presolvedData = getXPRSProblemData(gFJData.presolvedProblemCopy);
////                        }
////                    }
//
//                    ProblemInstance &data = usePresolved ? gFJData.presolvedData : gFJData.originalData;
//                    FeasibilityJumpSolver solver(seed, verbose, decayFactor);
//                    bool copyOk = copyDataToHeuristicSolver(solver, data, relaxContinuous);
//                    if (!copyOk)
//                        return;
//
//                    solver.solve(
//                        nullptr, [maxTotalSolutions, usePresolved](FJStatus status) -> CallbackControlFlow
//                        {
//
//
//                            double time = std::chrono::duration_cast<std::chrono::milliseconds>(
//                                std::chrono::steady_clock::now() - startTime).count() /1000.0;
//
//                            // If we received a solution, put it on the queue.
//                            if (status.solution != nullptr)
//                            {
//                                char *solutionObjectiveValueStr = mpz_get_str(NULL, 10, status.solutionObjectiveValue.get_mpz_t());
//                                printf("FJSOL %g %s\n", time, solutionObjectiveValueStr);
//                                free(solutionObjectiveValueStr);
//                                //mapHeuristicSolution(status, usePresolved);
//                            }
//
//                            // If we have enough solutions or spent enough time, quit.
//                            auto quitNumSol = totalNumSolutionsFound >= maxTotalSolutions;
//                            if(quitNumSol) printf(FJ_LOG_PREFIX "quitting because number of solutions %zd >= %zd.\n", totalNumSolutionsFound.load(), maxTotalSolutions);
//                            auto quitEffort = status.effortSinceLastImprovement > maxEffort;
//                            if(quitEffort) printf(FJ_LOG_PREFIX "quitting because effort %d > %d.\n", status.effortSinceLastImprovement , maxEffort);
//
//                            auto quit = quitNumSol || quitEffort || heuristicFinished;
//                            if (quit)
//                                printf(FJ_LOG_PREFIX "effort rate: %g Mops/sec\n", status.totalEffort / time / 1.0e6);
//                            return quit ? CallbackControlFlow::Terminate : CallbackControlFlow::Continue; });
//                })
//                .detach();
		}
	}

	if (heuristicOnly)
	{
		while (!heuristicFinished)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		printf(FJ_LOG_PREFIX "all threads exited.\n");
	}
}

#define CHECK_RETURN(call)                                  \
    do                                                      \
    {                                                       \
        int result_ = call;                                 \
        if (result_ != 0)                                   \
        {                                                   \
            fprintf(stderr, "Line %d: %s failed with %d\n", \
                    __LINE__, #call, result_);              \
            returnCode = result_;                           \
            goto cleanup;                                   \
        }                                                   \
    } while (0)

int printUsage()
{
	printf(
		"Usage: xpress_fj [--timeout|-t TIMEOUT] [--save-solutions|-s OUTDIR] [--verbose|-v] [--heuristic-only|-h] [--exponential-decay|-e] [--relax-continuous|-r] INFILE\n");
	return 1;
}

int main(int argc, char* argv[])
{
	int verbose = 0;
	bool heuristicOnly = false;
	bool relaxContinuous = false;
	bool exponentialDecay = false;
	int timeout = INT32_MAX / 2;

	std::string inputPath;
	for (int i = 1; i < argc; i += 1)
	{
		std::string argvi(argv[i]);
		if (argvi == "--save-solutions" || argvi == "-s")
		{
			if (i + 1 < argc)
				outDir = std::string(argv[i + 1]);
			else
				return printUsage();
			i += 1;
		}
		else if (argvi == "--timeout" || argvi == "-t")
		{
			if (i + 1 < argc)
				timeout = std::stoi(argv[i + 1]);
			else
				return printUsage();
			i += 1;
		}
		else if (argvi == "--verbose" || argvi == "-v")
			verbose += 1;
		else if (argvi == "--heuristic-only" || argvi == "-h")
			heuristicOnly = true;
//        else if (argvi == "--relax-continuous" || argvi == "-r")
//            relaxContinuous = true;
//        else if (argvi == "--exponential-decay" || argvi == "-e")
//            exponentialDecay = true;
		else if (!inputPath.empty())
			return printUsage();
		else
			inputPath = argvi;
	}

	if (inputPath.empty())
		return printUsage();

	inputFilename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

	int returnCode = 0;

	startTime = std::chrono::steady_clock::now();

	SimpleParser< AbcCallback >* parser = nullptr;
	try
	{
		string filename = inputPath;
		// assert filename ends with ".opb" or ".pb"
		cout << "Parsing file: " << filename << endl;
		assert(filename.substr(filename.find_last_of('.') + 1) == "opb" ||
			filename.substr(filename.find_last_of('.') + 1) == "pb");
		parser = new SimpleParser< AbcCallback >(filename.c_str());

		parser->setAutoLinearize(true);
		parser->parse();
	}
	catch (exception& e)
	{
		cout.flush();
		cerr << "ERROR: " << e.what() << endl;
	}

	assert(parser != nullptr);

//	Solver solver(make_shared< Problem >(parser->cb), make_shared< Settings >());
//	SolveResult result = solver.solve();



	start_feasibility_jump_heuristic(parser->cb, 1, heuristicOnly, relaxContinuous, exponentialDecay, verbose);

//    if (!heuristicOnly)
//    {
//
//        int numColsAll;
//        int numColsOrig;
//        //XPRSgetintattrib(problem, XPRS_COLS, &numColsAll);
//        //XPRSgetintattrib(problem, XPRS_ORIGINALCOLS, &numColsOrig);
//        assert(numColsAll == numColsOrig);
//
//        // Prepare the list of integer variables to be used in
//        // `checktime` for adding mip solutions.
//        auto varTypes = std::vector<char>(numColsAll);
//        XPRSgetcoltype(problem, varTypes.data(), 0, numColsAll - 1);
//        for (int i = 0; i < numColsAll; i += 1)
//            if (varTypes[i] != 'C')
//                gFJData.originalIntegerCols.push_back(i);
//
//        // An error in XPress causes solutions to be rejected sometimes
//        // if they are added using `addmipsol` while presolve is running.
//        // So, we need a callback to detect that presolve has finished.
//        // We wait until this has happened before adding any heuristic solutions
//        // to XPress.
//        CHECK_RETURN(XPRSaddcbpresolve(problem, presolve_callback, nullptr, 0));
//
//        // Install a callback that converts the heuristic solutions
//        // into XPRSaddmipsol calls...
//        CHECK_RETURN(XPRSaddcbchecktime(problem, checktime, nullptr, 0));
//        // ...and then solve normally.
//        XPRSsetintcontrol(problem, XPRS_THREADS, 1);
//
//        double time = std::chrono::duration_cast<std::chrono::milliseconds>(
//                          std::chrono::steady_clock::now() - startTime)
//                          .count() /
//                      1000.0;
//
//        int xpress_timeout = std::ceil(timeout - time);
//        XPRSsetintcontrol(problem, XPRS_MAXTIME, -xpress_timeout);
//        CHECK_RETURN(XPRSmipoptimize(problem, ""));
//
//        double ub, lb;
//        CHECK_RETURN(XPRSgetdblattrib(problem, XPRS_MIPBESTOBJVAL, &ub));
//        CHECK_RETURN(XPRSgetdblattrib(problem, XPRS_BESTBOUND, &lb));
//
//        time = std::chrono::duration_cast<std::chrono::milliseconds>(
//                   std::chrono::steady_clock::now() - startTime)
//                   .count() /
//               1000.0;
//
//        printf("{\"exit_time\":%g, \"lb\": %g, \"ub\": %g}\n", time, lb, ub);
//
//        heuristicFinished = true;
//
//        int status;
//        XPRSgetintattrib(problem, XPRS_MIPSTATUS, &status);
//        switch (status)
//        {
//        case XPRS_MIP_OPTIMAL:
//            printf("XPRESS Solved, optimal.\n");
//
//            break;
//        default:
//            printf(FJ_LOG_PREFIX "Unexpected solution status (%i).\n", status);
//        }
//    }

cleanup:
	if (returnCode > 0)
	{
		/* There was an error with the solver. Get the error code and error message.
		 * If prob is still NULL then the error was in XPRScreateprob() and
		 * we cannot find more detailed error information.
		 */
//        if (problem != NULL)
//        {
//            int errorCode = -1;
//            char errorMessage[512] = {0};
//            XPRSgetintattrib(problem, XPRS_ERRORCODE, &errorCode);
//            XPRSgetlasterror(problem, errorMessage);
//            fprintf(stderr, "Error %d: %s\n", errorCode, errorMessage);
//        }
	}

	// Normally, we would clean up XPress at this point, if it should be used
	// as part of a larger program.  However, for this benchmarking program, we
	// avoid having to deal with waiting for the threads to shut down if we
	// just skip de-allocating the global Xpress structs.  If we do XPRSfree
	// here, we risk that a thread running the FJ heuristic will run some
	// XPress function between the XPRSfree call and the program shutting down,
	// which is a use-after-free error.
	//
	// XPRSdestroyprob(problem);
	// XPRSfree();

	return returnCode;
}
