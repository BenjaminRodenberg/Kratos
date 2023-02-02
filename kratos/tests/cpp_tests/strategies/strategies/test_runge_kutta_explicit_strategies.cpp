//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Ruben Zorrilla, Eduard Gómez
//

// System includes

/* External includes */

/* Project includes */
#include "tests/cpp_tests/strategies/strategies/explicit_strategy_testing_utils.h"
#include "solving_strategies/strategies/explicit_solving_strategy_runge_kutta.h"

namespace Kratos
{
namespace Testing
{
namespace RungeKuttaTestInternals
{
using namespace ExplicitStrategyTestingUtils;

using ExplicitSolvingStrategyRungeKutta1Type = ExplicitSolvingStrategyRungeKutta1<SparseSpaceType, LocalSpaceType>;
using ExplicitSolvingStrategyRungeKutta2Type = ExplicitSolvingStrategyRungeKutta2<SparseSpaceType, LocalSpaceType>;
using ExplicitSolvingStrategyRungeKutta3Type = ExplicitSolvingStrategyRungeKutta3TVD<SparseSpaceType, LocalSpaceType>;
using ExplicitSolvingStrategyRungeKutta4Type = ExplicitSolvingStrategyRungeKutta4<SparseSpaceType, LocalSpaceType>;

} // namespace RungeKuttaTestInternals

/**
 * Checks if the Linear strategy performs correctly the resolution of the system
 */
TEST(ExplicitSolvingStrategyRungeKutta1, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    RunTest<ExplicitSolvingStrategyRungeKutta1Type>(1e-1);
}

TEST(ExplicitSolvingStrategyRungeKutta2, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    RunTest<ExplicitSolvingStrategyRungeKutta2Type>(1e-2);
}

TEST(ExplicitSolvingStrategyRungeKutta3, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    RunTest<ExplicitSolvingStrategyRungeKutta3Type>(1e-5);
}

TEST(ExplicitSolvingStrategyRungeKutta4, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    RunTest<ExplicitSolvingStrategyRungeKutta4Type>(1e-7);
}


TEST(ExplicitSolvingStrategyRungeKutta1Convergence, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    ConvergenceTest<ExplicitSolvingStrategyRungeKutta1Type>(1);
}

TEST(ExplicitSolvingStrategyRungeKutta2Convergence, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    ConvergenceTest<ExplicitSolvingStrategyRungeKutta2Type>(2);
}

TEST(ExplicitSolvingStrategyRungeKutta3Convergence, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    ConvergenceTest<ExplicitSolvingStrategyRungeKutta3Type>(3);
}

TEST(ExplicitSolvingStrategyRungeKutta4Convergence, KratosCoreFastSuite)
{
    using namespace RungeKuttaTestInternals;
    ConvergenceTest<ExplicitSolvingStrategyRungeKutta4Type>(4);
}


} // namespace Testing
} // namespace Kratos
