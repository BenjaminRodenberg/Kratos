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

#if !defined(KRATOS_QS_VMS_DERIVATIVE_UTILITIES_H)
#define KRATOS_QS_VMS_DERIVATIVE_UTILITIES_H

// System includes
#include <array>

// External includes

// Project includes
#include "containers/variable.h"
#include "fluid_dynamics_application_variables.h"
#include "geometries/geometry.h"
#include "includes/constitutive_law.h"
#include "includes/node.h"
#include "includes/process_info.h"
#include "includes/ublas_interface.h"
#include "utilities/time_discretization.h"

// Application includes
#include "custom_utilities/fluid_adjoint_utilities.h"
#include "custom_utilities/fluid_adjoint_variable_information.h"

namespace Kratos
{
///@name Kratos Classes
///@{

template <unsigned int TDim>
class QSVMSDerivativeUtilities
{
public:
    ///@name Type Definitions
    ///@{

    using NodeType = Node<3>;

    using GeometryType = Geometry<NodeType>;

    using IndexType = std::size_t;

    using AdjointVariableInformationType = AdjointVariableInformation::VariableInformation<TDim>;

    using DerivativeGradientsArray = std::array<const Variable<double>*, 9>;

    constexpr static IndexType TStrainSize = (TDim - 1) * 3; // 3 in 2D, 6 in 3D

    ///@}
    ///@name Static Operations
    ///@{

    // Ask Ruben: This is redundant code, can we agree to
    // create qvms_utilities.h, and store this there to be used in primal and adjoint analysis?
    static void CalculateStrainRate(
        Vector& rOutput,
        const Matrix& rNodalVelocity,
        const Matrix& rdNdX);

    static const std::array<const Variable<double>*, TStrainSize> GetStrainRateVariables();

    ///@}
    ///@name Classes
    ///@{

    class Derivative
    {
    public:
        ///@name Life Cycle
        ///@{

        Derivative(
            const IndexType NodeIndex,
            const IndexType DirectionIndex,
            const GeometryType& rGeometry,
            const double W,
            const Vector& rN,
            const Matrix& rdNdX,
            const double WDerivative,
            const double DetJDerivative,
            const Matrix& rdNdXDerivative);

        ///@}
        ///@name Operations
        ///@{

        std::vector<AdjointVariableInformationType> GetEffectiveViscosityDependentVariables() const { return std::vector<AdjointVariableInformationType>{{}}; }

        ///@}

    protected:
        ///@name Private Members
        ///@{

        const IndexType mNodeIndex;
        const IndexType mDirectionIndex;
        const GeometryType& mrGeometry;
        const double mW;
        const Vector& mrN;
        const Matrix& mrdNdX;
        const double mWDerivative;
        const double mDetJDerivative;
        const Matrix& mrdNdXDerivative;

        ///@}
    };

    template<unsigned int TNumNodes>
    class VelocityDerivative : public Derivative
    {
    public:
        /// name@ Type Definitions
        ///@{

        using BaseType = Derivative;

        static constexpr double VelocityDerivativeFactor = 1.0;

        static constexpr double PressureDerivativeFactor = 0.0;

        static constexpr unsigned int TDerivativeDimension = TDim;

        ///@}
        ///@name Life Cycle
        ///@{

        VelocityDerivative(
            const IndexType NodeIndex,
            const IndexType DirectionIndex,
            const GeometryType& rGeometry,
            const double W,
            const Vector& rN,
            const Matrix& rdNdX,
            const double WDerivative,
            const double DetJDerivative,
            const Matrix& rdNdXDerivative)
            : BaseType(NodeIndex, DirectionIndex, rGeometry, W, rN, rdNdX, WDerivative, DetJDerivative, rdNdXDerivative),
              mVariableInformation(DirectionIndex)
        {
        }

        ///@}
        ///@name Operations
        ///@{

        const Variable<double>& GetDerivativeVariable() const { return mVariableInformation.GetVariable(); }

        array_1d<double, TDim> CalculateEffectiveVelocityDerivative(const array_1d<double, TDim>& rVelocity) const;

        double CalculateElementLengthDerivative(const double ElementLength) const;

        void CalculateStrainRateDerivative(
            Vector& rOutput,
            const Matrix& rNodalVelocity) const;

        ///@}
    private:
        ///@name Private Members
        ///@{

        const AdjointVariableInformation::VelocityInformation<TDim> mVariableInformation;

        ///@}
    };

    template<unsigned int TNumNodes>
    class PressureDerivative : public Derivative
    {
    public:
        /// name@ Type Definitions
        ///@{

        using BaseType = Derivative;

        static constexpr double VelocityDerivativeFactor = 0.0;

        static constexpr double PressureDerivativeFactor = 1.0;

        static constexpr unsigned int TDerivativeDimension = 1;

        ///@}
        ///@name Life Cycle
        ///@{

        PressureDerivative(
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

        const Variable<double>& GetDerivativeVariable() const { return PRESSURE; }

        array_1d<double, TDim> CalculateEffectiveVelocityDerivative(const array_1d<double, TDim>& rVelocity) const;

        double CalculateElementLengthDerivative(const double ElementLength) const;

        void CalculateStrainRateDerivative(
            Vector& rOutput,
            const Matrix& rNodalVelocity) const;

        ///@}
    };

    template<unsigned int TNumNodes>
    class ShapeDerivative : public Derivative
    {
    public:
        /// name@ Type Definitions
        ///@{

        using BaseType = Derivative;

        static constexpr double VelocityDerivativeFactor = 0.0;

        static constexpr double PressureDerivativeFactor = 0.0;

        static constexpr unsigned int TDerivativeDimension = TDim;

        ///@}
        ///@name Life Cycle
        ///@{

        ShapeDerivative(
            const IndexType NodeIndex,
            const IndexType DirectionIndex,
            const GeometryType& rGeometry,
            const double W,
            const Vector& rN,
            const Matrix& rdNdX,
            const double WDerivative,
            const double DetJDerivative,
            const Matrix& rdNdXDerivative)
            : BaseType(NodeIndex, DirectionIndex, rGeometry, W, rN, rdNdX, WDerivative, DetJDerivative, rdNdXDerivative),
              mVariableInformation(DirectionIndex)
        {
        }

        ///@}
        ///@name Operations
        ///@{

        const Variable<double>& GetDerivativeVariable() const { return mVariableInformation.GetVariable(); }

        array_1d<double, TDim> CalculateEffectiveVelocityDerivative(const array_1d<double, TDim>& rVelocity) const;

        double CalculateElementLengthDerivative(const double ElementLength) const;

        void CalculateStrainRateDerivative(
            Vector& rOutput,
            const Matrix& rNodalVelocity) const;

        ///@}
    private:
        ///@name Private Members
        ///@{

        const AdjointVariableInformation::ShapeSensitivityInformation<TDim> mVariableInformation;

        ///@}
    };

    ///@}
private:
    ///@name Private Static Operations
    ///@{

    static void CalculateStrainRateVelocityDerivative(
        Vector& rOutput,
        const IndexType DerivativeNodeIndex,
        const IndexType DerivativeDirectionIndex,
        const Matrix& rdNdX);

    ///@}
};

///@}

} // namespace Kratos

#endif // KRATOS_QS_VMS_DERIVATIVE_UTILITIES_H