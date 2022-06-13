//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:        BSD License
//                  Kratos default license: kratos/license.txt
//
//  Main authors:   Sebastian Ares de Parga Regalado
//

#if !defined(KRATOS_PETROV_GALERKIN_ROM_BUILDER_AND_SOLVER)
#define KRATOS_PETROV_GALERKIN_ROM_BUILDER_AND_SOLVER

/* System includes */

/* External includes */
#include "concurrentqueue/concurrentqueue.h"

/* Project includes */
#include "includes/define.h"
#include "includes/model_part.h"
#include "solving_strategies/schemes/scheme.h"
#include "solving_strategies/builder_and_solvers/builder_and_solver.h"
#include "custom_strategies/rom_builder_and_solver.h"
#include "utilities/builtin_timer.h"
#include "utilities/reduction_utilities.h"
#include "utilities/dense_householder_qr_decomposition.h"

/* Application includes */
#include "rom_application_variables.h"
#include "custom_utilities/rom_auxiliary_utilities.h"

namespace Kratos
{

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

template <class TSparseSpace, class TDenseSpace, class TLinearSolver>
class PetrovGalerkinROMBuilderAndSolver : public ROMBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver>
{
public:

    //TODO: UPDATE THIS
    /**
     * This struct is used in the component wise calculation only
     * is defined here and is used to declare a member variable in the component wise builder and solver
     * private pointers can only be accessed by means of set and get functions
     * this allows to set and not copy the Element_Variables and Condition_Variables
     * which will be asked and set by another strategy object
     */

    ///@name Type Definitions
    ///@{

    // Class pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(PetrovGalerkinROMBuilderAndSolver);

    // The size_t types
    typedef std::size_t SizeType;
    typedef std::size_t IndexType;

    /// The definition of the current class
    typedef PetrovGalerkinROMBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver> ClassType;

    /// Definition of the classes from the base class
    typedef BuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver> BaseType;
    typedef typename BaseType::TSchemeType TSchemeType;
    typedef typename BaseType::DofsArrayType DofsArrayType;
    typedef typename BaseType::TSystemMatrixType TSystemMatrixType;
    typedef typename BaseType::TSystemVectorType TSystemVectorType;
    typedef typename BaseType::LocalSystemVectorType LocalSystemVectorType;
    typedef typename BaseType::LocalSystemMatrixType LocalSystemMatrixType;
    typedef typename BaseType::TSystemMatrixPointerType TSystemMatrixPointerType;
    typedef typename BaseType::TSystemVectorPointerType TSystemVectorPointerType;
    typedef typename BaseType::ElementsArrayType ElementsArrayType;
    typedef typename BaseType::ConditionsArrayType ConditionsArrayType;

    /// Additional definitions
    typedef typename ModelPart::MasterSlaveConstraintContainerType MasterSlaveConstraintContainerType;
    typedef Element::EquationIdVectorType EquationIdVectorType;
    typedef Element::DofsVectorType DofsVectorType;
    typedef boost::numeric::ublas::compressed_matrix<double> CompressedMatrixType;

    // Non-distributed, dense:
    typedef LocalSystemMatrixType RomSystemMatrixType;
    typedef LocalSystemVectorType RomSystemVectorType;

    //Distributed, dense
    typedef RomSystemMatrixType PetrovGalerkinSystemMatrixType; 
    typedef RomSystemVectorType PetrovGalerkinSystemVectorType;
    //      ^ Change this to a distributed dense type

    /// DoF types definition
    typedef Node<3> NodeType;
    typedef typename NodeType::DofType DofType;
    typedef typename DofType::Pointer DofPointerType;
    typedef moodycamel::ConcurrentQueue<DofType::Pointer> DofQueue;

    ///@}
    ///@name Life cycle
    ///@{

    explicit PetrovGalerkinROMBuilderAndSolver(
        typename TLinearSolver::Pointer pNewLinearSystemSolver,
        Parameters ThisParameters)
        : ROMBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver>(pNewLinearSystemSolver, ThisParameters) 
    {
        // Validate and assign defaults
        // Parameters this_parameters_copy = ThisParameters.Clone();
        // this_parameters_copy = this->ValidateAndAssignParameters(this_parameters_copy, this->GetDefaultParameters());
        // this->AssignSettings(this_parameters_copy);
    }

    ~PetrovGalerkinROMBuilderAndSolver() = default;

    ///@}
    ///@name Operators
    ///@{


    ///@}
    ///@name Operations
    ///@{

    void SetUpDofSet(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart) override
    {
        KRATOS_TRY;

        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 1)) << "Setting up the dofs" << std::endl;
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Number of threads" << ParallelUtilities::GetNumThreads() << "\n" << std::endl;
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Initializing element loop" << std::endl;

        // Get model part data
        if (this->mHromWeightsInitialized == false) {
            this->InitializeHROMWeights(rModelPart);
        }

        auto dof_queue = this->ExtractDofSet(pScheme, rModelPart);

        // Fill a sorted auxiliary array of with the DOFs set
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Initializing ordered array filling\n" << std::endl;
        auto dof_array = this->SortAndRemoveDuplicateDofs(dof_queue);

        // Update base builder and solver DOFs array and set corresponding flag
        BaseType::GetDofSet().swap(dof_array);
        BaseType::SetDofSetIsInitializedFlag(true);

        // Throw an exception if there are no DOFs involved in the analysis
        KRATOS_ERROR_IF(BaseType::GetDofSet().size() == 0) << "No degrees of freedom!" << std::endl;
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Number of degrees of freedom:" << BaseType::GetDofSet().size() << std::endl;
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Finished setting up the dofs" << std::endl;

#ifdef KRATOS_DEBUG
        // If reactions are to be calculated, we check if all the dofs have reactions defined
        if (BaseType::GetCalculateReactionsFlag())
        {
            for (const auto& r_dof: BaseType::GetDofSet())
            {
                KRATOS_ERROR_IF_NOT(r_dof.HasReaction())
                    << "Reaction variable not set for the following :\n"
                    << "Node : " << r_dof.Id() << '\n'
                    << "Dof  : " << r_dof      << '\n'
                    << "Not possible to calculate reactions." << std::endl;
            }
        }
#endif
        KRATOS_CATCH("");
    } 
    
    void BuildAndSolve(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &Dx,
        TSystemVectorType &b) override
    {
        KRATOS_TRY
        PetrovGalerkinSystemMatrixType Arom = ZeroMatrix(BaseType::GetEquationSystemSize(), this->GetNumberOfROMModes());
        PetrovGalerkinSystemVectorType brom = ZeroVector(BaseType::GetEquationSystemSize());
        BuildROM(pScheme, rModelPart, Arom, brom);
        SolveROM(rModelPart, Arom, brom, Dx);


        KRATOS_CATCH("")
    }

    Parameters GetDefaultParameters() const override
    {
        Parameters default_parameters = Parameters(R"(
        {
            "name" : "petrov_galerkin_rom_builder_and_solver",
            "nodal_unknowns" : [],
            "number_of_rom_dofs" : 10
        })");
        default_parameters.AddMissingParameters(BaseType::GetDefaultParameters());

        return default_parameters;
    }

    static std::string Name() 
    {
        return "petrov_galerkin_rom_builder_and_solver";
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
    virtual std::string Info() const override
    {
        return "PetrovGalerkinROMBuilderAndSolver";
    }

    /// Print information about this object.
    virtual void PrintInfo(std::ostream &rOStream) const override
    {
        rOStream << Info();
    }

    /// Print object's data.GetNumberOfROMModes()
    virtual void PrintData(std::ostream &rOStream) const override
    {
        rOStream << Info();
    }

    ///@}
    ///@name Friends
    ///@{


    ///@}
protected:
    ///@}
    ///@name Protected static member variables
    ///@{


    ///@}
    ///@name Protected member variables
    ///@{

    ///@}
    ///@name Protected operators
    ///@{


    ///@}
    ///@name Protected operations
    ///@{

    /**
    * Thread Local Storage containing dynamically allocated structures to avoid reallocating each iteration.
    */
    struct AssemblyTLS
    {
        Matrix phiE = {};                // Elemental Phi
        LocalSystemMatrixType lhs = {};  // Elemental LHS
        EquationIdVectorType eq_id = {}; // Elemental equation ID vector
        DofsVectorType dofs = {};        // Elemental dof vector
        RomSystemMatrixType romA;        // reduced LHS
        RomSystemVectorType romB;        // reduced RHS
    };

    /**
     * Resizes a Matrix if it's not the right size
     */
    template<typename TMatrix>
    static void ResizeIfNeeded(TMatrix& mat, const SizeType rows, const SizeType cols)
    {
        if(mat.size1() != rows || mat.size2() != cols) {
            mat.resize(rows, cols, false);
        }
    };

    /**
     * Builds the reduced system of equations on rank 0 
     */
    void BuildROM(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart,
        PetrovGalerkinSystemMatrixType &rA,
        PetrovGalerkinSystemVectorType &rb) override
    {
        KRATOS_TRY
        // Define a dense matrix to hold the reduced problem
        rA = ZeroMatrix(BaseType::GetEquationSystemSize(), this->GetNumberOfROMModes());
        rb = ZeroVector(BaseType::GetEquationSystemSize());

        // Build the system matrix by looping over elements and conditions and assembling to A
        KRATOS_ERROR_IF(!pScheme) << "No scheme provided!" << std::endl;

        // Get ProcessInfo from main model part
        const auto& r_current_process_info = rModelPart.GetProcessInfo();


        // Assemble all entities
        const auto assembling_timer = BuiltinTimer();

        AssemblyTLS assembly_tls_container;

        auto& elements = rModelPart.Elements();
        if(!elements.empty())
        {
            block_for_each(elements, assembly_tls_container, 
                [&](Element& r_element, AssemblyTLS& r_thread_prealloc)
            {
                CalculateLocalContributionPetrovGalerkin(r_element, rA, rb, r_thread_prealloc, *pScheme, r_current_process_info);
            });
        }


        auto& conditions = rModelPart.Conditions();
        if(!conditions.empty())
        {
            block_for_each(conditions, assembly_tls_container, 
                [&](Condition& r_condition, AssemblyTLS& r_thread_prealloc)
            {
                CalculateLocalContributionPetrovGalerkin(r_condition, rA, rb, r_thread_prealloc, *pScheme, r_current_process_info);
            });
        }

        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 0)) << "Build time: " << assembling_timer.ElapsedSeconds() << std::endl;
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Finished parallel building" << std::endl;

        KRATOS_CATCH("")
    }

    /**
     * Solves reduced system of equations and broadcasts it
     */
    void SolveROM(
        ModelPart &rModelPart,
        PetrovGalerkinSystemMatrixType &rA,
        PetrovGalerkinSystemVectorType &rb,
        TSystemVectorType &rDx) override
    {
        KRATOS_TRY

        PetrovGalerkinSystemVectorType dxrom(this->GetNumberOfROMModes());
        
        const auto solving_timer = BuiltinTimer();
        // Calculate the QR decomposition
        DenseHouseholderQRDecomposition<TDenseSpace> qr_decomposition;
        //                              ^Correct after properly defining PetrovGalerkinSystemMatrixType
        qr_decomposition.Compute(rA);
        qr_decomposition.Solve(rb, dxrom);
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 0)) << "Solve reduced system time: " << solving_timer.ElapsedSeconds() << std::endl;

        // Save the ROM solution increment in the root modelpart database
        auto& r_root_mp = rModelPart.GetRootModelPart();
        noalias(r_root_mp.GetValue(ROM_SOLUTION_INCREMENT)) += dxrom;

        // project reduced solution back to full order model
        const auto backward_projection_timer = BuiltinTimer();
        this->ProjectToFineBasis(dxrom, rModelPart, rDx);
        KRATOS_INFO_IF("PetrovGalerkinROMBuilderAndSolver", (this->GetEchoLevel() > 0)) << "Project to fine basis time: " << backward_projection_timer.ElapsedSeconds() << std::endl;

        KRATOS_CATCH("")
    }

    ///@}
    ///@name Protected access
    ///@{


    ///@}
    ///@name Protected inquiry
    ///@{


    ///@}
    ///@name Protected life cycle
    ///@{
    
private:
    ///@}
    ///@name Private operations 
    ///@{

    /**
     * Computes the local contribution of an element or condition for PetrovGalerkin
     */
    template<typename TEntity>
    void CalculateLocalContributionPetrovGalerkin(
        TEntity& rEntity,
        PetrovGalerkinSystemMatrixType& rAglobal,
        PetrovGalerkinSystemVectorType& rBglobal,
        AssemblyTLS& rPreAlloc,
        TSchemeType& rScheme,
        const ProcessInfo& rCurrentProcessInfo)
    {
        if (rEntity.IsDefined(ACTIVE) && rEntity.IsNot(ACTIVE)) return;

        // Calculate elemental contribution
        rScheme.CalculateSystemContributions(rEntity, rPreAlloc.lhs, rPreAlloc.romB, rPreAlloc.eq_id, rCurrentProcessInfo);
        rEntity.GetDofList(rPreAlloc.dofs, rCurrentProcessInfo);

        const SizeType ndofs = rPreAlloc.dofs.size();
        ResizeIfNeeded(rPreAlloc.phiE, ndofs, this->GetNumberOfROMModes());
        ResizeIfNeeded(rPreAlloc.romA, ndofs, this->GetNumberOfROMModes());

        const auto &r_geom = rEntity.GetGeometry();
        RomAuxiliaryUtilities::GetPhiElemental(rPreAlloc.phiE, rPreAlloc.dofs, r_geom, this->mMapPhi);

        noalias(rPreAlloc.romA) = prod(rPreAlloc.lhs, rPreAlloc.phiE);


        // Assembly
        for(SizeType row=0; row < ndofs; ++row)
        {
            const SizeType global_row = rPreAlloc.eq_id[row];

            double& r_bi = rBglobal(global_row);
            AtomicAdd(r_bi, rPreAlloc.romB[row]);

            if(rPreAlloc.dofs[row]->IsFixed()) continue;

            for(SizeType col=0; col < this->GetNumberOfROMModes(); ++col)
            {
                // const SizeType global_col = rPreAlloc.eq_id[col];
                const SizeType global_col = col;
                double& r_Aij = rAglobal(global_row, global_col);
                AtomicAdd(r_Aij, rPreAlloc.romA(row, col));
            }
        }
    }


    ///@}
}; /* Class PetrovGalerkinROMBuilderAndSolver */

///@}
///@name Type Definitions
///@{


///@}

} /* namespace Kratos.*/

#endif /* KRATOS_PETROV_GALERKIN_ROM_BUILDER_AND_SOLVER  defined */
