// KRATOS  ___|  |                   |                   |
//       \___ \  __|  __| |   |  __| __| |   |  __| _` | |
//             | |   |    |   | (    |   |   | |   (   | |
//       _____/ \__|_|   \__,_|\___|\__|\__,_|_|  \__,_|_| MECHANICS
//
//  License:         BSD License
//                   license: particle_mechanics_application/license.txt
//
//  Main authors:    Philip Kalkbrenner
//					 Massimo Petracca
//                   Alejandro Cornejo
//
//

// System includes

// External includes

// Project includes
#include "plane_stress_d_plus_d_minus_damage_masonry_2d.h"
#include "includes/model_part.h"
#include "particle_mechanics_application_variables.h"
#include "custom_utilities/constitutive_law_utilities.h"

//#define OPTIMIZE_CHARACTERISTIC_LENGTH
#define HEAVISIDE(X) ( X >= 0.0 ? 1.0 : 0.0)
#define MACAULAY(X)  ( X >= 0.0 ? X : 0.0)
#define PROJECTION_OPERATOR_CERVERA_2003
//#define PROJECTION_OPERATOR_CERVERA_2017

namespace Kratos
{
/***********************************************************************************/
/***********************************************************************************/
MPMDamageDPlusDMinusMasonry2DLaw::MPMDamageDPlusDMinusMasonry2DLaw()
	: ConstitutiveLaw()
{
}
/***********************************************************************************/
/***********************************************************************************/
ConstitutiveLaw::Pointer MPMDamageDPlusDMinusMasonry2DLaw::Clone() const
{
	return ConstitutiveLaw::Pointer( new MPMDamageDPlusDMinusMasonry2DLaw() );
}
/***********************************************************************************/
/***********************************************************************************/
bool MPMDamageDPlusDMinusMasonry2DLaw::Has(
	const Variable<double>& rThisVariable)
{
	if(rThisVariable == DAMAGE_TENSION)
		return true;
	if(rThisVariable == UNIAXIAL_STRESS_TENSION)
		return true;
	if(rThisVariable == THRESHOLD_TENSION)
		return true;
	if(rThisVariable == DAMAGE_COMPRESSION)
		return true;
	if(rThisVariable == UNIAXIAL_STRESS_COMPRESSION)
		return true;
	if(rThisVariable == THRESHOLD_COMPRESSION)
		return true;
	return false;
}
/***********************************************************************************/
/***********************************************************************************/

bool MPMDamageDPlusDMinusMasonry2DLaw::Has(
	const Variable<Vector>& rThisVariable)
{
	if(rThisVariable == INTERNAL_VARIABLES)
		return true;
	return false;
}
/***********************************************************************************/
/***********************************************************************************/
bool MPMDamageDPlusDMinusMasonry2DLaw::Has(
	const Variable<Matrix>& rThisVariable)
{
	return false;
}
/***********************************************************************************/
/***********************************************************************************/
bool MPMDamageDPlusDMinusMasonry2DLaw::Has(
	const Variable<array_1d<double, 3 > >& rThisVariable)
{
	return false;
}
/***********************************************************************************/
/***********************************************************************************/
bool MPMDamageDPlusDMinusMasonry2DLaw::Has(
	const Variable<array_1d<double, 6 > >& rThisVariable)
{
	return false;
}
/***********************************************************************************/
/***********************************************************************************/
double& MPMDamageDPlusDMinusMasonry2DLaw::GetValue(
	const Variable<double>& rThisVariable,
	double& rValue)
{
	rValue = 0.0;
	if(rThisVariable == DAMAGE_TENSION)
		rValue = DamageParameterTensionOutput;
	else if(rThisVariable == DAMAGE_COMPRESSION)
		rValue = DamageParameterCompressionOutput;
	else if(rThisVariable == UNIAXIAL_STRESS_TENSION)
		rValue = UniaxialStressTension;
	else if(rThisVariable == UNIAXIAL_STRESS_COMPRESSION)
		rValue = UniaxialStressCompression;
	else if(rThisVariable == THRESHOLD_TENSION)
		rValue = ThresholdTension;
	else if(rThisVariable == THRESHOLD_COMPRESSION)
		rValue = ThresholdCompression;
	else if(rThisVariable == EQ_STRAIN_RATE)
		rValue = mStrainRate;
	return rValue;
}
/***********************************************************************************/
/***********************************************************************************/
Vector& MPMDamageDPlusDMinusMasonry2DLaw::GetValue(
	const Variable<Vector>& rThisVariable,
	Vector& rValue)
{
	return rValue;
}
/***********************************************************************************/
/***********************************************************************************/
Matrix& MPMDamageDPlusDMinusMasonry2DLaw::GetValue(
	const Variable<Matrix>& rThisVariable,
	Matrix& rValue)
{
	return rValue;
}
/***********************************************************************************/
/***********************************************************************************/
array_1d<double, 3 > & MPMDamageDPlusDMinusMasonry2DLaw::GetValue(
	const Variable<array_1d<double, 3 > >& rVariable,
	array_1d<double, 3 > & rValue)
{
	return rValue;
}
/***********************************************************************************/
/***********************************************************************************/
array_1d<double, 6 > & MPMDamageDPlusDMinusMasonry2DLaw::GetValue(
	const Variable<array_1d<double, 6 > >& rVariable,
	array_1d<double, 6 > & rValue)
{
	return rValue;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::SetValue(
	const Variable<double>& rVariable,
	const double& rValue,
	const ProcessInfo& rCurrentProcessInfo)
{
	if(rVariable == DAMAGE_TENSION)
		DamageParameterTension = rValue;
	else if(rVariable == DAMAGE_COMPRESSION)
		DamageParameterCompression = rValue;
	else if(rVariable == UNIAXIAL_STRESS_TENSION)
		UniaxialStressTension = rValue;
	else if(rVariable == UNIAXIAL_STRESS_COMPRESSION)
		UniaxialStressCompression = rValue;
	else if(rVariable == THRESHOLD_TENSION)
		ThresholdTension = rValue;
	else if(rVariable == THRESHOLD_COMPRESSION)
		ThresholdCompression = rValue;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::SetValue(
	const Variable<Vector >& rVariable,
	const Vector& rValue,
	const ProcessInfo& rCurrentProcessInfo)
{
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::SetValue(
	const Variable<Matrix >& rVariable,
	const Matrix& rValue,
	const ProcessInfo& rCurrentProcessInfo)
{
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::SetValue(
	const Variable<array_1d<double, 3 > >& rVariable,
	const array_1d<double, 3 > & rValue,
	const ProcessInfo& rCurrentProcessInfo)
{
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::SetValue(
	const Variable<array_1d<double, 6 > >& rVariable,
	const array_1d<double, 6 > & rValue,
	const ProcessInfo& rCurrentProcessInfo)
{
}
/***********************************************************************************/
/***********************************************************************************/
bool MPMDamageDPlusDMinusMasonry2DLaw::ValidateInput(
	const Properties& rMaterialProperties)
{
	if( !rMaterialProperties.Has(YOUNG_MODULUS) ) 					return false;
	if( !rMaterialProperties.Has(POISSON_RATIO) ) 					return false;
	if( !rMaterialProperties.Has(YIELD_STRESS_TENSION) ) 			return false;
	if( !rMaterialProperties.Has(FRACTURE_ENERGY_TENSION) ) 		return false;
	if( !rMaterialProperties.Has(DAMAGE_ONSET_STRESS_COMPRESSION) ) return false;
	if( !rMaterialProperties.Has(YIELD_STRESS_COMPRESSION) ) 		return false;
	if( !rMaterialProperties.Has(RESIDUAL_STRESS_COMPRESSION) )	 	return false;
	if( !rMaterialProperties.Has(YIELD_STRAIN_COMPRESSION) ) 		return false;
	if( !rMaterialProperties.Has(FRACTURE_ENERGY_COMPRESSION) ) 	return false;
	if( !rMaterialProperties.Has(BIAXIAL_COMPRESSION_MULTIPLIER) ) 	return false;
	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C1_TENSION) ) 	return false;
	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C2_TENSION) ) 	return false;
	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C1_COMPRESSION) ) 	return false;
	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C2_COMPRESSION) ) 	return false;
	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C1_YOUNGS_MOD) ) 	return false;
	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C2_YOUNGS_MOD) ) 	return false;
	return true;
}
/***********************************************************************************/
/***********************************************************************************/
MPMDamageDPlusDMinusMasonry2DLaw::StrainMeasure MPMDamageDPlusDMinusMasonry2DLaw::GetStrainMeasure()
{
	return ConstitutiveLaw::StrainMeasure_Velocity_Gradient;
}
/***********************************************************************************/
/***********************************************************************************/
MPMDamageDPlusDMinusMasonry2DLaw::StressMeasure MPMDamageDPlusDMinusMasonry2DLaw::GetStressMeasure()
{
	return ConstitutiveLaw::StressMeasure_Cauchy;
}
/***********************************************************************************/
/***********************************************************************************/
bool MPMDamageDPlusDMinusMasonry2DLaw::IsIncremental()
{
	return false;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::InitializeMaterial(
	const Properties& rMaterialProperties,
	const GeometryType& rElementGeometry,
	const Vector& rShapeFunctionsValues)
{
	if(!InitializeDamageLaw){
		ThresholdTension            	= rMaterialProperties[YIELD_STRESS_TENSION];
		CurrentThresholdTension   		= ThresholdTension;
		ThresholdCompression        	= rMaterialProperties[DAMAGE_ONSET_STRESS_COMPRESSION];
		CurrentThresholdCompression 	= ThresholdCompression;
		DamageParameterTension      	= 0.0;
		DamageParameterCompression  	= 0.0;
		UniaxialStressTension			= 0.0;
		UniaxialStressCompression		= 0.0;

		this->ComputeCharacteristicLength(rElementGeometry, rMaterialProperties,
			InitialCharacteristicLength);

		element_center = rElementGeometry.GetGeometryParent(0).Center(); // for debugging - delete

		// Begin IMPLEX Integration - Only if switched on
		if (rMaterialProperties[INTEGRATION_IMPLEX] != 0){
			PreviousThresholdTension 		= ThresholdTension;
			PreviousThresholdCompression 	= ThresholdCompression;
			CurrentDeltaTime 				= 0.0;
			PreviousDeltaTime 				= 0.0;
	    }
		// End IMPLEX Integration

		InitializeDamageLaw    			= true;
	}
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::InitializeMaterialResponsePK2 (
	Parameters& rValues)
{
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::InitializeSolutionStep(
	const Properties& rMaterialProperties,
	const GeometryType& rElementGeometry,
	const Vector& rShapeFunctionsValues,
	const ProcessInfo& rCurrentProcessInfo)
{
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::FinalizeSolutionStep(
	const Properties& rMaterialProperties,
	const GeometryType& rElementGeometry,
	const Vector& rShapeFunctionsValues,
	const ProcessInfo& rCurrentProcessInfo)
{
	// Begin IMPLEX Integration - Only if switched on
	if (rMaterialProperties[INTEGRATION_IMPLEX] != 0){
		ThresholdTension 		= TemporaryImplicitThresholdTension;
		ThresholdCompression 	= TemporaryImplicitThresholdTCompression;

		// move from n to n-1
		PreviousThresholdTension  		= CurrentThresholdTension;
		PreviousThresholdCompression  	= CurrentThresholdCompression;
		PreviousDeltaTime 				= CurrentDeltaTime;
	}
	// End IMPLEX Integration

	// save converged values
	CurrentThresholdTension 		= ThresholdTension;
	CurrentThresholdCompression 	= ThresholdCompression;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateMaterialResponsePK1 (
	Parameters& rValues)
{
	CalculateMaterialResponseCauchy(rValues);
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateMaterialResponsePK2 (
	Parameters& rValues)
{
	CalculateMaterialResponseCauchy(rValues);
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateMaterialResponseKirchhoff (
	Parameters& rValues)
{
	CalculateMaterialResponseCauchy(rValues);
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateMaterialResponseCauchy (
	Parameters& rValues)
{
	const ProcessInfo&  pinfo = rValues.GetProcessInfo();
	const GeometryType& geom  = rValues.GetElementGeometry();
	const Properties&   props = rValues.GetMaterialProperties();

	CalculationData data;

	const Vector& StrainVector   	= rValues.GetStrainVector();

	KRATOS_ERROR_IF_NOT(StrainVector.size() == mStrainOld.size())
		<< "The new and old strain vectors are different sizes!"
		<< "\nStrainVector = " << StrainVector
		<< "\nStrainVectorOld = " << mStrainOld
		<< std::endl;
	const Vector strain_rate_vec = (StrainVector - mStrainOld) / pinfo[DELTA_TIME];
	mStrainRate = std::sqrt(0.5 * inner_prod(strain_rate_vec, strain_rate_vec));
	mStrainOld = Vector(rValues.GetStrainVector()); // only needed for the strain rate calc.

	this->InitializeCalculationData(props, geom, pinfo, data);

	Vector& PredictiveStressVector	= rValues.GetStressVector();

	this->CalculateMaterialResponseInternal(StrainVector, PredictiveStressVector, data, props);

	bool is_damaging_tension = false;
	bool is_damaging_compression = false;
	this->CheckDamageLoadingUnloading(is_damaging_tension, is_damaging_compression);

	// Computation of the Constitutive Tensor
	if (rValues.GetOptions().Is(COMPUTE_CONSTITUTIVE_TENSOR)) {
		KRATOS_ERROR << "NOT IMPLEMENTED!\n\n";
		if(is_damaging_tension || is_damaging_compression) {
			this->CalculateTangentTensor(rValues, StrainVector, PredictiveStressVector, data, props);
		}
		else {
			this->CalculateSecantTensor(rValues, data);
		}
	}
}

/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::FinalizeMaterialResponsePK1 (
	Parameters& rValues)
{
	FinalizeMaterialResponseCauchy(rValues);
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::FinalizeMaterialResponsePK2 (
	Parameters& rValues)
{
	FinalizeMaterialResponseCauchy(rValues);
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::FinalizeMaterialResponseKirchhoff (
	Parameters& rValues)
{
	FinalizeMaterialResponseCauchy(rValues);
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::FinalizeMaterialResponseCauchy (
	Parameters& rValues)
{
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::ResetMaterial(
	const Properties& rMaterialProperties,
	const GeometryType& rElementGeometry,
	const Vector& rShapeFunctionsValues)
{
	ThresholdTension 			= 0.0;
	CurrentThresholdTension 	= 0.0;
	ThresholdCompression 		= 0.0;
	CurrentThresholdCompression = 0.0;
	DamageParameterTension 		= 0.0;
	DamageParameterTensionOutput = 0.0;
	DamageParameterCompression 	= 0.0;
	DamageParameterCompressionOutput 	= 0.0;
	InitialCharacteristicLength = 0.0;
	InitializeDamageLaw 		= false;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::GetLawFeatures(
	Features& rFeatures)
{
	//Set the type of law
	rFeatures.mOptions.Set( PLANE_STRESS_LAW );
	rFeatures.mOptions.Set( FINITE_STRAINS);
	rFeatures.mOptions.Set( ISOTROPIC );

	//Set strain measure required by the consitutive law
	rFeatures.mStrainMeasures.push_back(StrainMeasure_Velocity_Gradient);

	//Set the strain size
	rFeatures.mStrainSize = GetStrainSize();

	//Set the space dimension
	rFeatures.mSpaceDimension = WorkingSpaceDimension();
}
/***********************************************************************************/
/***********************************************************************************/
int MPMDamageDPlusDMinusMasonry2DLaw::Check(
	const Properties& rMaterialProperties,
	const GeometryType& rElementGeometry,
	const ProcessInfo& rCurrentProcessInfo)
{
	KRATOS_TRY

	if( !rMaterialProperties.Has(YOUNG_MODULUS) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: YOUNG_MODULUS", "");

	if( !rMaterialProperties.Has(POISSON_RATIO) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: POISSON_RATIO", "");

	if( !rMaterialProperties.Has(YIELD_STRESS_TENSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: YIELD_STRESS_TENSION", "");

	if( !rMaterialProperties.Has(FRACTURE_ENERGY_TENSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: FRACTURE_ENERGY_TENSION", "");

	if( !rMaterialProperties.Has(DAMAGE_ONSET_STRESS_COMPRESSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: DAMAGE_ONSET_STRESS_COMPRESSION", "");

	if( !rMaterialProperties.Has(YIELD_STRESS_COMPRESSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: YIELD_STRESS_COMPRESSION", "");

	if( !rMaterialProperties.Has(RESIDUAL_STRESS_COMPRESSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: RESIDUAL_STRESS_COMPRESSION", "");

	if( !rMaterialProperties.Has(YIELD_STRAIN_COMPRESSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: YIELD_STRAIN_COMPRESSION", "");

	if( !rMaterialProperties.Has(FRACTURE_ENERGY_COMPRESSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: FRACTURE_ENERGY_COMPRESSION", "");

	if( !rMaterialProperties.Has(BIAXIAL_COMPRESSION_MULTIPLIER) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: BIAXIAL_COMPRESSION_MULTIPLIER", "");

	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C1_TENSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: STRAIN_RATE_FACTOR_C1_TENSION", "");

	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C2_TENSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: STRAIN_RATE_FACTOR_C2_TENSION", "");

	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C1_COMPRESSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: STRAIN_RATE_FACTOR_C1_COMPRESSION", "");

	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C2_COMPRESSION) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: STRAIN_RATE_FACTOR_C2_COMPRESSION", "");

	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C1_YOUNGS_MOD) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: STRAIN_RATE_FACTOR_C1_YOUNGS_MOD", "");

	if( !rMaterialProperties.Has(STRAIN_RATE_FACTOR_C2_YOUNGS_MOD) )
		KRATOS_THROW_ERROR(std::logic_error, "Missing variable: STRAIN_RATE_FACTOR_C2_YOUNGS_MOD", "");

	return 0;

	KRATOS_CATCH("");

}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateMaterialResponse(const Vector& StrainVector,
	const Matrix& DeformationGradient,
	Vector& StressVector,
	Matrix& AlgorithmicTangent,
	const ProcessInfo& rCurrentProcessInfo,
	const Properties& rMaterialProperties,
	const GeometryType& rElementGeometry,
	const Vector& rShapeFunctionsValues,
	bool CalculateStresses,
	int CalculateTangent,
	bool SaveInternalVariables)
{
	KRATOS_ERROR << "Hit MPMDamageDPlusDMinusMasonry2DLaw::CalculateMaterialResponse\n";
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::InitializeCalculationData(
	const Properties& props,
	const GeometryType& geom,
	const ProcessInfo& pinfo,
	CalculationData& data)
{
	// Include strain rate effects
	const double dif_tension = GetDIF(props, 0);
	const double dif_compression = GetDIF(props, 1);
	const double dif_youngs = GetDIF(props, 2);

	// elasticity
	data.YoungModulus   			= props[YOUNG_MODULUS]* dif_youngs;
	data.PoissonRatio  				= props[POISSON_RATIO];
	this->CalculateElasticityMatrix(data);

	// Tension Damage Properties
	data.YieldStressTension 		= props[YIELD_STRESS_TENSION]* dif_tension;
	data.FractureEnergyTension 		= props[FRACTURE_ENERGY_TENSION]* dif_tension;

	// Compression Damage Properties
	data.DamageOnsetStressCompression 	= props[DAMAGE_ONSET_STRESS_COMPRESSION] * dif_compression;
	data.YieldStressCompression 		= props[YIELD_STRESS_COMPRESSION]* dif_compression;
	data.ResidualStressCompression 		= props[RESIDUAL_STRESS_COMPRESSION];
	data.YieldStrainCompression  		= props[YIELD_STRAIN_COMPRESSION];
	data.BezierControllerS1  			= props.Has(BEZIER_CONTROLLER_S1) ? props[BEZIER_CONTROLLER_S1] : 0.75; // Updated for concrete
	data.BezierControllerEP1  			= props.Has(BEZIER_CONTROLLER_EP1) ? props[BEZIER_CONTROLLER_EP1] : 1.1; // Updated for concrete
	data.BezierControllerEP2  			= props.Has(BEZIER_CONTROLLER_EP2) ? props[BEZIER_CONTROLLER_EP2] : 1.1; // Updated for concrete
	data.BezierControllerEP3  			= props.Has(BEZIER_CONTROLLER_EP3) ? props[BEZIER_CONTROLLER_EP3] : 1.25; // Updated for concrete
	data.BezierControllerEP4  			= props.Has(BEZIER_CONTROLLER_EP4) ? props[BEZIER_CONTROLLER_EP4] : 1.25; // Updated for concrete
	data.FractureEnergyCompression  	= props[FRACTURE_ENERGY_COMPRESSION]* dif_compression;
	data.BiaxialCompressionMultiplier  	= props[BIAXIAL_COMPRESSION_MULTIPLIER];
	data.ShearCompressionReductor  		= props.Has(SHEAR_COMPRESSION_REDUCTOR) ? props[SHEAR_COMPRESSION_REDUCTOR] : 1.0; // Changed default to 1.0 to match Lubliner
	data.ShearCompressionReductor  		= std::min(std::max(data.ShearCompressionReductor,0.0),1.0);

	// Effective Stress Data
	data.EffectiveStressVector.resize(3,false);
	data.PrincipalStressVector.resize(2,false);
	data.EffectiveTensionStressVector.resize(3,false);
	data.EffectiveCompressionStressVector.resize(3,false);
	data.ProjectionTensorTension.resize(3,3,false);
	data.ProjectionTensorCompression.resize(3,3,false);

	// Misc
	data.CharacteristicLength = InitialCharacteristicLength;
	data.DeltaTime = pinfo[DELTA_TIME];
	data.TensionYieldModel = props.Has(TENSION_YIELD_MODEL) ? props[TENSION_YIELD_MODEL] : 0;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateElasticityMatrix(
	CalculationData& data)
{
	if(data.ElasticityMatrix.size1() != 3 || data.ElasticityMatrix.size2() != 3)
		data.ElasticityMatrix.resize(3,3,false);

	double c1 = data.YoungModulus / (1.0 - data.PoissonRatio * data.PoissonRatio);
	double c2 = c1 * data.PoissonRatio;
	double c3 = c1 * (1.0 - data.PoissonRatio) / 2.0;

	data.ElasticityMatrix(0,0) = c1;	data.ElasticityMatrix(0,1) = c2;	data.ElasticityMatrix(0,2) = 0.0;
	data.ElasticityMatrix(1,0) = c2;	data.ElasticityMatrix(1,1) = c1;	data.ElasticityMatrix(1,2) = 0.0;
	data.ElasticityMatrix(2,0) = 0.0;	data.ElasticityMatrix(2,1) = 0.0;	data.ElasticityMatrix(2,2) = c3;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::TensionCompressionSplit(
	CalculationData& data)
{
	const array_1d<double,3>& effective_stress_vector 		= data.EffectiveStressVector;
	array_1d<double,2>& principal_stress_vector 			= data.PrincipalStressVector;
	array_1d<double,3>& effective_tension_stress_vector 	= data.EffectiveTensionStressVector;
	array_1d<double,3>& effective_compression_stress_vector = data.EffectiveCompressionStressVector;

	ConstitutiveLawUtilities<3>::CalculatePrincipalStresses(
		principal_stress_vector, effective_stress_vector);
	ConstitutiveLawUtilities<3>::SpectralDecomposition(
		effective_stress_vector, effective_tension_stress_vector, effective_compression_stress_vector);
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::ConstructProjectionTensors(
	CalculationData& data)
{
	Matrix& projection_tensor_tension 	  = data.ProjectionTensorTension;
	Matrix& projection_tensor_compression = data.ProjectionTensorCompression;

	const array_1d<double,3>& effective_stress_vector = data.EffectiveStressVector;

	Matrix effective_stress_tensor = MathUtils<double>::StressVectorToTensor(effective_stress_vector);
    BoundedMatrix<double, Dimension, Dimension> eigen_vectors_matrix;
    BoundedMatrix<double, Dimension, Dimension> eigen_values_matrix;

	MathUtils<double>::GaussSeidelEigenSystem(effective_stress_tensor, eigen_vectors_matrix, eigen_values_matrix, 1.0e-16, 20);

	array_1d<double,2> eigen_vector_1;
	array_1d<double,2> eigen_vector_2;

	for (IndexType i = 0; i < Dimension; ++i) {
		eigen_vector_1[i] = eigen_vectors_matrix(0, i);
	}
	for (IndexType i = 0; i < Dimension; ++i) {
		eigen_vector_2[i] = eigen_vectors_matrix(1, i);
	}

	array_1d<double,3> projection_vector_11;
	Matrix projection_tensor_11;
	projection_tensor_11 = outer_prod(eigen_vector_1, eigen_vector_1);
	projection_vector_11 = MathUtils<double>::StressTensorToVector(projection_tensor_11);

	array_1d<double,3> projection_vector_22;
	Matrix projection_tensor_22;
	projection_tensor_22 = outer_prod(eigen_vector_2, eigen_vector_2);
	projection_vector_22 = MathUtils<double>::StressTensorToVector(projection_tensor_22);

	projection_tensor_tension.clear();
	noalias(projection_tensor_tension) += HEAVISIDE(eigen_values_matrix(0, 0)) *
										  outer_prod(projection_vector_11, projection_vector_11);
	noalias(projection_tensor_tension) += HEAVISIDE(eigen_values_matrix(1, 1)) *
										  outer_prod(projection_vector_22, projection_vector_22);

#ifdef PROJECTION_OPERATOR_CERVERA_2003
/*
Theory from: "Viscoelasticity and rate-dependent continuum damage models"
			  Miguel Cervera
			  2003 (page 58)
*/
	array_1d<double,3> projection_vector_12;
	Matrix projection_tensor_12;
	array_1d<double,3> projection_vector_21;
	Matrix projection_tensor_21;
	projection_tensor_12 = outer_prod(eigen_vector_1, eigen_vector_2);
	projection_tensor_21 = outer_prod(eigen_vector_2, eigen_vector_1);

	array_1d<double,3> projection_vector_cross;
	Matrix projection_tensor_cross;
	projection_tensor_cross = 0.5 * (projection_tensor_12 + projection_tensor_21);
	projection_vector_cross = MathUtils<double>::StressTensorToVector(projection_tensor_cross);

	double factor_12;
	factor_12 = MACAULAY(eigen_values_matrix(0, 0)) - MACAULAY(eigen_values_matrix(1, 1));

	if (std::abs(eigen_values_matrix(0, 0) - eigen_values_matrix(1, 1)) > 0.0) {
		factor_12 *= 2.0;
		factor_12 /= eigen_values_matrix(0, 0) - eigen_values_matrix(1, 1);
	} else {
		factor_12 = 1.0;
	}
	noalias(projection_tensor_tension) += factor_12 * outer_prod(projection_vector_cross, projection_vector_cross);
#endif //PROJECTION_OPERATOR_CERVERA_2003

#ifdef PROJECTION_OPERATOR_CERVERA_2017
/*
Theory from: 	"An Energy-Equivalent d+/d- Damage Model with
				Enhanced Microcrack Closure-Reopening
				Capabilities for Cohesive-Frictional Materials"
				Miguel Cervera & Claudia Tesei
				2017 (page 7/30)
*/
	array_1d<double,3> projection_vector_12;
	Matrix projection_tensor_12;
	array_1d<double,3> projection_vector_21;
	Matrix projection_tensor_21;
	projection_tensor_12 = outer_prod(eigen_vector_1, eigen_vector_2);
	projection_tensor_21 = outer_prod(eigen_vector_2, eigen_vector_1);

	array_1d<double,3> projection_vector_cross;
	Matrix projection_tensor_cross;
	projection_tensor_cross = 0.5 * (projection_tensor_12 + projection_tensor_21);
	projection_vector_cross = MathUtils<double>::StressTensorToVector(projection_tensor_cross);

	noalias(projection_tensor_tension) += (HEAVISIDE(eigen_values_matrix(0, 0)) + HEAVISIDE(eigen_values_matrix(1, 1))) *
										  outer_prod(projection_vector_cross, projection_vector_cross);
#endif //PROJECTION_OPERATOR_CERVERA_2017

	noalias(projection_tensor_compression) = IdentityMatrix(3,3) - projection_tensor_tension;
}

/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateEquivalentStressTension(CalculationData& data, double& UniaxialStressTension)
{
	UniaxialStressTension = 0.0;
	if(data.PrincipalStressVector(0) > 0.0){
		if (data.TensionYieldModel == 0) {
			// Lubliner Yield Criteria
			//const double yield_compression 		 	= 	data.YieldStressCompression;
			const double yield_compression 		 	=	data.DamageOnsetStressCompression; // Updated to match petr2016, cerv2018
			const double yield_tension 				= 	data.YieldStressTension;
			const double alpha 						= 	(data.BiaxialCompressionMultiplier - 1.0) /
														(2.0* data.BiaxialCompressionMultiplier - 1.0);
			double I1, J2;
			array_1d<double, 3> deviator = ZeroVector(3);

			ConstitutiveLawUtilities<3>::CalculateI1Invariant(data.EffectiveStressVector, I1);
			ConstitutiveLawUtilities<3>::CalculateJ2Invariant(data.EffectiveStressVector, I1, deviator, J2);

			const double beta 	= yield_compression / yield_tension * (1.0 - alpha) - (1.0 + alpha);
			const double smax 	= std::max(std::max(data.PrincipalStressVector(0), data.PrincipalStressVector(1)),0.0);

			UniaxialStressTension = 1.0 / (1.0-alpha) * (alpha * I1 + std::sqrt(3.0 * J2) + beta * smax) /
									yield_compression * yield_tension;
		}
		else if (data.TensionYieldModel == 1) {
			// Rankine Yield Criteria
			KRATOS_ERROR << "USING RANKINE\n";
			UniaxialStressTension = std::max(std::max(data.PrincipalStressVector(0), data.PrincipalStressVector(1)), 0.0);
		}
	}
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateEquivalentStressCompression(CalculationData& data, double& UniaxialStressCompression)
{
	UniaxialStressCompression = 0.0;
	if(data.PrincipalStressVector(1) < 0.0){
		const double yield_compression 			= 	data.DamageOnsetStressCompression;
		const double yield_tension 				= 	data.YieldStressTension;
		const double alpha 						= 	(data.BiaxialCompressionMultiplier - 1.0) /
													(2.0* data.BiaxialCompressionMultiplier - 1.0);
		double I1, J2;
		array_1d<double, 3> deviator = ZeroVector(3);

		ConstitutiveLawUtilities<3>::CalculateI1Invariant(data.EffectiveStressVector, I1);
		ConstitutiveLawUtilities<3>::CalculateJ2Invariant(data.EffectiveStressVector, I1, deviator, J2);

		const double beta = (yield_compression / yield_tension) * (1.0 - alpha) - (1.0 + alpha);
		const double smax = std::max(std::max(data.PrincipalStressVector(0), data.PrincipalStressVector(1)),0.0);

		KRATOS_ERROR_IF_NOT(data.ShearCompressionReductor == 1.0) << "data.ShearCompressionReductor DOES NOT EQUAL ONE!\n";

		UniaxialStressCompression = 1.0 / (1.0-alpha) * (alpha * I1 + std::sqrt(3.0 * J2) +
															data.ShearCompressionReductor * beta * smax);
	}
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateDamageTension(
	CalculationData& data,
	double internal_variable,
	double& rDamage)
{
	if (internal_variable <= data.YieldStressTension) {
		rDamage = DamageParameterTensionOutput;
	}
	else if (DamageParameterTensionOutput > 0.99) rDamage = DamageParameterTensionOutput;
	else {
		const double characteristic_length 		= 	data.CharacteristicLength;
		const double young_modulus   			= 	data.YoungModulus;
		const double yield_tension  			= 	data.YieldStressTension;
		const double fracture_energy_tension  	=  	data.FractureEnergyTension;
		const double initial_internal_variable  =  	yield_tension;
		const double material_length  			=  	2.0 * young_modulus * fracture_energy_tension /
													(yield_tension * yield_tension);

		if(characteristic_length >= material_length){
			std::stringstream ss;
			ss << "FRACTURE_ENERGY_TENSION is too low:  2*E*Gt/(ft*ft) = " << material_length
				<< ",   Characteristic Length = " << characteristic_length
				<< ",   FRACTURE_ENERGY_TENSION should be at least = " << (characteristic_length * yield_tension * yield_tension) / (2.0 * young_modulus) <<std::endl;
			std::cout << ss.str();
			exit(-1);
		}

		const double damage_parameter  = 2.0 * characteristic_length /
											(material_length - characteristic_length);

		rDamage = 	1.0 - initial_internal_variable / internal_variable *
							std::exp(damage_parameter *
							(1.0 - internal_variable / initial_internal_variable));

		if (rDamage > 0.99) rDamage = 1.0;
		if (rDamage < DamageParameterTensionOutput) rDamage = DamageParameterTensionOutput;
		else DamageParameterTensionOutput = rDamage;
	}
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateDamageCompression(
	CalculationData& data,
	double threshold_compression,
	double eq_compression_stress,
	double& rDamage)
{

	if (threshold_compression <= data.DamageOnsetStressCompression) rDamage = 0.0;
	else if (eq_compression_stress <= data.ResidualStressCompression) rDamage = 0.0; // dont apply damage if we are below the residual stress
	else if (DamageParameterCompressionOutput > 0.99) rDamage = 1.0 - data.ResidualStressCompression / eq_compression_stress; // set eq stress to residual stress if we are fully damaged
	else {
		// extract material parameters
		const double young_modulus = data.YoungModulus;
		const double s_0 = data.DamageOnsetStressCompression;
		const double s_p = data.YieldStressCompression;
		const double s_r = data.ResidualStressCompression;
		const double e_p = std::max(data.YieldStrainCompression, s_p / young_modulus);

		const double c_s1 			= data.BezierControllerS1;
		const double c_ep1 			= data.BezierControllerEP1;
		const double c_ep2 			= data.BezierControllerEP2;
		const double c_ep3 			= data.BezierControllerEP3;
		const double c_ep4 			= data.BezierControllerEP4;

		const double specific_fracture_energy = data.FractureEnergyCompression /
			data.CharacteristicLength;

		// Auto-computation of remaining constitutive law parameters
		const double s_k = s_r + (s_p - s_r) * c_s1;
		const double e_0 = s_0 / young_modulus;
		const double e_i = s_p / young_modulus;

		double e_j = c_ep1 * e_p;
		double e_k = c_ep2 * e_j;
		double e_r = c_ep3 * e_k;
		double e_u = e_r * c_ep4;

		// current abscissa
		const double strain_like_counterpart = eq_compression_stress / young_modulus;

		if (strain_like_counterpart > e_p)
		{
			// regularization - only if we are past the peak!
			double bezier_fracture_energy, bezier_energy_1;
			this->ComputeBezierEnergy(bezier_fracture_energy, bezier_energy_1,
				s_p, s_k, s_r, e_p, e_j, e_k, e_r, e_u);

			const double stretcher = (specific_fracture_energy - bezier_energy_1) /
				(bezier_fracture_energy - bezier_energy_1) - 1.0;

			if (stretcher <= -1.0) {
				std::stringstream ss;
				ss << "FRACTURE_ENERGY_COMPRESSION is too low" << std::endl;
				ss << "Characteristic Length = " << data.CharacteristicLength << std::endl;
				ss << "Input Gc/lch = " << specific_fracture_energy << std::endl;
				ss << "To avoid constitutive snap-back, FRACTURE_ENERGY_COMPRESSION should be at least = " << bezier_energy_1 << std::endl;
				ss << "Strain rate  = " << mStrainRate << std::endl;
				ss << "MP initial position = " << element_center << std::endl;
				std::cout << ss.str();
				exit(-1);
			}

			this->ApplyBezierStretcherToStrains(stretcher, e_p, e_j, e_k, e_r, e_u);
		}


		// Compute damage
		double damage_stress;
		if(strain_like_counterpart <= e_p){
			// Hardening pseudo-damage (reduction of stresses but not actual damage)
			this->EvaluateBezierCurve(damage_stress, strain_like_counterpart, e_0, e_i, e_p, s_0, s_p, s_p);
			rDamage = 1.0 - damage_stress / data.YieldStressCompression;
		}
		else
		{
			// Actual damage
			if (strain_like_counterpart <= e_k) {
				this->EvaluateBezierCurve(damage_stress, strain_like_counterpart, e_p, e_j, e_k, s_p, s_p, s_k);
			}
			else if (strain_like_counterpart <= e_u) {
				this->EvaluateBezierCurve(damage_stress, strain_like_counterpart, e_k, e_r, e_u, s_k, s_r, s_r);
			}
			else {
				damage_stress = s_r;
			}
			const double current_true_damage = 1.0 - (damage_stress - s_r) / (data.YieldStressCompression - s_r); // from 0-1
			if (current_true_damage > DamageParameterCompressionOutput) DamageParameterCompressionOutput = current_true_damage; // update if current damage increases

			const double predicted_damaged_stress = s_r + (1.0 - DamageParameterCompressionOutput) * (data.YieldStressCompression - s_r);

			rDamage = 1.0 - predicted_damaged_stress/ eq_compression_stress;
			rDamage = std::max(0.0, rDamage);
			rDamage = std::min(1.0, rDamage);
		}
	}
	if (rDamage <0.0 || rDamage > 1.0)
	{
		KRATOS_WATCH("DAMAGE OUT OF BOUNDS!")
		KRATOS_ERROR << "DAMAGE OUT OF BOUNDS!";
	}
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::ComputeBezierEnergy(double& rBezierEnergy, double& rBezierEnergy1,
								double s_p, double s_k, double s_r,
								double e_p, double e_j, double e_k, double e_r, double e_u)
{
	rBezierEnergy1 = e_p * s_p / 2.0;
	double bezier_energy_2 = this->EvaluateBezierArea(e_p, e_j, e_k, s_p, s_p, s_k);
	double bezier_energy_3 = this->EvaluateBezierArea(e_k, e_r, e_u, s_k, s_r, s_r);
	rBezierEnergy = rBezierEnergy1 + bezier_energy_2 + bezier_energy_3;
}
/***********************************************************************************/
/***********************************************************************************/
double MPMDamageDPlusDMinusMasonry2DLaw::EvaluateBezierArea(
	double x1,double x2,double x3,double y1,double y2,double y3)
{
	double bezier_area = 	x2 * y1 / 3.0 +
							x3 * y1 / 6.0 -
							x2 * y3 / 3.0 +
							x3 * y2 / 3.0 +
							x3 * y3 / 2.0 -
							x1 * (y1 / 2.0 + y2 / 3.0 + y3 / 6.0) ;
	return bezier_area;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::ApplyBezierStretcherToStrains(
	double stretcher, double e_p, double& e_j, double& e_k, double& e_r, double& e_u)
{
	e_j += (e_j - e_p) * stretcher;
	e_k += (e_k - e_p) * stretcher;
	e_r += (e_r - e_p) * stretcher;
	e_u += (e_u - e_p) * stretcher;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::EvaluateBezierCurve(
	double& rDamageParameter, double xi,
	double x1, double x2, double x3,
	double y1, double y2, double y3)
{
	double bezier_law_param_A = x1 - 2.0 * x2 + x3;
	double bezier_law_param_B = 2.0 * (x2 - x1);
	double bezier_law_param_C = x1 - xi;

	if(std::abs(bezier_law_param_A) < 1.0E-12){
		x2 					= x2 + 1.0E-6 * (x3 - x1);
		bezier_law_param_A 	= x1 - 2.0 * x2 + x3;
		bezier_law_param_B 	= 2.0 * (x2 - x1);
		bezier_law_param_C 	= x1 - xi;
	}

	double bezier_law_param_D = bezier_law_param_B * bezier_law_param_B -
								4.0 * bezier_law_param_A * bezier_law_param_C;
	double bezier_law_param_t = (-bezier_law_param_B + std::sqrt(bezier_law_param_D)) /
								(2.0 * bezier_law_param_A);
	rDamageParameter = 	(y1 - 2.0 * y2 + y3) * bezier_law_param_t * bezier_law_param_t +
						2.0 * (y2 - y1) * bezier_law_param_t + y1;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::ComputeCharacteristicLength(
    const GeometryType& geom,
	const Properties& rMaterialProperties,
    double& rCharacteristicLength)
{
	// Updated for MPM - we take the material point volume
	rCharacteristicLength = 0.0;
	const double area = geom.GetValue(MP_VOLUME) / rMaterialProperties[THICKNESS];
	rCharacteristicLength = std::sqrt(area);

	KRATOS_ERROR_IF(rCharacteristicLength == 0.0) << "Characteristic length not set properly!\n"
		<< "Geom MP_VOLUME = " << geom.GetValue(MP_VOLUME) << "\n";
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateMaterialResponseInternal(
	const Vector& StrainVector,
	Vector& PredictiveStressVector,
	CalculationData& data,
	Properties props)
{
	if(PredictiveStressVector.size() != VoigtSize)
		PredictiveStressVector.resize(VoigtSize,false);

	ThresholdTension     = CurrentThresholdTension;
	ThresholdCompression = CurrentThresholdCompression;

	noalias(data.EffectiveStressVector) = prod(data.ElasticityMatrix, StrainVector);

	if(std::abs(data.EffectiveStressVector(0)) < tolerance) {data.EffectiveStressVector(0) = 0.0;}
	if(std::abs(data.EffectiveStressVector(1)) < tolerance) {data.EffectiveStressVector(1) = 0.0;}
	if(std::abs(data.EffectiveStressVector(2)) < tolerance) {data.EffectiveStressVector(2) = 0.0;}

	this->TensionCompressionSplit(data);
	this->ConstructProjectionTensors(data);

	// compute the equivalent stress measures
	this->CalculateEquivalentStressTension(data, UniaxialStressTension);
	this->CalculateEquivalentStressCompression(data, UniaxialStressCompression);

	// damage update
	if (props[INTEGRATION_IMPLEX] != 0){ //IMPLEX Integration
		// time factor
		KRATOS_ERROR << "HIT IMPLEX!\n";
		double time_factor = 0.0;
		if(PreviousDeltaTime > 0.0) time_factor = data.DeltaTime / PreviousDeltaTime;
		CurrentDeltaTime = data.DeltaTime;

		// explicit evaluation
		ThresholdTension = CurrentThresholdTension + time_factor * (CurrentThresholdTension - PreviousThresholdTension);
		ThresholdCompression = CurrentThresholdCompression + time_factor * (CurrentThresholdCompression - PreviousThresholdCompression);

		// save implicit variables for the finalize_solution_step
		double implicit_threshold_tension 		= CurrentThresholdTension;
		double implicit_threshold_compression 	= CurrentThresholdCompression;

		if(UniaxialStressTension > implicit_threshold_tension)
			implicit_threshold_tension = UniaxialStressTension;

		if(UniaxialStressCompression > implicit_threshold_compression)
			implicit_threshold_compression = UniaxialStressCompression;

		TemporaryImplicitThresholdTension 		= implicit_threshold_tension;
		TemporaryImplicitThresholdTCompression 	= implicit_threshold_compression;

		// new damage variables (explicit)
		this->CalculateDamageTension(data, ThresholdTension, DamageParameterTension);
		this->CalculateDamageCompression(data, ThresholdCompression, UniaxialStressCompression,DamageParameterCompression);
	}
	else { // IMPLICIT Integration

		if(UniaxialStressTension > ThresholdTension)
			ThresholdTension = UniaxialStressTension;
		this->CalculateDamageTension(data, ThresholdTension, DamageParameterTension);

		if(UniaxialStressCompression > ThresholdCompression)
			ThresholdCompression = UniaxialStressCompression;
		this->CalculateDamageCompression(data, ThresholdCompression, UniaxialStressCompression, DamageParameterCompression);

		TemporaryImplicitThresholdTension = ThresholdTension;
		TemporaryImplicitThresholdTCompression = ThresholdCompression;
	}

	// calculation of stress tensor
	noalias(PredictiveStressVector)  = (1.0 - DamageParameterTension)     * data.EffectiveTensionStressVector;
	noalias(PredictiveStressVector) += (1.0 - DamageParameterCompression) * data.EffectiveCompressionStressVector;

}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CheckDamageLoadingUnloading(
	bool& is_damaging_tension,
	bool& is_damaging_compression)
{
	const double F_tension 		= UniaxialStressTension - CurrentThresholdTension;
	const double F_compression 	= UniaxialStressCompression - CurrentThresholdCompression;

	is_damaging_tension = false;
	if (F_tension > 0.0){
		is_damaging_tension = true;
	}
	is_damaging_compression = false;
	if (F_compression > 0.0){
		is_damaging_compression = true;
	}
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateTangentTensor(
	Parameters& rValues,
	Vector StrainVector,
	Vector PredictiveStressVector,
	CalculationData& data,
	Properties props)
{
	// prepare constitutive matrix
	Matrix& constitutive_matrix = rValues.GetConstitutiveMatrix();
	if(constitutive_matrix.size1() != VoigtSize || constitutive_matrix.size2() != VoigtSize)
		constitutive_matrix.resize(VoigtSize, VoigtSize);

	// save internal variables
	double save_threshold_tension			= ThresholdTension;
	double save_threshold_compression		= ThresholdCompression;
	double save_damage_tension				= DamageParameterTension;
	double save_damage_compression			= DamageParameterCompression;
	double save_uniaxial_stress_tension		= UniaxialStressTension;
	double save_uniaxial_stress_compression	= UniaxialStressCompression;

	// perturbation parameter
	double perturbation_factor = 1.0E-8;

	// perturbed vectors
	Vector perturbated_strain_vector(VoigtSize);
	Vector perturbated_stress_vector(VoigtSize);

	// apply perturbation to each strain component...
	for(size_t j = 0; j < VoigtSize; j++)
	{
		noalias(perturbated_strain_vector) = StrainVector;

		perturbated_strain_vector(j) = StrainVector(j) + perturbation_factor;
		this->CalculateMaterialResponseInternal(perturbated_strain_vector, perturbated_stress_vector, data, props);

		for(size_t i = 0; i < VoigtSize; i++)
			constitutive_matrix(i,j) = ( perturbated_stress_vector(i) - PredictiveStressVector(i) ) /
										perturbation_factor;
	}

	// restore internal variables
	ThresholdTension	   		= save_threshold_tension;
	ThresholdCompression	   	= save_threshold_compression;
	DamageParameterTension 		= save_damage_tension;
	DamageParameterCompression 	= save_damage_compression;
	UniaxialStressTension 		= save_uniaxial_stress_tension;
	UniaxialStressCompression 	= save_uniaxial_stress_compression;
}
/***********************************************************************************/
/***********************************************************************************/
void MPMDamageDPlusDMinusMasonry2DLaw::CalculateSecantTensor(
	Parameters& rValues,
	CalculationData& data)
{
	Matrix& constitutive_matrix = rValues.GetConstitutiveMatrix();
	if(constitutive_matrix.size1() != VoigtSize || constitutive_matrix.size2() != VoigtSize)
		constitutive_matrix.resize(VoigtSize, VoigtSize);

	Matrix DamageMatrix( IdentityMatrix(3,3) );
	noalias(DamageMatrix) -= DamageParameterTension     * data.ProjectionTensorTension;
	noalias(DamageMatrix) -= DamageParameterCompression * data.ProjectionTensorCompression;

	noalias(constitutive_matrix) = prod(DamageMatrix, data.ElasticityMatrix);
}

const double MPMDamageDPlusDMinusMasonry2DLaw::GetDIF(const Properties& rProps,
	const int dif_case)
{
	// Power strain rate laws to match Cusa2011 (tension compression)
	// Ozbo2006 (E)

	// DIF = c_1 * e_dot^c_2

	double c_1, c_2;

	switch (dif_case)
	{
	case 0:
		// tension
		c_1 = rProps[STRAIN_RATE_FACTOR_C1_TENSION];
		c_2 = rProps[STRAIN_RATE_FACTOR_C2_TENSION];
		break;
	case 1:
		// compression
		c_1 = rProps[STRAIN_RATE_FACTOR_C1_COMPRESSION];
		c_2 = rProps[STRAIN_RATE_FACTOR_C2_COMPRESSION];
		break;
	case 2:
		// Young's modulus
		c_1 = rProps[STRAIN_RATE_FACTOR_C1_YOUNGS_MOD];
		c_2 = rProps[STRAIN_RATE_FACTOR_C2_YOUNGS_MOD];
		break;
	default:
		KRATOS_ERROR << "INVALID DIF CASE\n";
		break;
	}

	double dif = c_1 * std::pow(mStrainRate, c_2);
	dif = std::max(1.0, dif);
	return dif;
}

} // namespace Kratos


