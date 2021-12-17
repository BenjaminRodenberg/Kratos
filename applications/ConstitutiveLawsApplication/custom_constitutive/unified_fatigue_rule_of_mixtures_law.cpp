// KRATOS ___                _   _ _         _   _             __                       _
//       / __\___  _ __  ___| |_(_) |_ _   _| |_(_)_   _____  / /  __ ___      _____   /_\  _ __  _ __
//      / /  / _ \| '_ \/ __| __| | __| | | | __| \ \ / / _ \/ /  / _` \ \ /\ / / __| //_\\| '_ \| '_  |
//     / /__| (_) | | | \__ \ |_| | |_| |_| | |_| |\ V /  __/ /__| (_| |\ V  V /\__ \/  _  \ |_) | |_) |
//     \____/\___/|_| |_|___/\__|_|\__|\__,_|\__|_| \_/ \___\____/\__,_| \_/\_/ |___/\_/ \_/ .__/| .__/
//                                                                                         |_|   |_|
//
//  License:         BSD License
//                   license: structural_mechanics_application/license.txt
//
//  Main authors:    Sergio Jimenez
//                   Alejandro Cornejo
//  Collaborator:    Lucia Barbu
//

// System includes

// External includes

// Project includes
#include "utilities/math_utils.h"
#include "constitutive_laws_application_variables.h"
#include "unified_fatigue_rule_of_mixtures_law.h"
#include "custom_utilities/tangent_operator_calculator_utility.h"
#include "custom_constitutive/constitutive_laws_integrators/high_cycle_fatigue_law_integrator.h"
#include "custom_constitutive/constitutive_laws_integrators/generic_constitutive_law_integrator_damage.h"
#include "custom_constitutive/constitutive_laws_integrators/generic_constitutive_law_integrator_plasticity.h"

// Yield surfaces
#include "custom_constitutive/yield_surfaces/generic_yield_surface.h"
#include "custom_constitutive/yield_surfaces/von_mises_yield_surface.h"
#include "custom_constitutive/yield_surfaces/modified_mohr_coulomb_yield_surface.h"
#include "custom_constitutive/yield_surfaces/mohr_coulomb_yield_surface.h"
#include "custom_constitutive/yield_surfaces/rankine_yield_surface.h"
#include "custom_constitutive/yield_surfaces/simo_ju_yield_surface.h"
#include "custom_constitutive/yield_surfaces/drucker_prager_yield_surface.h"
#include "custom_constitutive/yield_surfaces/tresca_yield_surface.h"

// Plastic potentials
#include "custom_constitutive/plastic_potentials/generic_plastic_potential.h"
#include "custom_constitutive/plastic_potentials/von_mises_plastic_potential.h"
#include "custom_constitutive/plastic_potentials/tresca_plastic_potential.h"
#include "custom_constitutive/plastic_potentials/modified_mohr_coulomb_plastic_potential.h"
#include "custom_constitutive/plastic_potentials/mohr_coulomb_plastic_potential.h"
#include "custom_constitutive/plastic_potentials/drucker_prager_plastic_potential.h"

namespace Kratos
{
template<class TConstLawIntegratorType>
ConstitutiveLaw::Pointer UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::Create(Kratos::Parameters NewParameters) const
{
    const double high_cycle_fatigue_initial_volumetric_participation = NewParameters["combination_factors"][0].GetDouble();
    return Kratos::make_shared<UnifiedFatigueRuleOfMixturesLaw>(high_cycle_fatigue_initial_volumetric_participation);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::InitializeMaterialResponsePK1(
    ConstitutiveLaw::Parameters& rValues
    )
{
    InitializeMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::InitializeMaterialResponsePK2(
    ConstitutiveLaw::Parameters& rValues
    )
{
    InitializeMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::InitializeMaterialResponseKirchhoff(
    ConstitutiveLaw::Parameters& rValues
    )
{
    InitializeMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::InitializeMaterialResponseCauchy(ConstitutiveLaw::Parameters& rValues)
{
    const double max_stress = mMaxStress;
    const double min_stress = mMinStress;
    bool max_indicator = mMaxDetected;
    bool min_indicator = mMinDetected;
    double fatigue_reduction_factor = mFatigueReductionFactor;
    double reversion_factor_relative_error = mReversionFactorRelativeError;
    double max_stress_relative_error = mMaxStressRelativeError;
    unsigned int global_number_of_cycles = mNumberOfCyclesGlobal;
    unsigned int local_number_of_cycles = mNumberOfCyclesLocal;
    double B0 = mFatigueReductionParameter;
    double previous_max_stress = mPreviousMaxStress;
    double previous_min_stress = mPreviousMinStress;
    double wohler_stress = mWohlerStress;
    bool new_cycle = false;
    double s_th = mThresholdStress;
    double cycles_to_failure = mCyclesToFailure;
    bool adnvance_strategy_applied = rValues.GetProcessInfo()[ADVANCE_STRATEGY_APPLIED];
    bool damage_activation = rValues.GetProcessInfo()[DAMAGE_ACTIVATION];

    if (max_indicator && min_indicator) {
        auto& r_material_properties = rValues.GetMaterialProperties();
        const auto it_cl_begin = r_material_properties.GetSubProperties().begin();
        const auto& r_props_HCF_cl = *(it_cl_begin);
        const auto& r_props_ULCF_cl = *(it_cl_begin + 1);
        ConstitutiveLaw::Parameters values_fatigue  = rValues;

        //Checking which material has the fatigue properties
        if (r_props_HCF_cl.Has(HIGH_CYCLE_FATIGUE_COEFFICIENTS)) {
                values_fatigue.SetMaterialProperties(r_props_HCF_cl);
        } else if (r_props_ULCF_cl.Has(HIGH_CYCLE_FATIGUE_COEFFICIENTS)) {
                values_fatigue.SetMaterialProperties(r_props_ULCF_cl);
        } else {
                KRATOS_ERROR << "Fatigue properties not defined" << std::endl;
        }

        const double previous_reversion_factor = HighCycleFatigueLawIntegrator<6>::CalculateReversionFactor(previous_max_stress, previous_min_stress);
        const double reversion_factor = HighCycleFatigueLawIntegrator<6>::CalculateReversionFactor(max_stress, min_stress);
        double alphat;
        HighCycleFatigueLawIntegrator<6>::CalculateFatigueParameters(
            max_stress,
            reversion_factor,
            values_fatigue.GetMaterialProperties(),
            B0,
            s_th,
            alphat,
            cycles_to_failure);

        double betaf = values_fatigue.GetMaterialProperties()[HIGH_CYCLE_FATIGUE_COEFFICIENTS][4];
        if (std::abs(min_stress) < 0.001) {
            reversion_factor_relative_error = std::abs(reversion_factor - previous_reversion_factor);
        } else {
            reversion_factor_relative_error = std::abs((reversion_factor - previous_reversion_factor) / reversion_factor);
        }
        if (std::abs(max_stress) < 0.001) {
            max_stress_relative_error = std::abs(max_stress - previous_max_stress);
        } else {
            max_stress_relative_error = std::abs((max_stress - previous_max_stress) / max_stress);
        }
        if (!damage_activation && global_number_of_cycles > 2 && !adnvance_strategy_applied && (reversion_factor_relative_error > 0.001 || max_stress_relative_error > 0.001)) {
            local_number_of_cycles = std::trunc(std::pow(10, std::pow(-(std::log(fatigue_reduction_factor) / B0), 1.0 / (betaf * betaf)))) + 1;
        }
        global_number_of_cycles++;
        local_number_of_cycles++;
        new_cycle = true;
        max_indicator = false;
        min_indicator = false;
        previous_max_stress = max_stress;
        previous_min_stress = min_stress;
        mCyclesToFailure = cycles_to_failure;

        HighCycleFatigueLawIntegrator<6>::CalculateFatigueReductionFactorAndWohlerStress(values_fatigue.GetMaterialProperties(),
                                                                                        max_stress,
                                                                                        local_number_of_cycles,
                                                                                        global_number_of_cycles,
                                                                                        B0,
                                                                                        s_th,
                                                                                        alphat,
                                                                                        fatigue_reduction_factor,
                                                                                        wohler_stress);
    }
    // if (adnvance_strategy_applied) {
    //     const double reversion_factor = HighCycleFatigueLawIntegrator<6>::CalculateReversionFactor(max_stress, min_stress);
    //     double alphat;
    //     HighCycleFatigueLawIntegrator<6>::CalculateFatigueParameters(
    //         max_stress,
    //         reversion_factor,
    //         rValues.GetMaterialProperties(),
    //         B0,
    //         s_th,
    //         alphat,
    //         cycles_to_failure);
    //     HighCycleFatigueLawIntegrator<6>::CalculateFatigueReductionFactorAndWohlerStress(rValues.GetMaterialProperties(),
    //                                                                                     max_stress,
    //                                                                                     local_number_of_cycles,
    //                                                                                     global_number_of_cycles,
    //                                                                                     B0,
    //                                                                                     s_th,
    //                                                                                     alphat,
    //                                                                                     fatigue_reduction_factor,
    //                                                                                     wohler_stress);
    // }
    mNumberOfCyclesGlobal = global_number_of_cycles;
    mNumberOfCyclesLocal = local_number_of_cycles;
    mReversionFactorRelativeError = reversion_factor_relative_error;
    mMaxStressRelativeError = max_stress_relative_error;
    mMaxDetected = max_indicator;
    mMinDetected = min_indicator;
    mFatigueReductionParameter = B0;
    mPreviousMaxStress = previous_max_stress;
    mPreviousMinStress = previous_min_stress;
    mFatigueReductionFactor = fatigue_reduction_factor;
    mWohlerStress = wohler_stress;
    mNewCycleIndicator = new_cycle;
    mThresholdStress = s_th;
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateMaterialResponsePK1(ConstitutiveLaw::Parameters& rValues)
{
    this->CalculateMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateMaterialResponsePK2(ConstitutiveLaw::Parameters& rValues)
{
    this->CalculateMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateMaterialResponseKirchhoff(ConstitutiveLaw::Parameters& rValues)
{
    this->CalculateMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateMaterialResponseCauchy(ConstitutiveLaw::Parameters& rValues)
{
    // Some auxiliar values
    const SizeType dimension = WorkingSpaceDimension();
    const SizeType voigt_size = GetStrainSize();

    // Get Values to compute the constitutive law:
    Flags& r_flags = rValues.GetOptions();

    // Previous flags saved
    const bool flag_strain = r_flags.Is(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN );
    const bool flag_const_tensor = r_flags.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR );
    const bool flag_stress = r_flags.Is(ConstitutiveLaw::COMPUTE_STRESS );

    const Properties& r_material_properties  = rValues.GetMaterialProperties();

    // The deformation gradient
    if (rValues.IsSetDeterminantF()) {
        const double determinant_f = rValues.GetDeterminantF();
        KRATOS_ERROR_IF(determinant_f < 0.0) << "Deformation gradient determinant (detF) < 0.0 : " << determinant_f << std::endl;
    }
    // In case the element has not computed the Strain
    if (r_flags.IsNot(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN)) {
        Vector& r_strain_vector = rValues.GetStrainVector();

        Matrix F_deformation_gradient(dimension, dimension);
        this->CalculateValue(rValues, DEFORMATION_GRADIENT, F_deformation_gradient);
        const Matrix B_matrix = prod(F_deformation_gradient, trans(F_deformation_gradient));
        // Doing resize in case is needed
        if (r_strain_vector.size() != voigt_size)
            r_strain_vector.resize(voigt_size);

         // Identity matrix
        Matrix identity_matrix(dimension, dimension);
        for (IndexType i = 0; i < dimension; ++i) {
            for (IndexType j = 0; j < dimension; ++j) {
                if (i == j) identity_matrix(i, j) = 1.0;
                else identity_matrix(i, j) = 0.0;
            }
        }

        // Calculating the inverse of the left Cauchy tensor
        Matrix inverse_B_tensor(dimension, dimension);
        double aux_det_b = 0;
        MathUtils<double>::InvertMatrix(B_matrix, inverse_B_tensor, aux_det_b);

        // Calculate E matrix
        const Matrix E_matrix = 0.5 * (identity_matrix - inverse_B_tensor);
        // Almansi Strain Calculation
        r_strain_vector = MathUtils<double>::StrainTensorToVector(E_matrix, voigt_size);
    }

    if (r_flags.Is(ConstitutiveLaw::COMPUTE_STRESS)) {
        // Set new flags
        r_flags.Set(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN, true);
        r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, false);
        r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, true);

        // Total strain vector hich is equal to component CLs strains
        Vector& r_strain_vector = rValues.GetStrainVector();

        // This method integrates the stress according to each simple material CL
        Vector high_cycle_fatigue_stress_vector, ultra_low_cycle_fatigue_stress_vector;
        this->IntegrateStressesOfHCFAndULCFModels(rValues, r_strain_vector, r_strain_vector, high_cycle_fatigue_stress_vector, ultra_low_cycle_fatigue_stress_vector);

        Vector& r_integrated_stress_vector = rValues.GetStressVector();
        noalias(r_integrated_stress_vector) = mHCFVolumetricParticipation * high_cycle_fatigue_stress_vector
                                     + (1.0 - mHCFVolumetricParticipation) * ultra_low_cycle_fatigue_stress_vector;

        // Previous flags restored
        r_flags.Set(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN, flag_strain);
        r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor);
        r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, flag_stress);

        if (r_flags.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR)) {
            this->CalculateTangentTensor(rValues);
        }
    }

} // End CalculateMaterialResponseCauchy

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::IntegrateStressesOfHCFAndULCFModels(
    ConstitutiveLaw::Parameters& rValues,
    Vector rHCFStrainVector,
    Vector rULCFStrainVector,
    Vector& rHCFStressVector,
    Vector& rULCFStressVector
)
{
    auto& r_material_properties = rValues.GetMaterialProperties();
    const auto it_cl_begin = r_material_properties.GetSubProperties().begin();
    const auto& r_props_HCF_cl = *(it_cl_begin);
    const auto& r_props_ULCF_cl = *(it_cl_begin + 1);

    ConstitutiveLaw::Parameters values_HCF  = rValues;
    ConstitutiveLaw::Parameters values_ULCF = rValues;

    values_HCF.SetStrainVector(rHCFStrainVector);
    values_ULCF.SetStrainVector(rULCFStrainVector);

    // Integrate Stress of the HCF part
    values_HCF.SetMaterialProperties(r_props_HCF_cl);
    this->CalculateMaterialResponseHCFModel(values_HCF);
    // mpHCFConstitutiveLaw->CalculateMaterialResponseCauchy(values_HCF);
    rHCFStressVector = values_HCF.GetStressVector();
    KRATOS_WATCH(rHCFStressVector)

    // Integrate Stress of the UÑCF part
    values_ULCF.SetMaterialProperties(r_props_ULCF_cl);
    this->CalculateMaterialResponseULCFModel(values_ULCF);
    // mpULCFConstitutiveLaw->CalculateMaterialResponseCauchy(values_ULCF);
    rULCFStressVector = values_ULCF.GetStressVector();
    KRATOS_WATCH(rULCFStressVector)
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateMaterialResponseHCFModel(
        ConstitutiveLaw::Parameters& values_HCF
)
{
    Vector& integrated_stress_vector = values_HCF.GetStressVector();
    array_1d<double, VoigtSize> auxiliar_integrated_stress_vector = integrated_stress_vector;
    Matrix& r_tangent_tensor = values_HCF.GetConstitutiveMatrix(); // todo modify after integration
    const Flags& r_constitutive_law_options = values_HCF.GetOptions();
    Matrix& r_constitutive_matrix = values_HCF.GetConstitutiveMatrix();
    mpHCFConstitutiveLaw->CalculateValue(values_HCF, CONSTITUTIVE_MATRIX, r_constitutive_matrix);

    Vector& r_strain_vector = values_HCF.GetStrainVector();
    this->template AddInitialStrainVectorContribution<Vector>(r_strain_vector);

    // Converged values
    double hcf_threshold = this->GetHCFThreshold();
    double damage = this->GetDamage();

    // S0 = C:(E-E0) + S0
    array_1d<double, VoigtSize> predictive_stress_vector = prod(r_constitutive_matrix, r_strain_vector);
    this->template AddInitialStressVectorContribution<array_1d<double, VoigtSize>>(predictive_stress_vector);

    // Initialize Plastic Parameters
    double uniaxial_stress;
    TConstLawIntegratorType::YieldSurfaceType::CalculateEquivalentStress(predictive_stress_vector, r_strain_vector, uniaxial_stress, values_HCF);

    const double F = uniaxial_stress - hcf_threshold;

    if (F <= 0.0) { // Elastic case
        noalias(auxiliar_integrated_stress_vector) = (1.0 - damage) * predictive_stress_vector;
		noalias(integrated_stress_vector) = auxiliar_integrated_stress_vector;

        if (r_constitutive_law_options.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR)) {
            noalias(r_tangent_tensor) = (1.0 - damage) * r_constitutive_matrix;
        }
    } else { // Damage case
        const double characteristic_length = AdvancedConstitutiveLawUtilities<VoigtSize>::CalculateCharacteristicLength(values_HCF.GetElementGeometry());
        // This routine updates the PredictiveStress to verify the yield surf
        TConstLawIntegratorType::IntegrateStressVector(
            predictive_stress_vector,
            uniaxial_stress, damage,
            hcf_threshold, values_HCF,
            characteristic_length);

        // Updated Values
        noalias(auxiliar_integrated_stress_vector) = predictive_stress_vector;

        if (r_constitutive_law_options.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR)) {
            this->CalculateTangentTensor(values_HCF);
        }
        noalias(integrated_stress_vector) = auxiliar_integrated_stress_vector;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateMaterialResponseULCFModel(
        ConstitutiveLaw::Parameters& values_ULCF
)
{
    // Auxiliar values
    const Flags& r_constitutive_law_options = values_ULCF.GetOptions();
    // We get the strain vector
    Vector& r_strain_vector = values_ULCF.GetStrainVector();
    // We get the constitutive tensor
    Matrix& r_constitutive_matrix = values_ULCF.GetConstitutiveMatrix();

    // We check the current step and NL iteration
    const ProcessInfo& r_current_process_info = values_ULCF.GetProcessInfo();
    const bool first_computation = (r_current_process_info[NL_ITERATION_NUMBER] == 1 && r_current_process_info[STEP] == 1) ? true : false;

    //NOTE: SINCE THE ELEMENT IS IN SMALL STRAINS WE CAN USE ANY STRAIN MEASURE. HERE EMPLOYING THE CAUCHY_GREEN
    if (first_computation) { // First computation always pure elastic for elemements not providing the strain
        if (r_constitutive_law_options.IsNot( ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN ) ) {
            BaseType::CalculateCauchyGreenStrain( values_ULCF, r_strain_vector);
        }
        this->template AddInitialStrainVectorContribution<Vector>(r_strain_vector);
        if (r_constitutive_law_options.Is( ConstitutiveLaw::COMPUTE_STRESS) ||
            r_constitutive_law_options.Is( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR)) {
            Vector& r_stress_vector = values_ULCF.GetStressVector();
            if (r_constitutive_law_options.Is( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR)) {
                BaseType::CalculateElasticMatrix( r_constitutive_matrix, values_ULCF);
                noalias(r_stress_vector) = prod( r_constitutive_matrix, r_strain_vector);
                this->template AddInitialStressVectorContribution<Vector>(r_stress_vector);
            } else {
                BaseType::CalculatePK2Stress( r_strain_vector, r_stress_vector, values_ULCF);
                this->template AddInitialStressVectorContribution<Vector>(r_stress_vector);
            }
        }
    } else { // We check for plasticity
        // Integrate Stress plasticity
        Vector& r_integrated_stress_vector = values_ULCF.GetStressVector();
        const double characteristic_length = AdvancedConstitutiveLawUtilities<VoigtSize>::CalculateCharacteristicLength(values_ULCF.GetElementGeometry());

        if (r_constitutive_law_options.IsNot( ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN)) {
            BaseType::CalculateCauchyGreenStrain( values_ULCF, r_strain_vector);
        }

        this->template AddInitialStrainVectorContribution<Vector>(r_strain_vector);

        // We compute the stress or the constitutive matrix
        if (r_constitutive_law_options.Is( ConstitutiveLaw::COMPUTE_STRESS) ||
            r_constitutive_law_options.Is( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR)) {

            // We get some variables
            double ulcf_threshold = this->GetULCFThreshold();
            double plastic_dissipation = this->GetPlasticDissipation();
            Vector plastic_strain = this->GetPlasticStrain();

            array_1d<double, VoigtSize> predictive_stress_vector;
            if (r_constitutive_law_options.Is( ConstitutiveLaw::U_P_LAW)) {
                noalias(predictive_stress_vector) = values_ULCF.GetStressVector();
            } else {
                // S0 = Elastic stress with strain (E-Ep) + S0
                Vector aux_stress = ZeroVector(VoigtSize);
                BaseType::CalculatePK2Stress(r_strain_vector - plastic_strain, aux_stress, values_ULCF);
                this->template AddInitialStressVectorContribution<Vector>(aux_stress);
                noalias(predictive_stress_vector) = aux_stress;
            }

            // Initialize Plastic Parameters
            double uniaxial_stress = 0.0, plastic_denominator = 0.0;
            array_1d<double, VoigtSize> f_flux = ZeroVector(VoigtSize); // DF/DS
            array_1d<double, VoigtSize> g_flux = ZeroVector(VoigtSize); // DG/DS
            array_1d<double, VoigtSize> plastic_strain_increment = ZeroVector(VoigtSize);

            // Elastic Matrix
            BaseType::CalculateElasticMatrix(r_constitutive_matrix, values_ULCF);

            // Compute the plastic parameters
            const double F = GenericConstitutiveLawIntegratorPlasticity<TConstLawIntegratorType::YieldSurfaceType>::CalculatePlasticParameters(
                predictive_stress_vector, r_strain_vector, uniaxial_stress,
                ulcf_threshold, plastic_denominator, f_flux, g_flux,
                plastic_dissipation, plastic_strain_increment,
                r_constitutive_matrix, values_ULCF, characteristic_length,
                plastic_strain);

            if (F <= std::abs(1.0e-4 * ulcf_threshold)) { // Elastic case
                noalias(r_integrated_stress_vector) = predictive_stress_vector;
            } else { // Plastic case
                // While loop backward euler
                /* Inside "IntegrateStressVector" the predictive_stress_vector is updated to verify the yield criterion */
                GenericConstitutiveLawIntegratorPlasticity<TConstLawIntegratorType::YieldSurfaceType>::IntegrateStressVector(
                    predictive_stress_vector, r_strain_vector, uniaxial_stress,
                    ulcf_threshold, plastic_denominator, f_flux, g_flux,
                    plastic_dissipation, plastic_strain_increment,
                    r_constitutive_matrix, plastic_strain, values_ULCF,
                    characteristic_length);
                noalias(r_integrated_stress_vector) = predictive_stress_vector;

                if (r_constitutive_law_options.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR)) {
                    this->CalculateTangentTensor(values_ULCF); // this modifies the ConstitutiveMatrix
                } else {
                    BaseType::CalculateElasticMatrix( r_constitutive_matrix, values_ULCF);
                }
            }
        }
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::FinalizeSolutionStep(
    const Properties& rMaterialProperties,
    const GeometryType& rElementGeometry,
    const Vector& rShapeFunctionsValues,
    const ProcessInfo& rCurrentProcessInfo)
{
    // Deprecated
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::FinalizeMaterialResponsePK1(ConstitutiveLaw::Parameters& rValues)
{
    this->FinalizeMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::FinalizeMaterialResponsePK2(ConstitutiveLaw::Parameters& rValues)
{
    this->FinalizeMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::FinalizeMaterialResponseKirchhoff(ConstitutiveLaw::Parameters& rValues)
{
    this->FinalizeMaterialResponseCauchy(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::FinalizeMaterialResponseCauchy(ConstitutiveLaw::Parameters& rValues)
{
    const Vector& r_strain_vector = rValues.GetStrainVector();

    // Recalculation to obtain the serial_strain_matrix and store the value
    const SizeType voigt_size = GetStrainSize();

    // Get Values to compute the constitutive law:
    Flags& r_flags = rValues.GetOptions();

    // Previous flags saved
    const bool flag_strain = r_flags.Is(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN);
    const bool flag_const_tensor = r_flags.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR);
    const bool flag_stress = r_flags.Is(ConstitutiveLaw::COMPUTE_STRESS);

    const Properties& r_material_properties = rValues.GetMaterialProperties();

    if (r_flags.Is(ConstitutiveLaw::COMPUTE_STRESS)) {
        // Set new flags
        r_flags.Set(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN, true);
        r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, false);
        r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, true);

        // Total strain vector
        Vector& r_strain_vector = rValues.GetStrainVector();

        r_flags.Set(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN, flag_strain);
        r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor);
        r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, flag_stress);

        // We call the FinalizeMaterialResponse of the HCF and ULCF CL
        auto& r_material_properties = rValues.GetMaterialProperties();
        const auto it_cl_begin = r_material_properties.GetSubProperties().begin();
        const auto& r_props_HCF_cl = *(it_cl_begin);
        const auto& r_props_ULCF_cl = *(it_cl_begin + 1);

        ConstitutiveLaw::Parameters values_HCF  = rValues;
        ConstitutiveLaw::Parameters values_ULCF = rValues;

        values_HCF.SetMaterialProperties(r_props_HCF_cl);
        values_ULCF.SetMaterialProperties(r_props_ULCF_cl);

        values_HCF.SetStrainVector(r_strain_vector);
        values_ULCF.SetStrainVector(r_strain_vector);

        this->FinalizeMaterialResponseHCFModel(values_HCF);
        this->FinalizeMaterialResponseULCFModel(values_ULCF);
        // mpHCFConstitutiveLaw->FinalizeMaterialResponseCauchy(values_HCF);
        // mpULCFConstitutiveLaw->FinalizeMaterialResponseCauchy(values_ULCF);

        // mpULCFConstitutiveLaw::ConstLawIntegratorType::YieldSurfaceTypePlas;

        Vector high_cycle_fatigue_stress_vector, ultra_low_cycle_fatigue_stress_vector;

        high_cycle_fatigue_stress_vector = values_HCF.GetStressVector();
        ultra_low_cycle_fatigue_stress_vector = values_ULCF.GetStressVector();

        Vector& r_integrated_stress_vector = rValues.GetStressVector();
        noalias(r_integrated_stress_vector) = mHCFVolumetricParticipation * high_cycle_fatigue_stress_vector
                                     + (1.0 - mHCFVolumetricParticipation) * ultra_low_cycle_fatigue_stress_vector;

        double uniaxial_stress;
        TConstLawIntegratorType::YieldSurfaceType::CalculateEquivalentStress(r_integrated_stress_vector, r_strain_vector, uniaxial_stress, rValues);

        double sign_factor = HighCycleFatigueLawIntegrator<6>::CalculateTensionCompressionFactor(r_integrated_stress_vector);
        uniaxial_stress *= sign_factor;
        double max_stress = mMaxStress;
        double min_stress = mMinStress;
        bool max_indicator = mMaxDetected;
        bool min_indicator = mMinDetected;

        HighCycleFatigueLawIntegrator<6>::CalculateMaximumAndMinimumStresses(
            uniaxial_stress,
            max_stress,
            min_stress,
            mPreviousStresses,
            max_indicator,
            min_indicator);
        mMaxStress = max_stress;
        mMinStress = min_stress;
        mMaxDetected = max_indicator;
        mMinDetected = min_indicator;

        Vector previous_stresses = ZeroVector(2);
        const Vector& r_aux_stresses = mPreviousStresses;
        previous_stresses[1] = uniaxial_stress;
        previous_stresses[0] = r_aux_stresses[1];
        mPreviousStresses = previous_stresses;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::FinalizeMaterialResponseHCFModel(
        ConstitutiveLaw::Parameters& values_HCF
)
{
    // Integrate Stress Damage
    const Flags& r_constitutive_law_options = values_HCF.GetOptions();

    // We get the strain vector
    Vector& r_strain_vector = values_HCF.GetStrainVector();

    // Elastic Matrix
    Matrix& r_constitutive_matrix = values_HCF.GetConstitutiveMatrix();
    this->CalculateValue(values_HCF, CONSTITUTIVE_MATRIX, r_constitutive_matrix);

    if (r_constitutive_law_options.IsNot(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN)) {
        BaseType::CalculateCauchyGreenStrain( values_HCF, r_strain_vector);
    }

    this->template AddInitialStrainVectorContribution<Vector>(r_strain_vector);

    // Converged values
    double hcf_threshold = this->GetHCFThreshold();
    double damage = this->GetDamage();

    // S0 = C:(E-E0) + S0
    array_1d<double, VoigtSize> predictive_stress_vector = prod(r_constitutive_matrix, r_strain_vector);
    this->template AddInitialStressVectorContribution<array_1d<double, VoigtSize>>(predictive_stress_vector);

    // Initialize Plastic Parameters
    double uniaxial_stress;
    TConstLawIntegratorType::YieldSurfaceType::CalculateEquivalentStress(predictive_stress_vector, r_strain_vector, uniaxial_stress, values_HCF);

    const double F = uniaxial_stress - hcf_threshold;

    if (F >= 0.0) { // Plastic case
        const double characteristic_length = AdvancedConstitutiveLawUtilities<VoigtSize>::CalculateCharacteristicLength(values_HCF.GetElementGeometry());
        // This routine updates the PredictiveStress to verify the yield surf
        TConstLawIntegratorType::IntegrateStressVector(
            predictive_stress_vector,
            uniaxial_stress, damage,
            hcf_threshold, values_HCF,
            characteristic_length);
        mDamage = damage;
        mHCFThreshold = uniaxial_stress;

    } else {
        predictive_stress_vector *= (1.0 - mDamage);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::FinalizeMaterialResponseULCFModel(
        ConstitutiveLaw::Parameters& values_ULCF
)
{
    // Auxiliar values
    const Flags& r_constitutive_law_options = values_ULCF.GetOptions();

    // We get the strain vector
    Vector& r_strain_vector = values_ULCF.GetStrainVector();

    // We get the constitutive tensor
    Matrix& r_constitutive_matrix = values_ULCF.GetConstitutiveMatrix();

    // Integrate Stress plasticity
    const double characteristic_length = AdvancedConstitutiveLawUtilities<VoigtSize>::CalculateCharacteristicLength(values_ULCF.GetElementGeometry());

    if (r_constitutive_law_options.IsNot( ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN)) {
        BaseType::CalculateCauchyGreenStrain( values_ULCF, r_strain_vector);
    }

    this->template AddInitialStrainVectorContribution<Vector>(r_strain_vector);

    // We compute the stress
    // Elastic Matrix
    this->CalculateElasticMatrix(r_constitutive_matrix, values_ULCF);

    // We get some variables
    double ulcf_threshold = this->GetULCFThreshold();
    double plastic_dissipation = this->GetPlasticDissipation();
    Vector plastic_strain = this->GetPlasticStrain();

    array_1d<double, VoigtSize> predictive_stress_vector;
    if (r_constitutive_law_options.Is( ConstitutiveLaw::U_P_LAW)) {
        noalias(predictive_stress_vector) = values_ULCF.GetStressVector();
    } else {
        // Spred = r_constitutive_matrix:(E-Ep) + S0
        noalias(predictive_stress_vector) = prod(r_constitutive_matrix, r_strain_vector - plastic_strain);
        this->template AddInitialStressVectorContribution<array_1d<double, VoigtSize>>(predictive_stress_vector);
    }

    // Initialize Plastic Parameters
    double uniaxial_stress = 0.0, plastic_denominator = 0.0;
    array_1d<double, VoigtSize> f_flux = ZeroVector(VoigtSize); // DF/DS
    array_1d<double, VoigtSize> g_flux = ZeroVector(VoigtSize); // DG/DS
    array_1d<double, VoigtSize> plastic_strain_increment = ZeroVector(VoigtSize);

    const double F = GenericConstitutiveLawIntegratorPlasticity<TConstLawIntegratorType::YieldSurfaceType>::CalculatePlasticParameters(
        predictive_stress_vector, r_strain_vector, uniaxial_stress,
        ulcf_threshold, plastic_denominator, f_flux, g_flux,
        plastic_dissipation, plastic_strain_increment,
        r_constitutive_matrix, values_ULCF, characteristic_length,
        plastic_strain);

    if (F > std::abs(1.0e-4 * ulcf_threshold)) { // Plastic case
        // While loop backward euler
        /* Inside "IntegrateStressVector" the predictive_stress_vector is updated to verify the yield criterion */
        GenericConstitutiveLawIntegratorPlasticity<TConstLawIntegratorType::YieldSurfaceType>::IntegrateStressVector(
            predictive_stress_vector, r_strain_vector, uniaxial_stress,
            ulcf_threshold, plastic_denominator, f_flux, g_flux,
            plastic_dissipation, plastic_strain_increment,
            r_constitutive_matrix, plastic_strain, values_ULCF,
            characteristic_length);
        BaseType::CalculateElasticMatrix( r_constitutive_matrix, values_ULCF);
    }

    mPlasticDissipation = plastic_dissipation;
    mPlasticStrain = plastic_strain;
    mULCFThreshold = ulcf_threshold;
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
double& UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::GetValue(
    const Variable<double>& rThisVariable,
    double& rValue
    )
{
    if (mpHCFConstitutiveLaw->Has(rThisVariable)) {
        return mpHCFConstitutiveLaw->GetValue(rThisVariable, rValue);
    } else if (mpULCFConstitutiveLaw->Has(rThisVariable)) {
        return mpULCFConstitutiveLaw->GetValue(rThisVariable, rValue);
    } else if (rThisVariable == WOHLER_STRESS) {
        rValue = mWohlerStress;
    } else if (rThisVariable == CYCLES_TO_FAILURE) {
        rValue = mCyclesToFailure;
    } else if (rThisVariable == REVERSION_FACTOR_RELATIVE_ERROR) {
        rValue = mReversionFactorRelativeError;
    } else if (rThisVariable == MAX_STRESS_RELATIVE_ERROR) {
        rValue = mMaxStressRelativeError;
    } else if (rThisVariable == MAX_STRESS) {
        rValue = mMaxStress;
    } else if (rThisVariable == THRESHOLD_STRESS) {
        rValue = mThresholdStress;
    } else if (rThisVariable == FATIGUE_REDUCTION_FACTOR) {
        rValue = mFatigueReductionFactor;
    } else {
        return rValue;
    }
    return rValue;

}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
Vector& UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::GetValue(
    const Variable<Vector>& rThisVariable,
    Vector& rValue
    )
{
    if (mpHCFConstitutiveLaw->Has(rThisVariable)) {
        return mpHCFConstitutiveLaw->GetValue(rThisVariable, rValue);
    } else if (mpULCFConstitutiveLaw->Has(rThisVariable)) {
        return mpULCFConstitutiveLaw->GetValue(rThisVariable, rValue);
    } else {
        return rValue;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
Matrix& UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::GetValue(
    const Variable<Matrix>& rThisVariable,
    Matrix& rValue
    )
{
    if (mpHCFConstitutiveLaw->Has(rThisVariable)) {
        return mpHCFConstitutiveLaw->GetValue(rThisVariable, rValue);
    } else if (mpULCFConstitutiveLaw->Has(rThisVariable)) {
        return mpULCFConstitutiveLaw->GetValue(rThisVariable, rValue);
    } else {
        return rValue;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
bool UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::Has(const Variable<bool>& rThisVariable)
{
    if (mpHCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else if (mpULCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else {
        return false;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
bool UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::Has(const Variable<double>& rThisVariable)
{
    if (mpHCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else if (mpULCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else if (rThisVariable == FATIGUE_REDUCTION_FACTOR) {
        return true;
    } else {
        return false;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
bool UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::Has(const Variable<Vector>& rThisVariable)
{
    if (mpHCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else if (mpULCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else {
        return false;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
bool UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::Has(const Variable<Matrix>& rThisVariable)
{
    if (mpHCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else if (mpULCFConstitutiveLaw->Has(rThisVariable)) {
        return true;
    } else {
        return false;
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
double& UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<double>& rThisVariable,
    double& rValue)
{
    // const Vector& r_strain_vector = rParameterValues.GetStrainVector();

    // Recalculation to obtain the serial_strain_matrix and store the value
    // const SizeType voigt_size = GetStrainSize();

    // Get Values to compute the constitutive law:
    // Flags& r_flags = rParameterValues.GetOptions();

    // Previous flags saved
    // const bool flag_strain = r_flags.Is(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN);
    // const bool flag_const_tensor = r_flags.Is(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR);
    // const bool flag_stress = r_flags.Is(ConstitutiveLaw::COMPUTE_STRESS);


    auto& r_material_properties = rParameterValues.GetMaterialProperties();
    const auto it_cl_begin = r_material_properties.GetSubProperties().begin();
    const auto& r_props_HCF_cl = *(it_cl_begin);
    const auto& r_props_ULCF_cl = *(it_cl_begin + 1);
    ConstitutiveLaw::Parameters values_HCF  = rParameterValues;
    ConstitutiveLaw::Parameters values_ULCF = rParameterValues;
    values_HCF.SetMaterialProperties(r_props_HCF_cl);
    values_ULCF.SetMaterialProperties(r_props_ULCF_cl);

    if (rThisVariable == UNIAXIAL_STRESS) {
        // Get Values to compute the constitutive law:
        // Flags& r_flags = rParameterValues.GetOptions();

        // Previous flags saved
        // const bool flag_const_tensor = r_flags.Is( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR );
        // const bool flag_stress = r_flags.Is( ConstitutiveLaw::COMPUTE_STRESS );

        // r_flags.Set( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, false );
        // r_flags.Set( ConstitutiveLaw::COMPUTE_STRESS, true );

        // Calculate the stress vector
        CalculateMaterialResponseCauchy(rParameterValues);

        // r_flags.Set(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN, flag_strain);
        // r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor);
        // r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, flag_stress);

        Vector& r_strain_vector = rParameterValues.GetStrainVector();
        Vector& r_integrated_stress_vector = rParameterValues.GetStressVector();
        TConstLawIntegratorType::YieldSurfaceType::CalculateEquivalentStress( r_integrated_stress_vector, r_strain_vector, rValue, rParameterValues);

        return rValue;

    } else if (rThisVariable == UNIAXIAL_STRESS_HCF) {
        this->CalculateMaterialResponseHCFModel(values_HCF);
        Vector& r_strain_vector = values_HCF.GetStrainVector();
        Vector& r_integrated_stress_vector = values_HCF.GetStressVector();
        TConstLawIntegratorType::YieldSurfaceType::CalculateEquivalentStress( r_integrated_stress_vector, r_strain_vector, rValue, values_HCF);
        return rValue;
    } else if (rThisVariable == UNIAXIAL_STRESS_ULCF) {
        this->CalculateMaterialResponseULCFModel(values_ULCF);
        Vector& r_strain_vector = rParameterValues.GetStrainVector();
        Vector& r_integrated_stress_vector = values_ULCF.GetStressVector();
        TConstLawIntegratorType::YieldSurfaceType::CalculateEquivalentStress( r_integrated_stress_vector, r_strain_vector, rValue, values_ULCF);
        return rValue;
    } else {
        return this->GetValue(rThisVariable, rValue);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
Vector& UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<Vector>& rThisVariable,
    Vector& rValue)
{
    return this->GetValue(rThisVariable, rValue);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::InitializeMaterial(
    const Properties& rMaterialProperties,
    const GeometryType& rElementGeometry,
    const Vector& rShapeFunctionsValues)
{
    const auto it_cl_begin = rMaterialProperties.GetSubProperties().begin();
    const auto r_props_HCF_cl = *(it_cl_begin);
    const auto r_props_ULCF_cl  = *(it_cl_begin + 1);

    KRATOS_ERROR_IF_NOT(r_props_HCF_cl.Has(CONSTITUTIVE_LAW)) << "No constitutive law set" << std::endl;
    KRATOS_ERROR_IF_NOT(r_props_ULCF_cl.Has(CONSTITUTIVE_LAW))  << "No constitutive law set" << std::endl;

    mpHCFConstitutiveLaw = r_props_HCF_cl[CONSTITUTIVE_LAW]->Clone();
    mpULCFConstitutiveLaw  = r_props_ULCF_cl[CONSTITUTIVE_LAW]->Clone();
    mpHCFConstitutiveLaw->InitializeMaterial(r_props_HCF_cl, rElementGeometry, rShapeFunctionsValues);
    mpULCFConstitutiveLaw ->InitializeMaterial(r_props_ULCF_cl, rElementGeometry, rShapeFunctionsValues);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
Matrix& UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<Matrix>& rThisVariable,
    Matrix& rValue
    )
{
    // We do some special operations for constitutive matrices
    if (rThisVariable == CONSTITUTIVE_MATRIX ||
        rThisVariable == CONSTITUTIVE_MATRIX_PK2 ||
        rThisVariable == CONSTITUTIVE_MATRIX_KIRCHHOFF) {
        // Get Values to compute the constitutive law:
        Flags& r_flags = rParameterValues.GetOptions();

        // Previous flags saved
        const bool flag_strain = r_flags.Is( ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN );
        const bool flag_const_tensor = r_flags.Is( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR );
        const bool flag_stress = r_flags.Is( ConstitutiveLaw::COMPUTE_STRESS );

        r_flags.Set(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN, false);
        r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, true);
        r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, false);

         // We compute the constitutive matrix
        if (rThisVariable == CONSTITUTIVE_MATRIX) {
            this->CalculateMaterialResponse(rParameterValues, this->GetStressMeasure());
        } else if (rThisVariable == CONSTITUTIVE_MATRIX_PK2) {
            this->CalculateMaterialResponsePK2(rParameterValues);
        } else if (rThisVariable == CONSTITUTIVE_MATRIX_KIRCHHOFF) {
            this->CalculateMaterialResponsePK2(rParameterValues);
        }

        noalias(rValue) = rParameterValues.GetConstitutiveMatrix();

        // Previous flags restored
        r_flags.Set(ConstitutiveLaw::USE_ELEMENT_PROVIDED_STRAIN, flag_strain);
        r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor);
        r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, flag_stress);
    } else if (rThisVariable == DEFORMATION_GRADIENT) { // TODO: Make in the future modifications for take into account different layers combinations
        noalias(rValue) = rParameterValues.GetDeformationGradientF();
    } else if (rThisVariable == CAUCHY_STRESS_TENSOR) {
        // Get Values to compute the constitutive law:
        Flags& r_flags = rParameterValues.GetOptions();

        // Previous flags saved
        const bool flag_const_tensor = r_flags.Is( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR);
        const bool flag_stress = r_flags.Is( ConstitutiveLaw::COMPUTE_STRESS);

        r_flags.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, false);
        r_flags.Set(ConstitutiveLaw::COMPUTE_STRESS, true);

        // We compute the stress
        this->CalculateMaterialResponseCauchy(rParameterValues);
        rValue = MathUtils<double>::StressVectorToTensor(rParameterValues.GetStressVector());

        // Previous flags restored
        r_flags.Set( ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor );
        r_flags.Set( ConstitutiveLaw::COMPUTE_STRESS, flag_stress );
        return rValue;
    } else {
        Matrix aux_value;
        Properties material_properties  = rParameterValues.GetMaterialProperties();
        Properties& r_prop = material_properties.GetSubProperties(0);

        rValue.clear();
        rParameterValues.SetMaterialProperties(r_prop);
        mpHCFConstitutiveLaw->CalculateValue(rParameterValues, rThisVariable, aux_value);
        noalias(rValue) += (1.0 - mHCFVolumetricParticipation) * aux_value;

        r_prop = material_properties.GetSubProperties(1);
        rParameterValues.SetMaterialProperties(r_prop);
        mpHCFConstitutiveLaw->CalculateValue(rParameterValues, rThisVariable, aux_value);
        noalias(rValue) += (1.0 - mHCFVolumetricParticipation) * aux_value;

        // Reset properties
        rParameterValues.SetMaterialProperties(material_properties);
    }
    return(rValue);
}

/***********************************************************************************/
/***********************************************************************************/

template <class TConstLawIntegratorType>
void UnifiedFatigueRuleOfMixturesLaw<TConstLawIntegratorType>::CalculateTangentTensor(ConstitutiveLaw::Parameters& rValues)
{
    const Properties& r_material_properties = rValues.GetMaterialProperties();

    const bool consider_perturbation_threshold = r_material_properties.Has(CONSIDER_PERTURBATION_THRESHOLD) ? r_material_properties[CONSIDER_PERTURBATION_THRESHOLD] : true;
    const TangentOperatorEstimation tangent_operator_estimation = r_material_properties.Has(TANGENT_OPERATOR_ESTIMATION) ? static_cast<TangentOperatorEstimation>(r_material_properties[TANGENT_OPERATOR_ESTIMATION]) : TangentOperatorEstimation::SecondOrderPerturbation;

    if (tangent_operator_estimation == TangentOperatorEstimation::Analytic) {
        KRATOS_ERROR << "Analytic solution not available" << std::endl;
    } else if (tangent_operator_estimation == TangentOperatorEstimation::FirstOrderPerturbation) {
        // Calculates the Tangent Constitutive Tensor by perturbation (first order)
        TangentOperatorCalculatorUtility::CalculateTangentTensor(rValues, this, ConstitutiveLaw::StressMeasure_Cauchy, consider_perturbation_threshold, 1);
    } else if (tangent_operator_estimation == TangentOperatorEstimation::SecondOrderPerturbation) {
        // Calculates the Tangent Constitutive Tensor by perturbation (second order)
        TangentOperatorCalculatorUtility::CalculateTangentTensor(rValues, this, ConstitutiveLaw::StressMeasure_Cauchy, consider_perturbation_threshold, 2);
    }
}
/***********************************************************************************/
/***********************************************************************************/
template class UnifiedFatigueRuleOfMixturesLaw<GenericConstitutiveLawIntegratorDamage<VonMisesYieldSurface<VonMisesPlasticPotential<6>>>>;

} // namespace Kratos
