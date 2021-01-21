//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:        BSD License
//                  Kratos default license: kratos/license.txt
//
//  Main authors:   Riccardo Rossi
//                  Raul Bravo
//
#if !defined(KRATOS_ROM_BUILDER_AND_SOLVER)
#define KRATOS_ROM_BUILDER_AND_SOLVER

/* System includes */

/* External includes */

/* Project includes */
#include "includes/define.h"
#include "includes/model_part.h"
#include "solving_strategies/schemes/scheme.h"
#include "solving_strategies/builder_and_solvers/builder_and_solver.h"

/* Application includes */
#include "rom_application_variables.h"
#include "custom_utilities/rom_bases.h"

namespace Kratos
{

template <class TSparseSpace,
          class TDenseSpace,  // = DenseSpace<double>,
          class TLinearSolver //= LinearSolver<TSparseSpace,TDenseSpace>
          >
class ROMBuilderAndSolver : public BuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver>
{
public:
    /**
     * This struct is used in the component wise calculation only
     * is defined here and is used to declare a member variable in the component wise builder and solver
     * private pointers can only be accessed by means of set and get functions
     * this allows to set and not copy the Element_Variables and Condition_Variables
     * which will be asked and set by another strategy object
     */

    //pointer definition

    KRATOS_CLASS_POINTER_DEFINITION(ROMBuilderAndSolver);

    // The size_t types
    typedef std::size_t SizeType;
    typedef std::size_t IndexType;

    /// Definition of the classes from the base class
    typedef BuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver> BaseType;
    typedef typename BaseType::TSchemeType TSchemeType;
    typedef typename BaseType::TDataType TDataType;
    typedef typename BaseType::DofsArrayType DofsArrayType;
    typedef typename BaseType::TSystemMatrixType TSystemMatrixType;
    typedef typename BaseType::TSystemVectorType TSystemVectorType;
    typedef typename BaseType::LocalSystemVectorType LocalSystemVectorType;
    typedef typename BaseType::LocalSystemMatrixType LocalSystemMatrixType;
    typedef typename BaseType::TSystemMatrixPointerType TSystemMatrixPointerType;
    typedef typename BaseType::TSystemVectorPointerType TSystemVectorPointerType;
    typedef typename BaseType::NodesArrayType NodesArrayType;
    typedef typename BaseType::ElementsArrayType ElementsArrayType;
    typedef typename BaseType::ConditionsArrayType ConditionsArrayType;

    /// Additional definitions
    typedef PointerVectorSet<Element, IndexedObject> ElementsContainerType;
    typedef Element::EquationIdVectorType EquationIdVectorType;
    typedef Element::DofsVectorType DofsVectorType;
    typedef boost::numeric::ublas::compressed_matrix<double> CompressedMatrixType;

    /// DoF types definition
    typedef Node<3> NodeType;
    typedef typename NodeType::DofType DofType;
    typedef typename DofType::Pointer DofPointerType;

    /*@} */
    /**@name Life Cycle
     */
    /*@{ */

    /**
     * @brief Default constructor. (with parameters)
     */
    explicit ROMBuilderAndSolver(typename TLinearSolver::Pointer pNewLinearSystemSolver, Parameters ThisParameters)
        : BuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver>(pNewLinearSystemSolver)
    {
        // Validate default parameters
        Parameters default_parameters = Parameters(R"(
        {
            "nodal_unknowns" : [],
            "number_of_rom_dofs" : []
        })");

        ThisParameters.ValidateAndAssignDefaults(default_parameters);

        // We set the other member variables
        mpLinearSystemSolver = pNewLinearSystemSolver;

        mNodalVariablesNames = ThisParameters["nodal_unknowns"].GetStringArray();

        mNodalDofs = mNodalVariablesNames.size();
        for (int i=0;i<(ThisParameters["number_of_rom_dofs"]).size(); i++){
            mRomDofsVector.push_back(ThisParameters["number_of_rom_dofs"][i].GetInt());
        }
        for (int i=0;i<mRomDofsVector.size(); i++){
            //KRATOS_WATCH(mRomDofsVector.at(i))
        }

        // Setting up mapping: VARIABLE_KEY --> CORRECT_ROW_IN_BASIS
        for(int k=0; k<mNodalDofs; k++){
            if(KratosComponents<Variable<double>>::Has(mNodalVariablesNames[k]))
            {
                const auto& var = KratosComponents<Variable<double>>::Get(mNodalVariablesNames[k]);
                mMapPhi[var.Key()] = k;
            }
            else
                KRATOS_ERROR << "variable \""<< mNodalVariablesNames[k] << "\" not valid" << std::endl;

        }
    }

    /** Destructor.
     */
    ~ROMBuilderAndSolver() = default;

    virtual void SetUpDofSet(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart) override
    {
        KRATOS_TRY;

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 1 && rModelPart.GetCommunicator().MyPID() == 0)) << "Setting up the dofs" << std::endl;

        //Gets the array of elements from the modeler
        auto &r_elements_array = rModelPart.Elements();
        const int number_of_elements = static_cast<int>(r_elements_array.size());

        DofsVectorType dof_list, second_dof_list; // NOTE: The second dof list is only used on constraints to include master/slave relations

        unsigned int nthreads = OpenMPUtils::GetNumThreads();

        typedef std::unordered_set<NodeType::DofType::Pointer, DofPointerHasher> set_type;

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Number of threads " << nthreads << "\n" << std::endl;

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Initializing element loop" << std::endl;

        /**
         * Here we declare three sets.
         * - The global set: Contains all the DoF of the system
         * - The slave set: The DoF that are not going to be solved, due to MPC formulation
         */
        set_type dof_global_set;
        dof_global_set.reserve(number_of_elements * 20);


        if (mHromSimulation == false && mTimeStep == 0){
            int number_of_hrom_elements=0;
            #pragma omp parallel firstprivate(dof_list, second_dof_list) reduction(+:number_of_hrom_elements)
            {
                const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();

                // We create the temporal set and we reserve some space on them
                set_type dofs_tmp_set;
                dofs_tmp_set.reserve(20000);
                // Gets the array of elements from the modeler
                ModelPart::ElementsContainerType selected_elements_private;
                #pragma omp for schedule(guided, 512) nowait
                for (int i = 0; i < number_of_elements; ++i)
                {
                    auto it_elem = r_elements_array.begin() + i;
                    //detect whether the element has a Hyperreduced Weight (H-ROM simulation) or not (ROM simulation)
                    if ((it_elem)->Has(HROM_WEIGHT)){
                        selected_elements_private.push_back(*it_elem.base());
                        number_of_hrom_elements++;
                    }
                    else
                        it_elem->SetValue(HROM_WEIGHT, 1.0);
                    // Gets list of Dof involved on every element
                    pScheme->GetDofList(*it_elem, dof_list, r_current_process_info);
                    dofs_tmp_set.insert(dof_list.begin(), dof_list.end());
                }

                // Gets the array of conditions from the modeler
                ConditionsArrayType &r_conditions_array = rModelPart.Conditions();
                const int number_of_conditions = static_cast<int>(r_conditions_array.size());

                ModelPart::ConditionsContainerType selected_conditions_private;
                #pragma omp for schedule(guided, 512) nowait
                for (int i = 0; i < number_of_conditions; ++i)
                {
                    auto it_cond = r_conditions_array.begin() + i;
                    // Gather the H-reduced conditions that are to be considered for assembling. Ignoring those for displaying results only
                    if (it_cond->Has(HROM_WEIGHT)){
                        selected_conditions_private.push_back(*it_cond.base());
                        number_of_hrom_elements++;
                    }
                    else
                        it_cond->SetValue(HROM_WEIGHT, 1.0);
                    // Gets list of Dof involved on every element
                    pScheme->GetDofList(*it_cond, dof_list, r_current_process_info);
                    dofs_tmp_set.insert(dof_list.begin(), dof_list.end());
                }
                #pragma omp critical
                {
                    for (auto &cond : selected_conditions_private){
                        mSelectedConditions.push_back(&cond);
                    }
                    for (auto &elem : selected_elements_private){
                        mSelectedElements.push_back(&elem);
                    }

                }

                // Gets the array of constraints from the modeler
                auto &r_constraints_array = rModelPart.MasterSlaveConstraints();
                const int number_of_constraints = static_cast<int>(r_constraints_array.size());
                #pragma omp for schedule(guided, 512) nowait
                for (int i = 0; i < number_of_constraints; ++i)
                {
                    auto it_const = r_constraints_array.begin() + i;

                    // Gets list of Dof involved on every element
                    it_const->GetDofList(dof_list, second_dof_list, r_current_process_info);
                    dofs_tmp_set.insert(dof_list.begin(), dof_list.end());
                    dofs_tmp_set.insert(second_dof_list.begin(), second_dof_list.end());
                }

                // We merge all the sets in one thread
                #pragma omp critical
                {
                    dof_global_set.insert(dofs_tmp_set.begin(), dofs_tmp_set.end());
                }
            }
            if (number_of_hrom_elements>0){
                mHromSimulation = true;
            }
        }
        else{
            #pragma omp parallel firstprivate(dof_list, second_dof_list)
            {
                const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();

                // We cleate the temporal set and we reserve some space on them
                set_type dofs_tmp_set;
                dofs_tmp_set.reserve(20000);

                // Gets the array of elements from the modeler
                #pragma omp for schedule(guided, 512) nowait
                for (int i = 0; i < number_of_elements; ++i) {
                    auto it_elem = r_elements_array.begin() + i;

                    // Gets list of Dof involved on every element
                    pScheme->GetDofList(*it_elem, dof_list, r_current_process_info);
                    dofs_tmp_set.insert(dof_list.begin(), dof_list.end());
                }

                // Gets the array of conditions from the modeler
                ConditionsArrayType& r_conditions_array = rModelPart.Conditions();
                const int number_of_conditions = static_cast<int>(r_conditions_array.size());
                #pragma omp for  schedule(guided, 512) nowait
                for (int i = 0; i < number_of_conditions; ++i) {
                    auto it_cond = r_conditions_array.begin() + i;

                    // Gets list of Dof involved on every element
                    pScheme->GetDofList(*it_cond, dof_list, r_current_process_info);
                    dofs_tmp_set.insert(dof_list.begin(), dof_list.end());
                }

                // Gets the array of constraints from the modeler
                auto& r_constraints_array = rModelPart.MasterSlaveConstraints();
                const int number_of_constraints = static_cast<int>(r_constraints_array.size());
                #pragma omp for  schedule(guided, 512) nowait
                for (int i = 0; i < number_of_constraints; ++i) {
                    auto it_const = r_constraints_array.begin() + i;

                    // Gets list of Dof involved on every element
                    it_const->GetDofList(dof_list, second_dof_list, r_current_process_info);
                    dofs_tmp_set.insert(dof_list.begin(), dof_list.end());
                    dofs_tmp_set.insert(second_dof_list.begin(), second_dof_list.end());
                }

                // We merge all the sets in one thread
                #pragma omp critical
                {
                    dof_global_set.insert(dofs_tmp_set.begin(), dofs_tmp_set.end());
                }
            }
        }

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Initializing ordered array filling\n" << std::endl;

        DofsArrayType Doftemp;
        BaseType::mDofSet = DofsArrayType();

        Doftemp.reserve(dof_global_set.size());
        for (auto it = dof_global_set.begin(); it != dof_global_set.end(); it++)
        {
            Doftemp.push_back(*it);
        }
        Doftemp.Sort();

        BaseType::mDofSet = Doftemp;

        //Throws an exception if there are no Degrees Of Freedom involved in the analysis
        KRATOS_ERROR_IF(BaseType::mDofSet.size() == 0) << "No degrees of freedom!" << std::endl;

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "Number of degrees of freedom:" << BaseType::mDofSet.size() << std::endl;

        BaseType::mDofSetIsInitialized = true;
        if (BaseType::mDofSetIsInitialized ==true)
            mTimeStep++;

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 2 && rModelPart.GetCommunicator().MyPID() == 0)) << "Finished setting up the dofs" << std::endl;

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 2)) << "End of setup dof set\n"
                                                                          << std::endl;

#ifdef KRATOS_DEBUG
        // If reactions are to be calculated, we check if all the dofs have reactions defined
        // This is tobe done only in debug mode
        if (BaseType::GetCalculateReactionsFlag())
        {
            for (auto dof_iterator = BaseType::mDofSet.begin(); dof_iterator != BaseType::mDofSet.end(); ++dof_iterator)
            {
                KRATOS_ERROR_IF_NOT(dof_iterator->HasReaction()) << "Reaction variable not set for the following : " << std::endl
                                                                 << "Node : " << dof_iterator->Id() << std::endl
                                                                 << "Dof : " << (*dof_iterator) << std::endl
                                                                 << "Not possible to calculate reactions." << std::endl;
            }
        }
#endif
        KRATOS_CATCH("");
    }

    /**
            organises the dofset in order to speed up the building phase
     */
    virtual void SetUpSystem(
        ModelPart &r_model_part
    ) override
    {
        //int free_id = 0;
        BaseType::mEquationSystemSize = BaseType::mDofSet.size();
        int ndofs = static_cast<int>(BaseType::mDofSet.size());

        #pragma omp parallel for firstprivate(ndofs)
        for (int i = 0; i < static_cast<int>(ndofs); i++){
            typename DofsArrayType::iterator dof_iterator = BaseType::mDofSet.begin() + i;
            dof_iterator->SetEquationId(i);
        }
    }

    void SetUpBases(RomBases ThisBases){
        mRomBases = ThisBases;
    }

    void SetUpDistances(DistanceToClusters ThisDistances){
        mDistanceToClusters = ThisDistances;
        //this->UpdateCurrentCluster();
    }

    Vector ProjectToReducedBasis()
    {
        const auto dofs_begin = BaseType::mDofSet.begin();
        const auto dofs_number = BaseType::mDofSet.size();
        Vector q = ZeroVector(mRomDofs);

        #pragma omp parallel firstprivate(dofs_begin, dofs_number)
        {
            Vector temp_q = ZeroVector(mRomDofs);
            for (unsigned int k = 0; k<dofs_number; k++){
                auto dof = dofs_begin + k;
                //KRATOS_WATCH(dof->GetSolutionStepValue(0));//current solution
                //KRATOS_WATCH(dof->GetSolutionStepValue(1));//prior solution
                temp_q +=  (dof->GetSolutionStepValue(0) - dof->GetSolutionStepValue(1)) *row(  *mRomBases.GetBasis(mDistanceToClusters.GetCurrentCluster())->GetNodalBasis(dof->Id()),\
                                                                                        mMapPhi[dof->GetVariable().Key()]   ) ;  // Delta_q = Phi^T * Delta_u

            }

            #pragma omp critical
            {
                noalias(q) +=temp_q;
            }
        }
        return q;
    }

    void ProjectToFineBasis(
        const TSystemVectorType &rRomUnkowns,
        TSystemVectorType &Dx)
    {
        const auto dofs_begin = BaseType::mDofSet.begin();
        const auto dofs_number = BaseType::mDofSet.size();

        #pragma omp parallel firstprivate(dofs_begin, dofs_number)
        {
            #pragma omp for nowait
            for (unsigned int k = 0; k<dofs_number; k++){
                auto dof = dofs_begin + k;
                // bool print_this_quantity = false;
                // for(int i = 0; i< NodesToPrint.size(); i++){
                //     if (dof->Id() == NodesToPrint.at(i)){
                //         print_this_quantity = true;
                //     }

                // }
                // if (print_this_quantity){
                //     //KRATOS_WATCH(dof->Id())
                //     //KRATOS_WATCH(mDistanceToClusters.GetCurrentCluster())
                //     //KRATOS_WATCH(pcurrent_rom_nodal_basis)
                // }
                Dx[dof->EquationId()] = inner_prod(  row(  *mRomBases.GetBasis(mDistanceToClusters.GetCurrentCluster())->GetNodalBasis(dof->Id()),\
                                                     mMapPhi[dof->GetVariable().Key()]   )     , rRomUnkowns);
            }
        }
    }


    int GetCurrentCluster(){
        return mDistanceToClusters.GetCurrentCluster();
    }



    void GetPhiElemental(
        Matrix &PhiElemental,
        const Element::DofsVectorType &dofs,
        const Element::GeometryType &geom,
        int element_id)
    {
        int counter = 0;
        for(int k = 0; k < dofs.size(); ++k){
            auto variable_key = dofs[k]->GetVariable().Key();
            if (dofs[k]->IsFixed())
                noalias(row(PhiElemental, k)) = ZeroVector(PhiElemental.size2());
            else
                noalias(row(PhiElemental, k)) = row(*mRomBases.GetBasis(mDistanceToClusters.GetCurrentCluster())->GetNodalBasis(dofs[k]->Id()), mMapPhi[variable_key]);
        }
        bool print_this_quantity = false;

        for(int i = 0; i< ElementsToPrint.size(); i++){
            if ((element_id) == ElementsToPrint.at(i)){
                print_this_quantity = true;
            }
        }

        //if (print_this_quantity){
            //std::cout<<"\n\n\n\n"<<std::endl;
            //KRATOS_WATCH(element_id)
            //for(int i=0;i<PhiElemental.size1();i++){
            //    for(int j=0;j<PhiElemental.size2();j++){
                    //std::cout<<PhiElemental(i,j)<<std::endl;
            //    }
            //}
            //KRATOS_WATCH(PhiElemental)
            //std::cout<<"\n\n\n\n"<<std::endl;
        //}
    }

    void UpdateZMatrix(){
        Deltaq = this->ProjectToReducedBasis();
        mDistanceToClusters.UpdateZMatrix(Deltaq);
    }

    void UpdateCurrentCluster(){
        mDistanceToClusters.UpdateCurrentCluster();
        mRomDofs = mRomDofsVector.at(mDistanceToClusters.GetCurrentCluster());
    }

    void HardSetCurrentCluster(int this_index){
        mDistanceToClusters.HardSetCurrentCluster(this_index);
    }


    Vector GetCurrentReducedCoefficients(){
        return Deltaq;
    }


    Vector GetCurrentFullDimensionalVector(){
        if (just_a_counter==0){
            CurrentFullDimensionalVector = ZeroVector(BaseType::mDofSet.size());
            this->ProjectToFineBasis(Deltaq , CurrentFullDimensionalVector);
        }
        else{
            Vector dummy = ZeroVector(BaseType::mDofSet.size());
            this->ProjectToFineBasis(Deltaq, dummy);
            CurrentFullDimensionalVector+= dummy;
        }
        just_a_counter++;
        return CurrentFullDimensionalVector;
    }

    void SetNodeToPrint(int this_node_id){
        NodesToPrint.push_back(this_node_id);
    }

    void SetElementToPrint(int this_element_id){
        ElementsToPrint.push_back(this_element_id);
    }

    /*@{ */

    /**
            Function to perform the building and solving phase at the same time.
            It is ideally the fastest and safer function to use when it is possible to solve
            just after building
     */
    virtual void BuildAndSolve(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &Dx,
        TSystemVectorType &b) override
    {
        //define a dense matrix to hold the reduced problem
        Matrix Arom = ZeroMatrix(mRomDofs, mRomDofs);
        Vector brom = ZeroVector(mRomDofs);
        double project_to_reduced_start = OpenMPUtils::GetCurrentTime();

        Vector dq = ZeroVector(mRomDofs);

        // if (! Deltaq_initialized){
        //     Deltaq = this->ProjectToReducedBasis();
        //     Deltaq_initialized = true;
        // }

        const double project_to_reduced_end = OpenMPUtils::GetCurrentTime();
        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() >= 1 && rModelPart.GetCommunicator().MyPID() == 0)) << "Project to reduced basis time: " << project_to_reduced_end - project_to_reduced_start << std::endl;

        //build the system matrix by looping over elements and conditions and assembling to A
        KRATOS_ERROR_IF(!pScheme) << "No scheme provided!" << std::endl;

        const ProcessInfo& CurrentProcessInfo = rModelPart.GetProcessInfo();

        // Getting the elements from the model
        auto help_nelements = static_cast<int>(rModelPart.Elements().size());
        auto help_el_begin = rModelPart.ElementsBegin();

        auto help_cond_begin = rModelPart.ConditionsBegin();
        auto help_nconditions = static_cast<int>(rModelPart.Conditions().size());

        if ( mHromSimulation == true){
            // Only selected conditions are considered for the calculation on an H-ROM simualtion.
            help_cond_begin = mSelectedConditions.begin();
            help_nconditions = static_cast<int>(mSelectedConditions.size());

            help_el_begin = mSelectedElements.begin();
            help_nelements = static_cast<int>(mSelectedElements.size());
        }

        // Getting the array of the conditions
        const auto cond_begin = help_cond_begin;
        const auto nconditions = help_nconditions;
        const auto nelements = help_nelements;
        const auto el_begin = help_el_begin;

        //contributions to the system
        LocalSystemMatrixType LHS_Contribution = LocalSystemMatrixType(0, 0);
        LocalSystemVectorType RHS_Contribution = LocalSystemVectorType(0);

        //vector containing the localization in the system of the different terms
        Element::EquationIdVectorType EquationId;

        // assemble all elements
        double start_build = OpenMPUtils::GetCurrentTime();

        #pragma omp parallel firstprivate(nelements, nconditions, LHS_Contribution, RHS_Contribution, EquationId, el_begin, cond_begin)
        {
            Matrix PhiElemental;
            Matrix tempA = ZeroMatrix(mRomDofs,mRomDofs);
            Vector tempb = ZeroVector(mRomDofs);
            Matrix aux;

            #pragma omp for nowait
            for (int k = 0; k < nelements; k++)
            {
                auto it_el = el_begin + k;
                //detect if the element is active or not. If the user did not make any choice the element
                //is active by default
                bool element_is_active = true;
                if ((it_el)->IsDefined(ACTIVE))
                    element_is_active = (it_el)->Is(ACTIVE);

                if (element_is_active){
                    //calculate elemental contribution
                    pScheme->CalculateSystemContributions(*it_el, LHS_Contribution, RHS_Contribution, EquationId, CurrentProcessInfo);
                    Element::DofsVectorType dofs;
                    it_el->GetDofList(dofs, CurrentProcessInfo);
                    const auto &geom = it_el->GetGeometry();
                    if(PhiElemental.size1() != dofs.size() || PhiElemental.size2() != mRomDofs)
                        PhiElemental.resize(dofs.size(), mRomDofs,false);
                    if(aux.size1() != dofs.size() || aux.size2() != mRomDofs)
                        aux.resize(dofs.size(), mRomDofs,false);
                    GetPhiElemental(PhiElemental, dofs, geom, k+1);
                    noalias(aux) = prod(LHS_Contribution, PhiElemental);
                    double h_rom_weight = it_el->GetValue(HROM_WEIGHT);
                    noalias(tempA) += prod(trans(PhiElemental), aux) * h_rom_weight;
                    noalias(tempb) += prod(trans(PhiElemental), RHS_Contribution) * h_rom_weight;
                }
            }

            #pragma omp for nowait
            for (int k = 0; k < nconditions; k++){
                auto it = cond_begin + k;

                //detect if the element is active or not. If the user did not make any choice the condition
                //is active by default
                bool condition_is_active = true;
                if ((it)->IsDefined(ACTIVE))
                    condition_is_active = (it)->Is(ACTIVE);
                if (condition_is_active){
                    Condition::DofsVectorType dofs;
                    it->GetDofList(dofs, CurrentProcessInfo);
                    //calculate elemental contribution
                    pScheme->CalculateSystemContributions(*it, LHS_Contribution, RHS_Contribution, EquationId, CurrentProcessInfo);
                    const auto &geom = it->GetGeometry();
                    if(PhiElemental.size1() != dofs.size() || PhiElemental.size2() != mRomDofs)
                        PhiElemental.resize(dofs.size(), mRomDofs,false);
                    if(aux.size1() != dofs.size() || aux.size2() != mRomDofs)
                        aux.resize(dofs.size(), mRomDofs,false);
                    GetPhiElemental(PhiElemental, dofs, geom, k);
                    noalias(aux) = prod(LHS_Contribution, PhiElemental);
                    double h_rom_weight = it->GetValue(HROM_WEIGHT);
                    noalias(tempA) += prod(trans(PhiElemental), aux) * h_rom_weight;
                    noalias(tempb) += prod(trans(PhiElemental), RHS_Contribution) * h_rom_weight;
                }
            }

            #pragma omp critical
            {
                noalias(Arom) +=tempA;
                noalias(brom) +=tempb;
            }

        }

        const double stop_build = OpenMPUtils::GetCurrentTime();
        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() >= 1 && rModelPart.GetCommunicator().MyPID() == 0)) << "Build time: " << stop_build - start_build << std::endl;

        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() > 2 && rModelPart.GetCommunicator().MyPID() == 0)) << "Finished parallel building" << std::endl;


        //solve for the rom unkowns dunk = Arom^-1 * brom
        double start_solve = OpenMPUtils::GetCurrentTime();
        MathUtils<double>::Solve(Arom, dq, brom);
        // Deltaq += dq;
        // KRATOS_WATCH(dq)
        // KRATOS_WATCH(Deltaq);

        const double stop_solve = OpenMPUtils::GetCurrentTime();
        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() >= 1 && rModelPart.GetCommunicator().MyPID() == 0)) << "Solve reduced system time: " << stop_solve - start_solve << std::endl;

        // project reduced solution back to full order model
        double project_to_fine_start = OpenMPUtils::GetCurrentTime();
        ProjectToFineBasis(dq, Dx);
        const double project_to_fine_end = OpenMPUtils::GetCurrentTime();
        KRATOS_INFO_IF("ROMBuilderAndSolver", (this->GetEchoLevel() >= 1 && rModelPart.GetCommunicator().MyPID() == 0)) << "Project to fine basis time: " << project_to_fine_end - project_to_fine_start << std::endl;
    }

    void ResizeAndInitializeVectors(
        typename TSchemeType::Pointer pScheme,
        TSystemMatrixPointerType &pA,
        TSystemVectorPointerType &pDx,
        TSystemVectorPointerType &pb,
        ModelPart &rModelPart) override
    {
        KRATOS_TRY
        if (pA == NULL) //if the pointer is not initialized initialize it to an empty matrix
        {
            TSystemMatrixPointerType pNewA = TSystemMatrixPointerType(new TSystemMatrixType(0, 0));
            pA.swap(pNewA);
        }
        if (pDx == NULL) //if the pointer is not initialized initialize it to an empty matrix
        {
            TSystemVectorPointerType pNewDx = TSystemVectorPointerType(new TSystemVectorType(0));
            pDx.swap(pNewDx);
        }
        if (pb == NULL) //if the pointer is not initialized initialize it to an empty matrix
        {
            TSystemVectorPointerType pNewb = TSystemVectorPointerType(new TSystemVectorType(0));
            pb.swap(pNewb);
        }

        TSystemVectorType &Dx = *pDx;
        TSystemVectorType &b = *pb;

        if (Dx.size() != BaseType::mEquationSystemSize)
            Dx.resize(BaseType::mEquationSystemSize, false);
        if (b.size() != BaseType::mEquationSystemSize)
            b.resize(BaseType::mEquationSystemSize, false);

        KRATOS_CATCH("")
    }

    /*@} */
    /**@name Operations */
    /*@{ */

    /*@} */
    /**@name Access */
    /*@{ */

    /*@} */
    /**@name Inquiry */
    /*@{ */

    ///@}
    ///@name Input and output
    ///@{

    /// Turn back information as a string.
    virtual std::string Info() const override
    {
        return "ROMBuilderAndSolver";
    }

    /// Print information about this object.
    virtual void PrintInfo(std::ostream &rOStream) const override
    {
        rOStream << Info();
    }

    /// Print object's data.
    virtual void PrintData(std::ostream &rOStream) const override
    {
        rOStream << Info();
    }

    /*@} */
    /**@name Friends */
    /*@{ */

    /*@} */

protected:
    /**@name Protected static Member Variables */
    /*@{ */

    /*@} */
    /**@name Protected member Variables */
    /*@{ */

    /** Pointer to the Model.
     */
    typename TLinearSolver::Pointer mpLinearSystemSolver;

    //DofsArrayType mDofSet;
    std::vector<DofPointerType> mDofList;

    bool mReshapeMatrixFlag = false;

    /// flag taking care if the dof set was initialized ot not
    bool mDofSetIsInitialized = false;

    /// flag taking in account if it is needed or not to calculate the reactions
    bool mCalculateReactionsFlag = false;

    /// number of degrees of freedom of the problem to be solve
    unsigned int mEquationSystemSize;
    /*@} */
    /**@name Protected Operators*/
    /*@{ */

    int mEchoLevel = 0;

    TSystemVectorPointerType mpReactionsVector;

    std::vector<std::string> mNodalVariablesNames;
    int mNodalDofs;
    std::vector<int> mRomDofsVector;
    unsigned int mRomDofs;
    std::unordered_map<Kratos::VariableData::KeyType,int> mMapPhi;
    ModelPart::ConditionsContainerType mSelectedConditions;
    ModelPart::ElementsContainerType mSelectedElements;
    bool mHromSimulation = false;
    int mTimeStep = 0;
    RomBases mRomBases;
    DistanceToClusters mDistanceToClusters;
    Vector Deltaq;
    bool Deltaq_initialized = false;
    int just_a_counter = 0;
    Vector CurrentFullDimensionalVector;
    std::vector<int> NodesToPrint;
    std::vector<int> ElementsToPrint;


    /*@} */
    /**@name Protected Operations*/
    /*@{ */

    /*@} */
    /**@name Protected  Access */
    /*@{ */

    /*@} */
    /**@name Protected Inquiry */
    /*@{ */

    /*@} */
    /**@name Protected LifeCycle */
    /*@{ */

    /*@} */

private:
    /**@name Static Member Variables */
    /*@{ */

    /*@} */
    /**@name Member Variables */
    /*@{ */

    /*@} */
    /**@name Private Operators*/
    /*@{ */

    /*@} */
    /**@name Private Operations*/
    /*@{ */

    /*@} */
    /**@name Private  Access */
    /*@{ */

    /*@} */
    /**@name Private Inquiry */
    /*@{ */

    /*@} */
    /**@name Un accessible methods */
    /*@{ */

    /*@} */

}; /* Class ROMBuilderAndSolver */

/*@} */

/**@name Type Definitions */
/*@{ */

/*@} */

} /* namespace Kratos.*/

#endif /* KRATOS_ROM_BUILDER_AND_SOLVER  defined */
