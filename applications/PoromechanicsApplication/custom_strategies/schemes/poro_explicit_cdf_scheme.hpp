//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license:
//kratos/license.txt
//
//  Main authors:    Ignasi de Pouplana
//
//

#if !defined(KRATOS_PORO_EXPLICIT_CDF_SCHEME_HPP_INCLUDED)
#define KRATOS_PORO_EXPLICIT_CDF_SCHEME_HPP_INCLUDED

/* External includes */

/* Project includes */
#include "custom_strategies/schemes/poro_explicit_cd_scheme.hpp"
#include "utilities/variable_utils.h"

// Application includes
#include "poromechanics_application_variables.h"

namespace Kratos {

///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

///@}

///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{

/**
 * @class PoroExplicitCDFScheme
 * @ingroup StructuralMechanicsApplciation
 * @brief An explicit forward euler scheme with a split of the inertial term
 * @author Ignasi de Pouplana
 */
template <class TSparseSpace,
          class TDenseSpace //= DenseSpace<double>
          >
class PoroExplicitCDFScheme
    : public PoroExplicitCDScheme<TSparseSpace, TDenseSpace> {

public:
    ///@name Type Definitions
    ///@{

    /// The definition of the base type
    typedef Scheme<TSparseSpace, TDenseSpace> BaseofBaseType;
    typedef PoroExplicitCDScheme<TSparseSpace, TDenseSpace> BaseType;

    /// Some definitions related with the base class
    typedef typename BaseType::DofsArrayType DofsArrayType;
    typedef typename BaseType::TSystemMatrixType TSystemMatrixType;
    typedef typename BaseType::TSystemVectorType TSystemVectorType;
    typedef typename BaseType::LocalSystemVectorType LocalSystemVectorType;

    /// The arrays of elements and nodes
    typedef ModelPart::ElementsContainerType ElementsArrayType;
    typedef ModelPart::ConditionsContainerType ConditionsArrayType;
    typedef ModelPart::NodesContainerType NodesArrayType;

    /// Definition of the size type
    typedef std::size_t SizeType;

    /// Definition of the index type
    typedef std::size_t IndexType;

    /// Definition fo the node iterator
    typedef typename ModelPart::NodeIterator NodeIterator;

    /// The definition of the numerical limit
    static constexpr double numerical_limit = std::numeric_limits<double>::epsilon();

    using BaseType::mDeltaTime;
    using BaseType::mAlpha;
    using BaseType::mBeta;
    using BaseType::mTheta;
    using BaseType::mGCoefficient;

    /// Counted pointer of PoroExplicitCDFScheme
    KRATOS_CLASS_POINTER_DEFINITION(PoroExplicitCDFScheme);

    ///@}
    ///@name Life Cycle
    ///@{

    /**
     * @brief Default constructor.
     * @details The PoroExplicitCDFScheme method
     */
    PoroExplicitCDFScheme()
        : PoroExplicitCDScheme<TSparseSpace, TDenseSpace>()
    {

    }

    /** Destructor.
    */
    virtual ~PoroExplicitCDFScheme() {}

    ///@}
    ///@name Operators
    ///@{

    void Initialize(ModelPart& rModelPart) override
    {
        KRATOS_TRY

        BaseType::Initialize(rModelPart);

        const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();

        mDelta = r_current_process_info[DELTA];
        mB0 = r_current_process_info[B_0];
        mB1 = r_current_process_info[B_1];
        mB2 = r_current_process_info[B_2];
        mAlphab = r_current_process_info[RAYLEIGH_ALPHA_B];
        mBetab = r_current_process_info[RAYLEIGH_BETA_B];
        mDelta0 = 7.0/12.0*mDelta;
        mDelta1 = -mDelta/6.0;
        mDelta2 = -mDelta;
        mB = 1.0+23.0/12.0*mDelta;

        KRATOS_CATCH("")
    }

    /**
     * @brief This method updates the translation DoF
     * @param itCurrentNode The iterator of the current node
     * @param DisplacementPosition The position of the displacement dof on the database
     * @param DomainSize The current dimention of the problem
     */
    void UpdateTranslationalDegreesOfFreedom(
        NodeIterator itCurrentNode,
        const IndexType DisplacementPosition,
        const SizeType DomainSize = 3
        ) override
    {
        array_1d<double, 3>& r_displacement = itCurrentNode->FastGetSolutionStepValue(DISPLACEMENT);
        array_1d<double, 3> displacement_aux;
        noalias(displacement_aux) = r_displacement;
        array_1d<double, 3>& r_displacement_old = itCurrentNode->FastGetSolutionStepValue(DISPLACEMENT_OLD);
        array_1d<double, 3>& r_displacement_older = itCurrentNode->FastGetSolutionStepValue(DISPLACEMENT_OLDER);
        const double nodal_mass = itCurrentNode->GetValue(NODAL_MASS);

        double& r_current_water_pressure = itCurrentNode->FastGetSolutionStepValue(WATER_PRESSURE);
        double& r_current_dt_water_pressure = itCurrentNode->FastGetSolutionStepValue(DT_WATER_PRESSURE);      

        const array_1d<double, 3>& r_external_force = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE);
        const array_1d<double, 3>& r_external_force_old = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE,1);
        array_1d<double, 3>& r_external_force_older = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE_OLDER);
        const array_1d<double, 3>& r_internal_force = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE);
        const array_1d<double, 3>& r_internal_force_old = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE,1);
        array_1d<double, 3>& r_internal_force_older = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE_OLDER);

        std::array<bool, 3> fix_displacements = {false, false, false};
        fix_displacements[0] = (itCurrentNode->GetDof(DISPLACEMENT_X, DisplacementPosition).IsFixed());
        fix_displacements[1] = (itCurrentNode->GetDof(DISPLACEMENT_Y, DisplacementPosition + 1).IsFixed());
        if (DomainSize == 3)
            fix_displacements[2] = (itCurrentNode->GetDof(DISPLACEMENT_Z, DisplacementPosition + 2).IsFixed());

        const double eps_hat = (mB0+mB1+mB2)*mDelta*mDeltaTime*mAlphab;
        const double eps_i = (mB0+mB1+mB2)/3.0*mDelta*mDeltaTime*mBetab;

        // CDF_01-03-22
        for (IndexType j = 0; j < DomainSize; j++) {
            if (fix_displacements[j] == false) {
                    r_displacement[j] = ( (2.0*mB-mDeltaTime*(mAlpha+mDelta*mB0*mAlphab))*nodal_mass*r_displacement[j]
                                          - mDeltaTime*(mBeta+mDelta*mB0*mBetab+mDeltaTime*(1.0+mDelta0))*r_internal_force[j]
                                          - (mB-eps_hat+mDeltaTime*(-mAlpha+mDelta*mB1*mAlphab))*nodal_mass*r_displacement_old[j]
                                          - mDeltaTime*(-mBeta+mDelta*mB1*mBetab+mDeltaTime*mDelta1)*r_internal_force_old[j]
                                          - mDeltaTime*mDelta*mB2*mAlphab*nodal_mass*r_displacement_older[j]
                                          - mDeltaTime*(mDelta*mB2*mBetab+mDeltaTime*mDelta2)*r_internal_force_older[j]
                                          + mDeltaTime*mDeltaTime*((1.0+mDelta0)*r_external_force[j]+mDelta1*r_external_force_old[j]+mDelta2*r_external_force_older[j])
                                          + eps_i*(r_external_force[j]+r_external_force_old[j]+r_external_force_older[j])
                                        ) / ( nodal_mass*mB );
            }
        }

        // Solution of the darcy_equation
        if( itCurrentNode->IsFixed(WATER_PRESSURE) == false ) {
            // TODO: this is on standby
            r_current_water_pressure = 0.0;
            r_current_dt_water_pressure = 0.0;
        }

        noalias(r_displacement_older) = r_displacement_old;
        noalias(r_displacement_old) = displacement_aux;
        noalias(r_external_force_older) = r_external_force_old;
        noalias(r_internal_force_older) = r_internal_force_old;
        const array_1d<double, 3>& r_velocity_old = itCurrentNode->FastGetSolutionStepValue(VELOCITY,1);
        array_1d<double, 3>& r_velocity = itCurrentNode->FastGetSolutionStepValue(VELOCITY);
        array_1d<double, 3>& r_acceleration = itCurrentNode->FastGetSolutionStepValue(ACCELERATION);

        noalias(r_velocity) = (1.0/mDeltaTime) * (r_displacement - r_displacement_old);
        noalias(r_acceleration) = (1.0/mDeltaTime) * (r_velocity - r_velocity_old);
    }

    /**
     * @brief This method updates the translation DoF
     * @param itCurrentNode The iterator of the current node
     * @param DisplacementPosition The position of the displacement dof on the database
     * @param DomainSize The current dimention of the problem
     */
    void UpdateTranslationalDegreesOfFreedomWithNodalMassArray(
        NodeIterator itCurrentNode,
        const IndexType DisplacementPosition,
        const SizeType DomainSize = 3
        ) override
    {
        array_1d<double, 3>& r_displacement = itCurrentNode->FastGetSolutionStepValue(DISPLACEMENT);
        array_1d<double, 3> displacement_aux;
        noalias(displacement_aux) = r_displacement;
        array_1d<double, 3>& r_displacement_old = itCurrentNode->FastGetSolutionStepValue(DISPLACEMENT_OLD);
        array_1d<double, 3>& r_displacement_older = itCurrentNode->FastGetSolutionStepValue(DISPLACEMENT_OLDER);
        const array_1d<double, 3>& r_nodal_mass_array = itCurrentNode->GetValue(NODAL_MASS_ARRAY);

        double& r_current_water_pressure = itCurrentNode->FastGetSolutionStepValue(WATER_PRESSURE);
        double& r_current_dt_water_pressure = itCurrentNode->FastGetSolutionStepValue(DT_WATER_PRESSURE);      

        const array_1d<double, 3>& r_external_force = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE);
        const array_1d<double, 3>& r_external_force_old = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE,1);
        array_1d<double, 3>& r_external_force_older = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE_OLDER);
        const array_1d<double, 3>& r_internal_force = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE);
        const array_1d<double, 3>& r_internal_force_old = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE,1);
        array_1d<double, 3>& r_internal_force_older = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE_OLDER);

        std::array<bool, 3> fix_displacements = {false, false, false};
        fix_displacements[0] = (itCurrentNode->GetDof(DISPLACEMENT_X, DisplacementPosition).IsFixed());
        fix_displacements[1] = (itCurrentNode->GetDof(DISPLACEMENT_Y, DisplacementPosition + 1).IsFixed());
        if (DomainSize == 3)
            fix_displacements[2] = (itCurrentNode->GetDof(DISPLACEMENT_Z, DisplacementPosition + 2).IsFixed());

        // CDF_01-03-22
        for (IndexType j = 0; j < DomainSize; j++) {
            if (fix_displacements[j] == false) {
                    r_displacement[j] = ( (2.0*mB-mDeltaTime*(mAlpha+mDelta*mB0*mAlphab))*r_nodal_mass_array[j]*r_displacement[j]
                                          - mDeltaTime*(mBeta+mDelta*mB0*mBetab+mDeltaTime*(1.0+mDelta0))*r_internal_force[j]
                                          - (mB+mDeltaTime*(-mAlpha+mDelta*mB1*mAlphab))*r_nodal_mass_array[j]*r_displacement_old[j]
                                          - mDeltaTime*(-mBeta+mDelta*mB1*mBetab+mDeltaTime*mDelta1)*r_internal_force_old[j]
                                          - mDeltaTime*mDelta*mB2*mAlphab*r_nodal_mass_array[j]*r_displacement_older[j]
                                          - mDeltaTime*(mDelta*mB2*mBetab+mDeltaTime*mDelta2)*r_internal_force_older[j]
                                          + mDeltaTime*mDeltaTime*((1.0+mDelta0)*r_external_force[j]+mDelta1*r_external_force_old[j]+mDelta2*r_external_force_older[j])
                                        ) / ( r_nodal_mass_array[j]*mB );
            }
        }

        // Solution of the darcy_equation
        if( itCurrentNode->IsFixed(WATER_PRESSURE) == false ) {
            // TODO: this is on standby
            r_current_water_pressure = 0.0;
            r_current_dt_water_pressure = 0.0;
        }

        noalias(r_displacement_older) = r_displacement_old;
        noalias(r_displacement_old) = displacement_aux;
        noalias(r_external_force_older) = r_external_force_old;
        noalias(r_internal_force_older) = r_internal_force_old;
        const array_1d<double, 3>& r_velocity_old = itCurrentNode->FastGetSolutionStepValue(VELOCITY,1);
        array_1d<double, 3>& r_velocity = itCurrentNode->FastGetSolutionStepValue(VELOCITY);
        array_1d<double, 3>& r_acceleration = itCurrentNode->FastGetSolutionStepValue(ACCELERATION);

        noalias(r_velocity) = (1.0/mDeltaTime) * (r_displacement - r_displacement_old);
        noalias(r_acceleration) = (1.0/mDeltaTime) * (r_velocity - r_velocity_old);
    }

        // TODO. Older CDF tests
        // CDF-1d
        // for (IndexType j = 0; j < DomainSize; j++) {
        //     if (fix_displacements[j] == false) {
        //             r_displacement[j] = ( (2.0*(1.0+mDelta)-mDeltaTime*(mAlpha+mDelta*mAlphab))*nodal_mass*r_displacement[j]
        //                                   + (mDeltaTime*(mAlpha+2.0*mDelta*mAlphab)-(1.0+mDelta))*nodal_mass*r_displacement_old[j]
        //                                   - mDeltaTime*mDelta*mAlphab*nodal_mass*r_displacement_older[j]
        //                                   - mDeltaTime*(mBeta+mDelta*mBetab+mDeltaTime*(mGamma+mDelta*mKappa0))*r_internal_force[j]
        //                                   + mDeltaTime*(mBeta+2.0*mDelta*mBetab-mDeltaTime*(1.0-mGamma+mDelta*mKappa1))*r_internal_force_old[j]
        //                                   - mDeltaTime*mDelta*mBetab*r_internal_force_older[j]
        //                                   + mDeltaTime*mDeltaTime*((mGamma+mDelta*mKappa0)*r_external_force[j]+(1.0-mGamma+mDelta*mKappa1)*r_external_force_old[j])
        //                                 ) / ( nodal_mass*(1.0+mDelta) );
        //     }
        // }
        // CDF-1d*
        // for (IndexType j = 0; j < DomainSize; j++) {
        //     if (fix_displacements[j] == false) {
        //             r_displacement[j] = ( (2.0+mDelta-mDeltaTime*(mAlpha+mDelta*mAlphab))*nodal_mass*r_displacement[j]
        //                                   + (mDeltaTime*(mAlpha+2.0*mDelta*mAlphab)-(1.0+2.0*mDelta))*nodal_mass*r_displacement_old[j]
        //                                   + (mDelta-mDeltaTime*mDelta*mAlphab)*nodal_mass*r_displacement_older[j]
        //                                   - mDeltaTime*(mBeta+mDelta*mBetab+mDeltaTime*(mGamma-mDelta*mKappa0))*r_internal_force[j]
        //                                   + mDeltaTime*(mBeta+2.0*mDelta*mBetab-mDeltaTime*(1.0-mGamma-mDelta*mKappa1))*r_internal_force_old[j]
        //                                   - mDeltaTime*mDelta*mBetab*r_internal_force_older[j]
        //                                   + mDeltaTime*mDeltaTime*((mGamma-mDelta*mKappa0)*r_external_force[j]+(1.0-mGamma-mDelta*mKappa1)*r_external_force_old[j])
        //                                 ) / ( nodal_mass );
        //     }
        // }
        // CDF-1
        // for (IndexType j = 0; j < DomainSize; j++) {
        //     if (fix_displacements[j] == false) {
        //             r_displacement[j] = ( (2.0*(1.0-mDelta)-mAlpha*mDeltaTime)*nodal_mass*r_displacement[j]
        //                                   + (mDelta-1.0+mAlpha*mDeltaTime)*nodal_mass*r_displacement_old[j]
        //                                   - mDeltaTime*(mBeta+mDeltaTime*(mGamma-mDelta*mKappa0))*r_internal_force[j]
        //                                   + mDeltaTime*(mBeta-mDeltaTime*(1.0-mGamma-mDelta*mKappa1))*r_internal_force_old[j]
        //                                   + mDeltaTime*mDeltaTime*((mGamma-mDelta*mKappa0)*r_external_force[j]+(1.0-mGamma-mDelta*mKappa1)*r_external_force_old[j])
        //                                 ) / ( nodal_mass*(1.0-mDelta) );
        //     }
        // }
        // CDF-12bd
        // for (IndexType j = 0; j < DomainSize; j++) {
        //     if (fix_displacements[j] == false) {
        //             r_displacement[j] = ( (2.0+mDelta+3.5*mDeltab-mAlpha*mDeltaTime)*nodal_mass*r_displacement[j]
        //                                   + (mAlpha*mDeltaTime-1.0+mDelta-4.0*mDeltab)*nodal_mass*r_displacement_old[j]
        //                                   + (1.5*mDeltab-mDelta)*nodal_mass*r_displacement_older[j]
        //                                   - mDeltaTime*(mBeta+0.5*mDeltaTime*(0.75+mDelta-mDeltab))*r_internal_force[j]
        //                                   + mDeltaTime*(mBeta-0.5*mDeltaTime*(1.0+mDelta-mDeltab))*r_internal_force_old[j]
        //                                   - 0.125*mDeltaTime*mDeltaTime*r_internal_force_older[j]
        //                                   + mDeltaTime*mDeltaTime*(0.5*(0.75+mDelta-mDeltab)*r_external_force[j]+0.5*(1.0+mDelta-mDeltab)*r_external_force_old[j]+0.125*r_external_force_older[j])
        //                                 ) / ( nodal_mass*(1.0+mDelta+mDeltab) );
        //     }
        // }


    ///@}
    ///@name Operations
    ///@{

    ///@}
    ///@name Access
    ///@{

    ///@}
    ///@name Inquiry
    ///@{

    ///@}
    ///@name Friends
    ///@{

    ///@}

protected:

    double mDelta;
    double mB0;
    double mB1;
    double mB2;
    double mAlphab;
    double mBetab;
    double mDelta0;
    double mDelta1;
    double mDelta2;
    double mB;

    ///@}
    ///@name Protected Structs
    ///@{


    ///@name Protected static Member Variables
    ///@{


    ///@}
    ///@name Protected member Variables
    ///@{

    ///@}
    ///@name Protected Operators
    ///@{

    ///@}
    ///@name Protected Operations
    ///@{

    ///@}
    ///@name Protected  Access
    ///@{

    ///@}
    ///@name Protected Inquiry
    ///@{

    ///@}
    ///@name Protected LifeCycle
    ///@{

    ///@}

private:
    ///@name Static Member Variables
    ///@{

    ///@}
    ///@name Member Variables
    ///@{

    ///@}
    ///@name Private Operators
    ///@{

    ///@}
    ///@name Private Operations
    ///@{

    ///@}
    ///@name Private  Access
    ///@{

    ///@}
    ///@name Private Inquiry
    ///@{

    ///@}
    ///@name Un accessible methods
    ///@{

    ///@}

}; /* Class PoroExplicitCDFScheme */

///@}

///@name Type Definitions
///@{

///@}

} /* namespace Kratos.*/

#endif /* KRATOS_PORO_EXPLICIT_CDF_SCHEME_HPP_INCLUDED  defined */
