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
#include "contact_angle_evaluator.h"
// See the header file

namespace Kratos
{

/* Public functions *******************************************************/

void ContactAngleEvaluator::Execute()
{
    KRATOS_TRY;

    const double theta_advancing = 180.0*PI/180.0;;//149.0*PI/180.0;//129.78*PI/
    const double theta_receding = 0.0*PI/180.0;;//115.0*PI/180.0;//129.78*PI/

    const unsigned int num_nodes = mrModelPart.NumberOfNodes();
    const unsigned int num_elements = mrModelPart.NumberOfElements();

    // Auxiliar containers
    Vector N, distances, solid_normal, gradient;
    Matrix InvJ0, J0, DN_DX;

    // First element iterator
    const auto it_element_begin = mrModelPart.ElementsBegin();

    // Current domain size
    const std::size_t dimension = mrModelPart.GetProcessInfo()[DOMAIN_SIZE];

    // Initial resize
    const auto& r_first_element_geometry = it_element_begin->GetGeometry();
    const std::size_t number_of_nodes_first_element = r_first_element_geometry.PointsNumber();
    const std::size_t local_space_dimension_first_element = r_first_element_geometry.LocalSpaceDimension();

    // Resize if needed
    DN_DX.resize(number_of_nodes_first_element, dimension);
    J0.resize(dimension, local_space_dimension_first_element);
    N.resize(number_of_nodes_first_element);
    distances.resize(number_of_nodes_first_element);
    solid_normal.resize(dimension);
    gradient.resize(dimension);

    // Iterate over the elements
    #pragma omp parallel for firstprivate(N, distances, DN_DX, J0, InvJ0, solid_normal, gradient)
    for(int i_elem=0; i_elem < num_elements; ++i_elem) {
        auto it_elem = it_element_begin + i_elem;

        it_elem->SetValue(CONTACT_ANGLE, 0.0);
        it_elem->SetValue(NORMAL_VECTOR, ZeroVector(3));

        auto& r_geometry = it_elem->GetGeometry();
        unsigned int n_contact_neg = 0;
        unsigned int n_contact_pos = 0;
        //unsigned int n_contact = 0;

        solid_normal = ZeroVector(dimension);

        for (std::size_t i_node = 0; i_node < number_of_nodes_first_element; ++i_node){
            auto& node = r_geometry[i_node];
            const double distance = node.FastGetSolutionStepValue(DISTANCE);
            distances[i_node] = distance;
            if (node.GetValue(IS_STRUCTURE) == 1.0 && distance > 0.0){
                n_contact_pos++;
                solid_normal += node.FastGetSolutionStepValue(NORMAL);
                //n_contact++;
            } else if (node.GetValue(IS_STRUCTURE) == 1.0){
                n_contact_neg++;
                solid_normal += node.FastGetSolutionStepValue(NORMAL);
                //n_contact++;
            }
        }

        if (n_contact_pos > 0 && n_contact_neg > 0){
            const double normal_norm = Kratos::norm_2(solid_normal);
            solid_normal = (1.0/normal_norm)*solid_normal;

            // // The integration points
            // const auto& r_integration_method = GeometryData::GI_GAUSS_1; //r_geometry.GetDefaultIntegrationMethod();
            // const auto& r_integration_points = r_geometry.IntegrationPoints(r_integration_method);
            // const std::size_t number_of_integration_points = r_integration_points.size();

            // double detJ0 = 0.0;

            // // The containers of the shape functions and the local gradients
            // // const Matrix& rNcontainer = r_geometry.ShapeFunctionsValues(r_integration_method);
            // const auto& rDN_DeContainer = r_geometry.ShapeFunctionsLocalGradients(r_integration_method);

            // for ( IndexType point_number = 0; point_number < number_of_integration_points; ++point_number ) {
            //     // Getting the shape functions
            //     // noalias(N) = row(rNcontainer, point_number);

            //     // Getting the jacobians and local gradients
            //     GeometryUtils::JacobianOnInitialConfiguration(r_geometry, r_integration_points[point_number], J0);
            //     MathUtils<double>::GeneralizedInvertMatrix(J0, InvJ0, detJ0);
            //     const Matrix& rDN_De = rDN_DeContainer[point_number];
            //     GeometryUtils::ShapeFunctionsGradients(rDN_De, InvJ0, DN_DX);

            //     gradient += prod(trans(DN_DX), distances);

            //     // const double gauss_point_volume = r_integration_points[point_number].Weight() * detJ0;
            // }

            for (std::size_t i_node = 0; i_node < number_of_nodes_first_element; ++i_node){
                auto& node = r_geometry[i_node];
                if (node.GetValue(IS_STRUCTURE) == 1.0){
                    gradient += node.FastGetSolutionStepValue(DISTANCE_GRADIENT);
                }
            }

            const double gradient_norm = Kratos::norm_2(gradient);
            gradient = (1.0/gradient_norm)*gradient;

            it_elem->GetValue(CONTACT_ANGLE) = PI - std::acos( inner_prod(solid_normal, gradient) );
            it_elem->GetValue(NORMAL_VECTOR) = gradient;
        }

    }

    // First node iterator
    const auto it_node_begin = mrModelPart.NodesBegin();

    #pragma omp parallel for
    for (unsigned int k = 0; k < num_nodes; ++k) {
        auto it_node = it_node_begin + k;

        //it_node->SetValue(CONTACT_ANGLE, 0.0);
        it_node->FastGetSolutionStepValue(CONTACT_ANGLE) = 0.0;
        it_node->FastGetSolutionStepValue(CONTACT_VELOCITY) = 0.0;
        it_node->Free(DISTANCE);

        double weight = 0.0;
        double avg_contact_angle = 0.0;
        Vector normal_avg = ZeroVector(3);

        auto& neighbour_elements = it_node->GetValue(NEIGHBOUR_ELEMENTS);

        for (auto i_element = neighbour_elements.begin(); i_element != neighbour_elements.end(); i_element++){
            const double elemental_contact_angle = i_element->GetValue(CONTACT_ANGLE);

            if (elemental_contact_angle > 1.0e-12){
                avg_contact_angle += elemental_contact_angle;
                normal_avg += i_element->GetValue(NORMAL_VECTOR);
                weight += 1.0;
            }
        }

        if (weight >= 1.0){
            const double contact_angle = avg_contact_angle/weight;
            it_node->FastGetSolutionStepValue(CONTACT_ANGLE) = contact_angle;
            const Vector normal = (1.0/norm_2(normal_avg)) * normal_avg;
            it_node->FastGetSolutionStepValue(NORMAL_VECTOR) = normal;

            //Vector velocity = it_node->FastGetSolutionStepValue(VELOCITY);
            //const double velocity_norm = Kratos::norm_2(velocity);
            //velocity = (1.0/velocity_norm)*velocity;

            const double distance_diff = it_node->FastGetSolutionStepValue(DISTANCE) - it_node->GetValue(DISTANCE_AUX);
            const int velocity_direction = (distance_diff < 0.0) - (distance_diff > 0.0);//inner_prod(velocity, normal);
            //it_node->FastGetSolutionStepValue(CONTACT_VELOCITY) = velocity_direction;

            if (it_node->GetValue(IS_STRUCTURE) == 1.0){
                /* if (velocity_direction > 0.0 && contact_angle < theta_advancing){
                    it_node->Fix(DISTANCE);
                } else if (velocity_direction < 0.0 && contact_angle > theta_receding){
                    it_node->Fix(DISTANCE);
                } else if (velocity_direction == 0.0 && contact_angle < theta_advancing && contact_angle > theta_receding){
                    it_node->Fix(DISTANCE);
                } */

                if ( ( !(velocity_direction <= 0 && contact_angle <= theta_receding) &&
                    !(velocity_direction >= 0 && contact_angle >= theta_advancing) ) /* ||
                    (velocity_direction == 0 && contact_angle < theta_advancing && contact_angle > theta_receding) */ ){ // this last OR condition is unnecessary!
                    it_node->Fix(DISTANCE);
                    it_node->FastGetSolutionStepValue(CONTACT_VELOCITY) = static_cast<double>(velocity_direction);
                }

                /* if (contact_angle < theta_advancing && contact_angle > theta_receding){
                    it_node->Fix(DISTANCE);
                } */
            }

        }
    }

    /* #pragma omp parallel for
    for (unsigned int i = 0; i < num_nodes; ++i) {

        auto it_node_i = it_node_begin + i;
        if (it_node_i->GetValue(IS_STRUCTURE) == 1.0 && it_node_i->Coordinates()[2] == 0.0){

            auto& node_i_contact_angle = it_node_i->FastGetSolutionStepValue(CONTACT_ANGLE);

            if (node_i_contact_angle == 0.0){

                double min_dist = 1.0e6;
                double min_dist_contact_angle;

                for (unsigned int j = 0; j < num_nodes; ++j) {

                    auto it_node_j = it_node_begin + j;
                    const double node_j_contact_angle = it_node_j->FastGetSolutionStepValue(CONTACT_ANGLE);

                    if (node_j_contact_angle > 1.0e-12){

                        const double nodal_dist = norm_2(it_node_i->Coordinates() - it_node_j->Coordinates());
                        if (nodal_dist < min_dist){
                            min_dist = nodal_dist;
                            min_dist_contact_angle = node_j_contact_angle;
                        }

                    }

                }

                node_i_contact_angle = min_dist_contact_angle;

            }
        }

    } */

    KRATOS_CATCH("");
}

/* Protected functions ****************************************************/

//void SurfaceSmoothingProcess::CheckAndStoreVariablesList(const std::vector<std::string>& rVariableStringArray){
//Nothing
//}

}; // namespace Kratos