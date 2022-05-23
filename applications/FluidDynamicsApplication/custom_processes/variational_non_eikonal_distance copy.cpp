//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    KratosAppGenerator
//
//

// System includes
// See the header file

// External includes
// See the header file

// Project includes
// See the header file

// Application includes
#include "variational_non_eikonal_distance.h"
// See the header file

namespace Kratos
{

/* Public functions *******************************************************/
VariationalNonEikonalDistance::VariationalNonEikonalDistance(
    ModelPart& rModelPart,
    TLinearSolver::Pointer plinear_solver)
    : Process(),
      mrModelPart(rModelPart)
{
    // Member variables initialization
    // Nothing!

    // Generate an auxilary model part and populate it by elements of type MySimpleElement
    CreateAuxModelPart();

    auto p_builder_solver = Kratos::make_shared<ResidualBasedBlockBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver> >(plinear_solver);
    //auto p_builder_solver = Kratos::make_unique<ResidualBasedBlockBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver> >(plinear_solver);

    InitializeSolutionStrategy(plinear_solver, p_builder_solver);

    mpGradientCalculator = Kratos::make_unique<ComputeGradientProcessType>(
        mrModelPart,
        DISTANCE_AUX,
        DISTANCE_GRADIENT,
        NODAL_AREA,
        false);
}

void VariationalNonEikonalDistance::CreateAuxModelPart()
{
    ModelPart::NodesContainerType& r_nodes = mrModelPart.Nodes();
    ModelPart::ElementsContainerType& r_elems = mrModelPart.Elements();

    // KRATOS_INFO("VariationalNonEikonalDistanceProcess") << "Here 1" << std::endl;

    Model& current_model = mrModelPart.GetModel();
    if(current_model.HasModelPart( mAuxModelPartName ))
        current_model.DeleteModelPart( mAuxModelPartName );

    // Adding DISTANCE_AUX2 and DISTANCE_GRADIENT_X,Y,Z to the solution variables is not needed if it is already a solution variable of the problem
    mrModelPart.AddNodalSolutionStepVariable(DISTANCE_AUX2);
    mrModelPart.AddNodalSolutionStepVariable(DISTANCE_GRADIENT);

    // Ensure that the nodes have distance as a DOF
    VariableUtils().AddDof<Variable<double> >(DISTANCE_AUX2, mrModelPart);

    // KRATOS_INFO("VariationalNonEikonalDistanceProcess") << "Here 2" << std::endl;

    // Ensure that the nodes have DISTANCE_AUX2 and DISTANCE_GRADIENT_X,Y,Z as a DOF !!!! NOT NEEDED HERE IF IT IS DONE IN THE PYTHON SCRIPT !!!!
    //#pragma omp parallel for
       for (int k = 0; k < static_cast<int>(mrModelPart.NumberOfNodes()); ++k) {
            auto it_node = mrModelPart.NodesBegin() + k;
            //it_node->AddDof(DISTANCE_AUX2);
            it_node->AddDof(DISTANCE_GRADIENT_X);
            it_node->AddDof(DISTANCE_GRADIENT_Y);
            it_node->AddDof(DISTANCE_GRADIENT_Z);
        }

    // KRATOS_INFO("VariationalNonEikonalDistanceProcess") << "Here 3" << std::endl;

    // Generate AuxModelPart
    ModelPart& r_distance_model_part = current_model.CreateModelPart( mAuxModelPartName );

    Element::Pointer p_distance_element = Kratos::make_intrusive<VariationalNonEikonalDistanceElement>();

    ConnectivityPreserveModeler modeler;
    modeler.GenerateModelPart(mrModelPart, r_distance_model_part, *p_distance_element);

    // KRATOS_INFO("VariationalNonEikonalDistanceProcess") << "Here 4" << std::endl;
}

void VariationalNonEikonalDistance::Execute()
{
    KRATOS_TRY;

    const unsigned int NumNodes = mrModelPart.NumberOfNodes();
    const unsigned int NumElements = mrModelPart.NumberOfElements();

    const unsigned int num_nodes = 4;

    auto it_elem = mrModelPart.ElementsBegin();
    auto& geom = it_elem->GetGeometry();
    const double elem_size = ElementSizeCalculator<3,4>::AverageElementSize(geom);

    //double distance_min = 1.0e10;
    //unsigned int node_nearest = 0;

    //double distance_max = 0.0;
    //unsigned int node_farthest = 0;

    // #pragma omp parallel for
    // for (unsigned int i_node = 0; i_node < NumNodes; ++i_node) {
    //     auto it_node = mrModelPart.NodesBegin() + i_node;
    //     it_node->Free(DISTANCE_AUX2);
    //     const double distance = it_node->FastGetSolutionStepValue(DISTANCE);
    //     it_node->FastGetSolutionStepValue(DISTANCE_AUX2) = distance;

    //     const double temp_X = it_node->FastGetSolutionStepValue(DISTANCE_GRADIENT_X);
    //     it_node->SetValue(DISTANCE_GRADIENT_X, temp_X);
    //     const double temp_Y = it_node->FastGetSolutionStepValue(DISTANCE_GRADIENT_Y);
    //     it_node->SetValue(DISTANCE_GRADIENT_Y, temp_Y);
    //     const double temp_Z = it_node->FastGetSolutionStepValue(DISTANCE_GRADIENT_Z);
    //     it_node->SetValue(DISTANCE_GRADIENT_Z, temp_Z);

        /* if (abs(distance) < distance_min){
            #pragma omp critical
            distance_min = abs(distance);
            #pragma omp critical
            node_nearest = i_node;
        } */

        /* if (abs(distance) >= distance_max){
            distance_max = abs(distance);
            //node_farthest = i_node;
        } */

        /* if (abs(distance) > 7*elem_size){
            it_node->Fix(DISTANCE_AUX2);
        } */
    //}

    /* for (auto it_cond = mrModelPart.ConditionsBegin(); it_cond != mrModelPart.ConditionsEnd(); ++it_cond){
       Geometry< Node<3> >& geom = it_cond->GetGeometry();

       for(unsigned int i=0; i<geom.size(); i++){
           geom[i].Fix(DISTANCE_AUX2);
       }
    } */

    //auto it_node = mrModelPart.NodesBegin() + node_nearest; //node_farthest;
    //it_node->Fix(DISTANCE_AUX2);
    //KRATOS_INFO("VariationalNonEikonalDistancem, fixed distance") << it_node->FastGetSolutionStepValue(DISTANCE) << std::endl;

    /* const double epsilon = 1.0e-15;
    #pragma omp parallel for
    for (unsigned int i_node = 0; i_node < NumNodes; ++i_node) {
        auto it_node = mrModelPart.NodesBegin() + i_node;
        const double distance = it_node->FastGetSolutionStepValue(DISTANCE);

        if (abs((abs(distance) - distance_min)/(distance_min + epsilon)) <= epsilon){ // (abs((distance - distance_max)/distance_max) <= 1.0e-9){
            it_node->Fix(DISTANCE_AUX2);
            //KRATOS_INFO("VariationalNonEikonalDistancem, fixed distance") << distance << std::endl;
        }
    } */

    //#pragma omp parallel for
    //for (int i_elem = 0; i_elem < NumElements; ++i_elem){

    //    auto it_elem = mrModelPart.ElementsBegin() + i_elem;

    //     array_1d<double,num_nodes> distances;

    //    auto& geom = it_elem->GetGeometry();

    //     double distance_min = 1.0e10;
    //     unsigned int node_nearest = 0;

    //     for (unsigned int i_node = 0; i_node < num_nodes; i_node++){

    //         const double distance = geom[i_node].FastGetSolutionStepValue(DISTANCE);
    //         distances[i_node] = distance;

    //         if (abs(distance) <= distance_min){
    //             distance_min = abs(distance);
    //             node_nearest = i_node;
    //         }
    //     }

        // unsigned int nneg=0, npos=0;
        // for (unsigned int i_node = 0; i_node < num_nodes; ++i_node)
        // {
        //     const double distance = geom[i_node].FastGetSolutionStepValue(DISTANCE);
        //     if (distance >= 1.0e-14) npos += 1;
        //     else if (distance <= -1.0e-14) nneg += 1;
        // }

        // if (npos != 0 && nneg != 0){
        //     for (unsigned int i_node = 0; i_node < num_nodes; ++i_node)
        //     {
        //         const double distance = geom[i_node].FastGetSolutionStepValue(DISTANCE);
        //         if (distance >= 0.0){
        //             geom[i_node].SetLock();
        //             geom[i_node].Fix(DISTANCE_AUX2);
        //             geom[i_node].UnSetLock();
        //         }
        //     }
    //         geom[node_nearest].SetLock();
    //         geom[node_nearest].Fix(DISTANCE_AUX2);
    //         geom[node_nearest].UnSetLock();
    //    }
    //}

    #pragma omp parallel for
    for (unsigned int k = 0; k < NumNodes; ++k) {
        auto it_node = mrModelPart.NodesBegin() + k;
        it_node->SetValue(NODAL_AREA, 0.0);

        const double distance = it_node->FastGetSolutionStepValue(DISTANCE);
        it_node->FastGetSolutionStepValue(DISTANCE_AUX2) = distance;

        if ( it_node->IsFixed(DISTANCE) ) {
            it_node->Fix(DISTANCE_AUX2);
        } else {
            it_node->Free(DISTANCE_AUX2);
        }

        it_node->Set(BOUNDARY,false);
    }

    #pragma omp parallel for
    for (auto it_cond = mrModelPart.ConditionsBegin(); it_cond != mrModelPart.ConditionsEnd(); ++it_cond){
        Geometry< Node<3> >& geom = it_cond->GetGeometry();
        for(unsigned int i=0; i<geom.size(); i++){
            geom[i].Set(BOUNDARY,true);
        }
    }

    KRATOS_INFO("VariationalNonEikonalDistance") << "About to solve the LSE" << std::endl;
    mp_solving_strategy->Solve();
    KRATOS_INFO("VariationalNonEikonalDistance") << "LSE is solved" << std::endl;

    KRATOS_CATCH("")
}

void VariationalNonEikonalDistance::ExecuteInitialize()
{
    KRATOS_TRY;
    KRATOS_CATCH("");
}

void VariationalNonEikonalDistance::ExecuteBeforeSolutionLoop() {
    this->ExecuteInitializeSolutionStep();
    this->ExecuteFinalizeSolutionStep();
}

void VariationalNonEikonalDistance::ExecuteInitializeSolutionStep() {
//Nothing
}

void VariationalNonEikonalDistance::ExecuteFinalizeSolutionStep() {
//Nothing
}

/* Protected functions ****************************************************/

//void VariationalNonEikonalDistance::CheckAndStoreVariablesList(const std::vector<std::string>& rVariableStringArray){
//Nothing
//}

}; // namespace Kratos
