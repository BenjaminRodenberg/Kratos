// KRATOS    ______            __             __  _____ __                  __                   __
//          / ____/___  ____  / /_____ ______/ /_/ ___// /________  _______/ /___  ___________ _/ /
//         / /   / __ \/ __ \/ __/ __ `/ ___/ __/\__ \/ __/ ___/ / / / ___/ __/ / / / ___/ __ `/ / 
//        / /___/ /_/ / / / / /_/ /_/ / /__/ /_ ___/ / /_/ /  / /_/ / /__/ /_/ /_/ / /  / /_/ / /  
//        \____/\____/_/ /_/\__/\__,_/\___/\__//____/\__/_/   \__,_/\___/\__/\__,_/_/   \__,_/_/  MECHANICS
//
//  License:         BSD License
//                   license: ContactStructuralMechanicsApplication/license.txt
//
//  Main authors:  Vicente Mataix Ferrandiz
//

// System includes

// External includes

// Project includes
#include "custom_frictional_laws/frictional_law_with_derivative.h"

namespace Kratos
{
template< std::size_t TDim, std::size_t TNumNodes, bool TNormalVariation, std::size_t TNumNodesMaster>
double FrictionalLawWithDerivative<TDim,TNumNodes,TNormalVariation, TNumNodesMaster>::GetDerivativeThresholdValue(
    const NodeType& rNode,
    const PairedCondition& rCondition,
    const ProcessInfo& rCurrentProcessInfo,
    const DerivativeDataType& rDerivativeData,
    const MortarConditionMatrices& rMortarConditionMatrices,
    const IndexType IndexDerivative,
    const IndexType IndexNode
    )
{
    KRATOS_ERROR << "You are calling to the base class method GetDerivativeThresholdValue, check your frictional law declaration" << std::endl;

    return 0.0;
}

/***********************************************************************************/
/***********************************************************************************/

template class FrictionalLawWithDerivative<2, 2, false, 2>;
template class FrictionalLawWithDerivative<3, 3, false, 3>;
template class FrictionalLawWithDerivative<3, 4, false, 4>;
template class FrictionalLawWithDerivative<3, 3, false, 4>;
template class FrictionalLawWithDerivative<3, 4, false, 3>;
template class FrictionalLawWithDerivative<2, 2, true,  2>;
template class FrictionalLawWithDerivative<3, 3, true,  3>;
template class FrictionalLawWithDerivative<3, 4, true,  4>;
template class FrictionalLawWithDerivative<3, 3, true,  4>;
template class FrictionalLawWithDerivative<3, 4, true,  3>;

}  // namespace Kratos.

