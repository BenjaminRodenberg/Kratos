//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ \.
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Bodhinanda Chandra
//


#ifndef KRATOS_MPM_BOUNDARY_ROTATION_UTILITY
#define KRATOS_MPM_BOUNDARY_ROTATION_UTILITY

// system includes

// external includes

// kratos includes
#include "includes/define.h"
#include "includes/node.h"
#include "containers/variable.h"
#include "geometries/geometry.h"
#include "utilities/coordinate_transformation_utilities.h"

namespace Kratos {

///@addtogroup ParticleMechanicsApplication
///@{

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

/* A utility to rotate the local contributions of certain nodes to the system matrix,
which is required to apply slip conditions (roller-type support) in arbitrary directions to the boundary nodes.*/
template<class TLocalMatrixType, class TLocalVectorType>
class MPMBoundaryRotationUtility: public CoordinateTransformationUtils<TLocalMatrixType,TLocalVectorType,double> {
public:
	///@name Type Definitions
	///@{

	/// Pointer definition of MPMBoundaryRotationUtility
	KRATOS_CLASS_POINTER_DEFINITION(MPMBoundaryRotationUtility);

	using CoordinateTransformationUtils<TLocalMatrixType,TLocalVectorType,double>::Rotate;

	typedef Node<3> NodeType;

	typedef Geometry< Node<3> > GeometryType;

	///@}
	///@name Life Cycle
	///@{

	/// Constructor.
	/** @param DomainSize Number of space dimensions (2 or 3)
	 * @param NumRowsPerNode Number of matrix or vector rows associated to each node. Displacement DOFs are assumed to be the first mDomainSize rows in each block of rows.
	 * @param rVariable Kratos variable used to flag nodes where local system contributions will be rotated. All nodes with rVariable != Zero will be rotated.
	 */
	MPMBoundaryRotationUtility(
        const unsigned int DomainSize,
		const unsigned int BlockSize,
		const Variable<double>& rVariable):
    CoordinateTransformationUtils<TLocalMatrixType,TLocalVectorType,double>(DomainSize,BlockSize,SLIP), mrFlagVariable(rVariable)
	{}

	/// Destructor.
	~MPMBoundaryRotationUtility() override {}

	/// Assignment operator.
	MPMBoundaryRotationUtility& operator=(MPMBoundaryRotationUtility const& rOther) {}

	///@}
	///@name Operators
	///@{

	///@}
	///@name Operations
	///@{

	/// Rotate the local system contributions so that they are oriented with each node's normal.
	/**
	 @param rLocalMatrix Local system matrix
	 @param rLocalVector Local RHS vector
	 @param rGeometry A reference to the element's (or condition's) geometry
	 */
	void Rotate(
        TLocalMatrixType& rLocalMatrix,
		TLocalVectorType& rLocalVector,
		GeometryType& rGeometry) const override
	{

		const unsigned int num_nodes = rGeometry.PointsNumber();
		const unsigned int dimension = this->GetDomainSize();
		const unsigned int local_size = num_nodes * dimension;
	
		
		if (rLocalVector.size()>0){
			if (rLocalVector.size() == local_size) // irreducible case
			{
				if (this->GetDomainSize() == 2) this->template RotateAuxPure<2>(rLocalMatrix,rLocalVector,rGeometry);
				else if (this->GetDomainSize() == 3) this->template RotateAuxPure<3>(rLocalMatrix,rLocalVector,rGeometry);
			}
			else if (rLocalVector.size() == dimension + local_size) //lagrange multiplier condition
			{
				// TODO: Improve if with dimension --> template it in coordinate_transformation_utilities?; 
				if (dimension==2)
				{
					const unsigned int LocalSize = rLocalVector.size();

					unsigned int Index = 0;
					int rotations_needed = 0;
					const unsigned int NumBlocks = num_nodes + 1;
					const unsigned int BlockSize = 2;
					DenseVector<bool> NeedRotation( NumBlocks, false);

					std::vector< BoundedMatrix<double,BlockSize,BlockSize> > rRot(NumBlocks);
					for(unsigned int j = 0; j < NumBlocks; ++j)
					{
						if (j<num_nodes)
						{
							if( this->IsSlip(rGeometry[j]) )
							{
								NeedRotation[j] = true;
								rotations_needed++;
								LocalRotationOperatorPure(rRot[j],rGeometry[j]);
							}
						}
						else{
							auto pBoundaryParticle = rGeometry.GetGeometryParent(0).GetValue(MPC_LAGRANGE_NODE);
							if( this->IsSlip(*pBoundaryParticle) )
							{
								NeedRotation[j] = true;
								rotations_needed++;
								LocalRotationOperatorPure(rRot[j],*pBoundaryParticle);
							}
						}

						Index += BlockSize;
					}

					if(rotations_needed > 0)
					{
						BoundedMatrix<double,BlockSize,BlockSize> mat_block, tmp;
						array_1d<double,BlockSize> aux, aux1;

						for(unsigned int i=0; i<NumBlocks; i++)
						{
							if(NeedRotation[i] == true)
							{
								for(unsigned int j=0; j<NumBlocks; j++)
								{
									if(NeedRotation[j] == true)
									{
										this->ReadBlockMatrix<BlockSize>(mat_block, rLocalMatrix, i*BlockSize, j*BlockSize);	
										noalias(tmp) = prod(mat_block,trans(rRot[j]));
										noalias(mat_block) = prod(rRot[i],tmp);
										// Avoid singularities as numerical zero can appear if same rotation matrices are used
										for(unsigned int k=0; k<BlockSize; k++)
										{
											for(unsigned int l=0; l<BlockSize; l++)
											{
												if ((mat_block(k,l)*mat_block(k,l))<1e-20)
													mat_block(k,l) = 0.0;
											}
										}
										this->WriteBlockMatrix<BlockSize>(mat_block, rLocalMatrix, i*BlockSize, j*BlockSize);
									}
									else
									{
										this->ReadBlockMatrix<BlockSize>(mat_block, rLocalMatrix, i*BlockSize, j*BlockSize);
										noalias(tmp) = prod(rRot[i],mat_block);
										this->WriteBlockMatrix<BlockSize>(tmp, rLocalMatrix, i*BlockSize, j*BlockSize);
									}
								}


								for(unsigned int k=0; k<BlockSize; k++)
								aux[k] = rLocalVector[i*BlockSize+k];

								noalias(aux1) = prod(rRot[i],aux);

								for(unsigned int k=0; k<BlockSize; k++)
								rLocalVector[i*BlockSize+k] = aux1[k];
							}
							else
							{
								for(unsigned int j=0; j<NumBlocks; j++)
								{
									if(NeedRotation[j] == true)
									{
										this->ReadBlockMatrix<BlockSize>(mat_block, rLocalMatrix, i*BlockSize, j*BlockSize);
										noalias(tmp) = prod(mat_block,trans(rRot[j]));
										this->WriteBlockMatrix<BlockSize>(tmp, rLocalMatrix, i*BlockSize, j*BlockSize);
									}
								}
							}
			
						}
					}
				}
				else 	// dimension=3
				{
					const unsigned int LocalSize = rLocalVector.size();

					unsigned int Index = 0;
					int rotations_needed = 0;
					const unsigned int NumBlocks = num_nodes + 1;
					const unsigned int BlockSize = 3;

					std::vector< BoundedMatrix<double,BlockSize,BlockSize> > rRot(NumBlocks);
					for(unsigned int j = 0; j < NumBlocks; ++j)
					{
						if (j<num_nodes)
						{
							// Normals of the nodes (only node 1 is considered)
							if( this->IsSlip(rGeometry[j]) )
							{
								rotations_needed++;
								LocalRotationOperatorPure(rRot[j],rGeometry[j]);

							}
						}
						else{
							auto pBoundaryParticle = rGeometry.GetGeometryParent(0).GetValue(MPC_LAGRANGE_NODE);
							if( this->IsSlip(*pBoundaryParticle) )
							{
								rotations_needed++;
								LocalRotationOperatorPure(rRot[j],*pBoundaryParticle);

							}
						}

						Index += BlockSize;
					}

					if(rotations_needed > 0)
					{
						BoundedMatrix<double,BlockSize,BlockSize> mat_block, tmp;
						array_1d<double,BlockSize> aux, aux1;

						for(unsigned int i=0; i<NumBlocks; i++)
						{
							for(unsigned int j=0; j<NumBlocks; j++)
							{
								this->ReadBlockMatrix<BlockSize>(mat_block, rLocalMatrix, i*BlockSize, j*BlockSize);
								noalias(tmp) = prod(mat_block,trans(rRot[j]));
								noalias(mat_block) = prod(rRot[i],tmp);
								// Avoid singularities as numerical zero can appear if same rotation matrices are used
								for(unsigned int k=0; k<BlockSize; k++)
								{
									for(unsigned int l=0; l<BlockSize; l++)
									{
										if ((mat_block(k,l)*mat_block(k,l))<1e-20)
											mat_block(k,l) = 0.0;
									}
								}
								this->WriteBlockMatrix<BlockSize>(mat_block, rLocalMatrix, i*BlockSize, j*BlockSize);
							}

							for(unsigned int k=0; k<BlockSize; k++)
							aux[k] = rLocalVector[i*BlockSize+k];

							noalias(aux1) = prod(rRot[i],aux);

							for(unsigned int k=0; k<BlockSize; k++)
							rLocalVector[i*BlockSize+k] = aux1[k];

						}
					}
				}
			}
			else // mixed formulation case
			{
				if (this->GetDomainSize() == 2) this->template RotateAux<2,3>(rLocalMatrix,rLocalVector,rGeometry);
				else if (this->GetDomainSize() == 3) this->template RotateAux<3,4>(rLocalMatrix,rLocalVector,rGeometry);
			}
		}

	}

	/// RHS only version of Rotate
	void RotateRHS(
        TLocalVectorType& rLocalVector,
		GeometryType& rGeometry) const
	{
		this->Rotate(rLocalVector,rGeometry);
	}

	/// Apply roler type boundary conditions to the rotated local contributions.
	/** This function takes the rotated local system contributions so each
	 node's displacement are expressed using a base oriented with its normal
	 and imposes that the normal displacement is equal to the mesh displacement in
	 the normal direction.
	 */
	void ApplySlipCondition(TLocalMatrixType& rLocalMatrix,
			TLocalVectorType& rLocalVector,
			GeometryType& rGeometry) const override
	{
		const unsigned int LocalSize = rLocalVector.size();

		if (LocalSize > 0)
		{
			for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
			{
				if(this->IsSlip(rGeometry[itNode]) )
				{
					// We fix the first displacement dof (normal component) for each rotated block
					unsigned int j = itNode * this->GetBlockSize();

					// Get the displacement of the boundary mesh, this does not assume that the mesh is moving.
					// If the mesh is moving, need to consider the displacement of the moving mesh into account.
					const array_1d<double,3> & displacement = rGeometry[itNode].FastGetSolutionStepValue(DISPLACEMENT);

					// Get Normal Vector of the boundary
					array_1d<double,3> rN = rGeometry[itNode].FastGetSolutionStepValue(NORMAL);
					this->Normalize(rN);

					for( unsigned int i = 0; i < j; ++i)// Skip term (i,i)
					{
						rLocalMatrix(i,j) = 0.0;
						rLocalMatrix(j,i) = 0.0;
					}
					for( unsigned int i = j+1; i < LocalSize; ++i)
					{
						rLocalMatrix(i,j) = 0.0;
						rLocalMatrix(j,i) = 0.0;
					}

					rLocalVector[j] = inner_prod(rN,displacement);
					rLocalMatrix(j,j) = 1.0;
				}
			}
		}
	}

	/// RHS only version of ApplySlipCondition
	void ApplySlipCondition(TLocalVectorType& rLocalVector,
			GeometryType& rGeometry) const override
	{
		if (rLocalVector.size() > 0)
		{
			for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
			{
				if( this->IsSlip(rGeometry[itNode]) )
				{
					// We fix the first momentum dof (normal component) for each rotated block
					unsigned int j = itNode * this->GetBlockSize(); // +1

					// Get the displacement of the boundary mesh, this does not assume that the mesh is moving.
					// If the mesh is moving, need to consider the displacement of the moving mesh into account.
					const array_1d<double,3> & displacement = rGeometry[itNode].FastGetSolutionStepValue(DISPLACEMENT);
					array_1d<double,3> rN = rGeometry[itNode].FastGetSolutionStepValue(NORMAL);
					this->Normalize(rN);

					rLocalVector[j] = inner_prod(rN,displacement);

				}
			}
		}
	}

	// An extra function to distinguish the application of slip in element considering penalty imposition
	void ElementApplySlipCondition(TLocalMatrixType& rLocalMatrix,
			TLocalVectorType& rLocalVector,
			GeometryType& rGeometry) const
	{
		// If it is not a penalty element, do as standard
		// Otherwise, if it is a penalty element, dont do anything
		if (!this->IsPenalty(rGeometry) && !this->IsLagrange(rGeometry))
		{
			this->ApplySlipCondition(rLocalMatrix, rLocalVector, rGeometry);
		}
	}

	// An extra function to distinguish the application of slip in element considering penalty imposition (RHS Version)
	void ElementApplySlipCondition(TLocalVectorType& rLocalVector,
			GeometryType& rGeometry) const
	{
		// If it is not a penalty element, do as standard
		// Otherwise, if it is a penalty element, dont do anything
		if (!this->IsPenalty(rGeometry) && !this->IsLagrange(rGeometry))
		{
			this->ApplySlipCondition(rLocalVector, rGeometry);
		}
	}

	// An extra function to distinguish the application of slip in condition considering penalty imposition
	void ConditionApplySlipCondition(TLocalMatrixType& rLocalMatrix,
			TLocalVectorType& rLocalVector,
			GeometryType& rGeometry) const
	{
		// If it is not a penalty condition, do as standard
		if (!this->IsPenalty(rGeometry) && !this->IsLagrange(rGeometry))
		{
			this->ApplySlipCondition(rLocalMatrix, rLocalVector, rGeometry);
		}
		// For Penalty, do the following modification
		else if (this->IsPenalty(rGeometry))
		{
			const unsigned int LocalSize = rLocalVector.size();

			if (LocalSize > 0)
			{
				const unsigned int block_size = this->GetBlockSize();
				TLocalMatrixType temp_matrix = ZeroMatrix(rLocalMatrix.size1(),rLocalMatrix.size2());
				for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
				{
					if(this->IsSlip(rGeometry[itNode]) )
					{
						// We fix the first displacement dof (normal component) for each rotated block
						unsigned int j = itNode * block_size;

						// Copy all normal value in LHS to the temp_matrix
						for (unsigned int i = j; i < rLocalMatrix.size1(); i+= block_size)
						{
							temp_matrix(i,j) = rLocalMatrix(i,j);
							temp_matrix(j,i) = rLocalMatrix(j,i);
						}

						// Remove all other value in RHS than the normal component
						for(unsigned int i = j; i < (j + block_size); ++i)
						{
							if (i!=j) rLocalVector[i] = 0.0;
						}
					}
				}
				rLocalMatrix = temp_matrix;
			}
		}
		else if (this->IsLagrange(rGeometry))
		{
			const unsigned int dimension = this->GetDomainSize();
			const unsigned int LocalSize = rLocalVector.size();

			if (LocalSize > 0)
			{
				for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
				{
					if(this->IsSlip(rGeometry[itNode]) )
					{				
						// Remove all other value in LHS than the normal component
						const unsigned int ibase = dimension * rGeometry.PointsNumber();
						for (unsigned int k = 0; k < dimension-1; k++){
							rLocalMatrix(itNode * dimension + k, ibase + k + 1) = 0.0;
							rLocalMatrix(itNode * dimension + k + 1, ibase + k + 1) = 0.0;
							rLocalMatrix(ibase + k + 1 ,itNode * dimension + k) = 0.0;
							rLocalMatrix(ibase + k + 1, itNode * dimension + k +1) = 0.0;
							rLocalMatrix(ibase + k + 1 ,ibase + k + 1) = 1.0;
						}
						
						// Remove all other value in RHS than the normal component
						for (unsigned int k = 1; k < dimension; k++){
							rLocalVector[dimension * itNode + k] = 0.0;
							rLocalVector[dimension * rGeometry.PointsNumber() + k] = 0.0;
						}
					}
				}
			}

		}
		else{
			KRATOS_WATCH("Error in the Rotation of the conditions!!!")
		}
	}

	// An extra function to distinguish the application of slip in condition considering penalty imposition (RHS Version)
	void ConditionApplySlipCondition(TLocalVectorType& rLocalVector,
			GeometryType& rGeometry) const
	{
		// If it is not a penalty condition, do as standard
		if (!this->IsPenalty(rGeometry) && !this->IsLagrange(rGeometry))
		{
			this->ApplySlipCondition(rLocalVector, rGeometry);
		}
		// Otherwise, if it is a penalty element, dont do anything
		else if (this->IsPenalty(rGeometry))
		{
			if (rLocalVector.size() > 0)
			{
				const unsigned int block_size = this->GetBlockSize();
				for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
				{
					if( this->IsSlip(rGeometry[itNode]) )
					{
						// We fix the first momentum dof (normal component) for each rotated block
						unsigned int j = itNode * block_size;

						// Remove all other value than the normal component
						for(unsigned int i = j; i < (j + block_size); ++i)
						{
							if (i!=j) rLocalVector[i] = 0.0;
						}
					}
				}
			}
		}
		else if (this->IsLagrange(rGeometry))
		{
			const unsigned int dimension = this->GetDomainSize();
			if (rLocalVector.size() > 0)
			{
				for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
				{
						if(this->IsSlip(rGeometry[itNode]) )
						{
							
							// Remove all other value in RHS than the normal component
							for (unsigned int k = 1; k < dimension; k++){
								rLocalVector[dimension * itNode + k] = 0.0;
								rLocalVector[dimension * rGeometry.PointsNumber() + k] = 0.0;
							}
						}
				}
			}

		}
		else{
			KRATOS_WATCH("Error in the Rotation of the conditions!!!")
		}
	}

	// Checking whether it is normal element or penalty element
	bool IsPenalty(GeometryType& rGeometry) const
	{
		bool is_penalty = false;
		bool is_lagrange = false;
		for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
		{
			if(this->IsSlip(rGeometry[itNode]) )
			{
				const double identifier = rGeometry[itNode].FastGetSolutionStepValue(mrFlagVariable);
				const double tolerance  = 1.e-6;
				if (identifier > 1.00 + tolerance && identifier < 2 + tolerance)
				{
					is_penalty = true;
					break;
				}
			}
		}

		return is_penalty;
	}
	bool IsLagrange(GeometryType& rGeometry) const
	{
		bool is_penalty = false;
		bool is_lagrange = false;
		for(unsigned int itNode = 0; itNode < rGeometry.PointsNumber(); ++itNode)
		{
			if(this->IsSlip(rGeometry[itNode]) )
			{
				const double identifier = rGeometry[itNode].FastGetSolutionStepValue(mrFlagVariable);
				const double tolerance  = 1.e-6;
				if (identifier > 2 + tolerance)
				{
					is_lagrange = true;
					break;
				}

			}
		}

		return is_lagrange;
	}

	/// Same functionalities as RotateVelocities, just to have a clear function naming
	virtual	void RotateDisplacements(ModelPart& rModelPart) const
	{
		this->RotateVelocities(rModelPart);
	}

	/// Transform nodal displacement to the rotated coordinates (aligned with each node's normal)
	/// The name is kept to be Rotate Velocities, since it is currently a derived class of coordinate_transformation_utilities in the core
	void RotateVelocities(ModelPart& rModelPart) const override
	{
		TLocalVectorType displacement(this->GetDomainSize());
		TLocalVectorType Tmp(this->GetDomainSize());
		TLocalVectorType lagrange(this->GetDomainSize());

		ModelPart::NodeIterator it_begin = rModelPart.NodesBegin();
		#pragma omp parallel for firstprivate(displacement,Tmp)
		for(int iii=0; iii<static_cast<int>(rModelPart.Nodes().size()); iii++)
		{
			ModelPart::NodeIterator itNode = it_begin+iii;
			if( this->IsSlip(*itNode) )
			{
				//this->RotationOperator<TLocalMatrixType>(Rotation,);
				if(this->GetDomainSize() == 3)
				{
					BoundedMatrix<double,3,3> rRot;
					this->LocalRotationOperatorPure(rRot,*itNode);

					array_1d<double,3>& rDisplacement = itNode->FastGetSolutionStepValue(DISPLACEMENT);
					for(unsigned int i = 0; i < 3; i++) displacement[i] = rDisplacement[i];
					noalias(Tmp) = prod(rRot,displacement);
					for(unsigned int i = 0; i < 3; i++) rDisplacement[i] = Tmp[i];
				}
				else
				{
					BoundedMatrix<double,2,2> rRot;
					this->LocalRotationOperatorPure(rRot,*itNode);

					array_1d<double,3>& rDisplacement = itNode->FastGetSolutionStepValue(DISPLACEMENT);
					for(unsigned int i = 0; i < 2; i++) displacement[i] = rDisplacement[i];
					noalias(Tmp) = prod(rRot,displacement);
					for(unsigned int i = 0; i < 2; i++) rDisplacement[i] = Tmp[i];

					if (itNode->SolutionStepsDataHas(VECTOR_LAGRANGE_MULTIPLIER)){
						array_1d<double,3>& rLagrange = itNode->FastGetSolutionStepValue(VECTOR_LAGRANGE_MULTIPLIER);
						for(unsigned int i = 0; i < 2; i++) lagrange[i] = rLagrange[i];
						noalias(Tmp) = prod(rRot,lagrange);
						for(unsigned int i = 0; i < 2; i++) rLagrange[i] = Tmp[i];
					}	
					
				}
			}
		}
	}

	/// Same functionalities as RecoverVelocities, just to have a clear function naming
	virtual void RecoverDisplacements(ModelPart& rModelPart) const
	{
		this->RecoverVelocities(rModelPart);
	}

	/// Transform nodal displacement from the rotated system to the original configuration
	/// The name is kept to be Recover Velocities, since it is currently a derived class of coordinate_transformation_utilities in the core
	void RecoverVelocities(ModelPart& rModelPart) const override
	{
		TLocalVectorType displacement(this->GetDomainSize());
		TLocalVectorType Tmp(this->GetDomainSize());
		TLocalVectorType lagrange(this->GetDomainSize());

		ModelPart::NodeIterator it_begin = rModelPart.NodesBegin();
		#pragma omp parallel for firstprivate(displacement,Tmp)
		for(int iii=0; iii<static_cast<int>(rModelPart.Nodes().size()); iii++)
		{
			ModelPart::NodeIterator itNode = it_begin+iii;
			if( this->IsSlip(*itNode) )
			{
				if(this->GetDomainSize() == 3)
				{
					BoundedMatrix<double,3,3> rRot;
					this->LocalRotationOperatorPure(rRot,*itNode);

					array_1d<double,3>& rDisplacement = itNode->FastGetSolutionStepValue(DISPLACEMENT);
					for(unsigned int i = 0; i < 3; i++) displacement[i] = rDisplacement[i];
					noalias(Tmp) = prod(trans(rRot),displacement);
					for(unsigned int i = 0; i < 3; i++) rDisplacement[i] = Tmp[i];
				}
				else
				{
					BoundedMatrix<double,2,2> rRot;
					this->LocalRotationOperatorPure(rRot,*itNode);

					array_1d<double,3>& rDisplacement = itNode->FastGetSolutionStepValue(DISPLACEMENT);
					for(unsigned int i = 0; i < 2; i++) displacement[i] = rDisplacement[i];
					noalias(Tmp) = prod(trans(rRot),displacement);
					for(unsigned int i = 0; i < 2; i++) rDisplacement[i] = Tmp[i];

					if (itNode->SolutionStepsDataHas(VECTOR_LAGRANGE_MULTIPLIER)){
						array_1d<double,3>& rLagrange = itNode->FastGetSolutionStepValue(VECTOR_LAGRANGE_MULTIPLIER);
						for(unsigned int i = 0; i < 2; i++) lagrange[i] = rLagrange[i];
						noalias(Tmp) = prod(trans(rRot),lagrange);
						for(unsigned int i = 0; i < 2; i++) rLagrange[i] = Tmp[i];
					}
				}
			}
		}
	}

	///@}
	///@name Access
	///@{

	///@}
	///@name Inquiry
	///@{

	///@}
	///@name Input and output
	///@{

	/// Turn back information as a string.
	std::string Info() const override
	{
		std::stringstream buffer;
		buffer << "MPMBoundaryRotationUtility";
		return buffer.str();
	}

	/// Print information about this object.
	void PrintInfo(std::ostream& rOStream) const override
	{
		rOStream << "MPMBoundaryRotationUtility";
	}

	/// Print object's data.
	void PrintData(std::ostream& rOStream) const override {}

	///@}
	///@name Friends
	///@{

	///@}

protected:
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

	const Variable<double>& mrFlagVariable;

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
};

///@}

///@name Type Definitions
///@{

///@}
///@name Input and output
///@{

/// input stream function
template<class TLocalMatrixType, class TLocalVectorType>
inline std::istream& operator >>(std::istream& rIStream,
		MPMBoundaryRotationUtility<TLocalMatrixType, TLocalVectorType>& rThis) {
	return rIStream;
}

/// output stream function
template<class TLocalMatrixType, class TLocalVectorType>
inline std::ostream& operator <<(std::ostream& rOStream,
		const MPMBoundaryRotationUtility<TLocalMatrixType, TLocalVectorType>& rThis) {
	rThis.PrintInfo(rOStream);
	rOStream << std::endl;
	rThis.PrintData(rOStream);

	return rOStream;
}

///@}

///@} addtogroup block

}

#endif // KRATOS_MPM_BOUNDARY_ROTATION_UTILITY