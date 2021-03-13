//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Suneth Warnakulasuriya
//

#if !defined(KRATOS_RANS_QS_VMS_DERIVATIVE_UTILITIES_H)
#define KRATOS_RANS_QS_VMS_DERIVATIVE_UTILITIES_H

// System includes

// External includes

// Project includes
#include "containers/variable.h"
#include "geometries/geometry.h"
#include "includes/node.h"
#include "includes/process_info.h"
#include "includes/ublas_interface.h"
#include "utilities/time_discretization.h"

// Application includes
#include "custom_elements/data_containers/qs_vms/qs_vms_derivative_utilities.h"
#include "custom_utilities/fluid_adjoint_variable_information.h"

namespace Kratos
{
///@name Kratos Classes
///@{

template <unsigned int TDim>
class RansQSVMSDerivativeUtilities
{
public:
    ///@name Type Definitions
    ///@{

    using NodeType = Node<3>;

    using GeometryType = Geometry<NodeType>;

    using IndexType = std::size_t;

    using AdjointVariableInformationType = AdjointVariableInformation::VariableInformation<TDim>;

    ///@}
    ///@name Classes
    ///@{

    template<unsigned int TNumNodes, class TElementData>
    class TurbulenceVariableDerivative : public QSVMSDerivativeUtilities<TDim>::Derivative
    {
    public:
        /// name@ Type Definitions
        ///@{

        using BaseType = typename QSVMSDerivativeUtilities<TDim>::Derivative;

        using ElementDataType = TElementData;

        static constexpr double VelocityDerivativeFactor = 0.0;

        static constexpr double PressureDerivativeFactor = 0.0;

        static constexpr unsigned int TDerivativeDimension = 1;

        ///@}
        ///@name Life Cycle
        ///@{

        TurbulenceVariableDerivative(
            const IndexType NodeIndex,
            const IndexType DirectionIndex,
            const GeometryType& rGeometry,
            const double W,
            const Vector& rN,
            const Matrix& rdNdX,
            const double WDerivative,
            const double DetJDerivative,
            const Matrix& rdNdXDerivative)
            : BaseType(NodeIndex, DirectionIndex, rGeometry, W, rN, rdNdX, WDerivative, DetJDerivative, rdNdXDerivative)
        {
        }

        ///@}
        ///@name Operations
        ///@{

        const Variable<double>& GetDerivativeVariable() const;

        array_1d<double, TDim> CalculateEffectiveVelocityDerivative(const array_1d<double, TDim>& rVelocity) const;

        double CalculateElementLengthDerivative(const double ElementLength) const;

        void CalculateStrainRateDerivative(
            Vector& rOutput,
            const Matrix& rNodalVelocity) const;

        ///@}
    };

    /**
     * @brief This class is used with k-omega-sst adjoints
     *
     * k-omega-sst turbulence model calculate nu_t using tke, omega, and velocity_gradient.
     * Therefore, in the case of the derivative w.r.t. VELOCITY, we need to calculate
     * velocity_gradient derivatives as well. Then QSVMSDerivatives will apply chain rule
     * to computed velocity_gradient derivatives to convert them to velocity derivatives.
     *
     * @tparam TNumNodes
     */
    template<unsigned int TNumNodes>
    class KOmegaSSTVelocityDerivative : public QSVMSDerivativeUtilities<TDim>::VelocityDerivative<TNumNodes>
    {
    public:
        /// name@ Type Definitions
        ///@{

        using BaseType = typename QSVMSDerivativeUtilities<TDim>::VelocityDerivative<TNumNodes>;

        static constexpr double VelocityDerivativeFactor = 1.0;

        static constexpr double PressureDerivativeFactor = 0.0;

        static constexpr unsigned int TDerivativeDimension = TDim;

        ///@}
        ///@name Life Cycle
        ///@{

        KOmegaSSTVelocityDerivative(
            const IndexType NodeIndex,
            const IndexType DirectionIndex,
            const GeometryType& rGeometry,
            const double W,
            const Vector& rN,
            const Matrix& rdNdX,
            const double WDerivative,
            const double DetJDerivative,
            const Matrix& rdNdXDerivative)
            : BaseType(NodeIndex, DirectionIndex, rGeometry, W, rN, rdNdX, WDerivative, DetJDerivative, rdNdXDerivative)
        {
        }

        ///@}
        ///@name Operations
        ///@{

        std::vector<AdjointVariableInformationType> GetEffectiveViscosityDependentVariables() const;

        ///@}
    };

    /**
     * @brief This class is used with k-omega-sst adjoints
     *
     * k-omega-sst turbulence model calculate nu_t using tke, omega, and velocity_gradient.
     * Therefore, in the case of the derivative w.r.t. VELOCITY, we need to calculate
     * velocity_gradient derivatives as well. Then QSVMSDerivatives will apply chain rule
     * to computed velocity_gradient derivatives to convert them to velocity derivatives.
     *
     * @tparam TNumNodes
     */
    template<unsigned int TNumNodes>
    class KOmegaSSTShapeDerivative : public QSVMSDerivativeUtilities<TDim>::ShapeDerivative<TNumNodes>
    {
    public:
        /// name@ Type Definitions
        ///@{

        using BaseType = typename QSVMSDerivativeUtilities<TDim>::ShapeDerivative<TNumNodes>;

        static constexpr double VelocityDerivativeFactor = 0.0;

        static constexpr double PressureDerivativeFactor = 0.0;

        static constexpr unsigned int TDerivativeDimension = TDim;

        ///@}
        ///@name Life Cycle
        ///@{

        KOmegaSSTShapeDerivative(
            const IndexType NodeIndex,
            const IndexType DirectionIndex,
            const GeometryType& rGeometry,
            const double W,
            const Vector& rN,
            const Matrix& rdNdX,
            const double WDerivative,
            const double DetJDerivative,
            const Matrix& rdNdXDerivative)
            : BaseType(NodeIndex, DirectionIndex, rGeometry, W, rN, rdNdX, WDerivative, DetJDerivative, rdNdXDerivative)
        {
        }

        ///@}
        ///@name Operations
        ///@{

        std::vector<AdjointVariableInformationType> GetEffectiveViscosityDependentVariables() const;

        ///@}
    };

    ///@}
};

///@}

} // namespace Kratos

#endif // KRATOS_RANS_QS_VMS_DERIVATIVE_UTILITIES_H