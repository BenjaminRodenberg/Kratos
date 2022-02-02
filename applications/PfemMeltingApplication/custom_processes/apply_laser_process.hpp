//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Miguel Angel Celigueta (maceli@cimne.upc.edu)
//


#if !defined(KRATOS_APPLY_LASER_PROCESS )
#define  KRATOS_APPLY_LASER_PROCESS

#include "includes/table.h"
#include "includes/kratos_parameters.h"
#include "processes/process.h"

#include "utilities/parallel_utilities.h"
#include "utilities/function_parser_utility.h"
#include "utilities/interval_utility.h"
#include "utilities/python_function_callback_utility.h"


#include "includes/define.h"
#include "includes/model_part.h"
#include "includes/node.h"
#include "utilities/geometry_utilities.h"
#include "geometries/tetrahedra_3d_4.h"
#include "spatial_containers/spatial_containers.h"
#include "utilities/timer.h"
#include "processes/node_erase_process.h"
#include "utilities/binbased_fast_point_locator.h"
#include "includes/deprecated_variables.h"

#include <boost/timer.hpp>
#include "utilities/timer.h"

namespace Kratos
{

class ApplyLaserProcess : public Process
{

public:

    KRATOS_CLASS_POINTER_DEFINITION(ApplyLaserProcess);

    typedef Table<double, double> TableType;


    /// Constructor
    ApplyLaserProcess(
        ModelPart& rModelPart,
        Parameters rParameters
        ) : Process() ,
            mrModelPart(rModelPart)
    {
        KRATOS_TRY

        Parameters default_parameters( R"(
            {
                "model_part_name":"MODEL_PART_NAME",
                "filename"        : "./LaserSettings.json"
            }  )" );


 //       Parameters default_parameters( R"(
 //           {
 //               //"model_part_name":"MODEL_PART_NAME"
 //                "x_coordinate": 0.0,
//		 "y_coordinate": 0.0,
//		 "z_coordinate": 0.0,
//		 "radius": 0.0,
//		 "face_heat_flux": 0.0
  //          }  )" );

        mRadius = rParameters["laser_profile"]["radius"].GetDouble();
        mPower = rParameters["laser_profile"]["power"].GetDouble();
        mDirectionx = rParameters["direction"][0].GetDouble();
        mDirectiony = rParameters["direction"][1].GetDouble();
        mDirectionz = rParameters["direction"][2].GetDouble();


        // Now validate agains defaults -- this also ensures no type mismatch
        //rParameters.ValidateAndAssignDefaults(default_parameters);

        //mLaserProfile.clear();
        //mDirection.clear();
        //mPositionLaserTable.clear();


        KRATOS_CATCH("");
    }

    ///------------------------------------------------------------------------------------

    /// Destructor
    ~ApplyLaserProcess() override {}

///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    /// Execute method is used to execute the ApplyLaserProcess algorithms.
    void Execute() override
    {

    }

    /// this function is designed for being called at the beginning of the computations
    /// right after reading the model and the groups
    void ExecuteInitialize() override
    {
        KRATOS_TRY;
        KRATOS_CATCH("");
    }

    /// this function will be executed at every time step BEFORE performing the solve phase
	void ApplyLaser(double x, double y, double z)  {
        KRATOS_TRY;

        //defintions for spatial search
        typedef Node < 3 > PointType;
        typedef Node < 3 > ::Pointer PointTypePointer;
        typedef std::vector<PointType::Pointer> PointVector;

        //creating an auxiliary list for the new nodes
        PointVector list_of_nodes;

        for (ModelPart::NodesContainerType::iterator node_it = mrModelPart.NodesBegin(); node_it != mrModelPart.NodesEnd(); ++node_it) {
            PointTypePointer pnode = *(node_it.base());
            node_it->FastGetSolutionStepValue(FACE_HEAT_FLUX)=0.0;
            if(node_it->FastGetSolutionStepValue(IS_FREE_SURFACE)) {
                list_of_nodes.push_back(pnode);
            }
        }

        Node < 3 > work_point(0, 0.0, 0.0, 0.0);

        array_1d<double, 3> direction;
        /*direction[0] = 0.0;
        direction[1] = 1.0;
        direction[2] = 0.0;*/

        direction[0] = mDirectionx;
        direction[1] = mDirectiony;
        direction[2] = mDirectionz;

	//KRATOS_WATCH(direction)
        array_1d<double, 3> unitary_dir = direction * (1.0 / MathUtils<double>::Norm3(direction));

        //KRATOS_WATCH(unitary_dir)
        for (size_t k = 0; k < list_of_nodes.size(); k++) {
            const array_1d<double, 3>& coords = list_of_nodes[k]->Coordinates();
            array_1d<double, 3> distance_vector;
            distance_vector[0] = coords[0]-x;
            distance_vector[1] = coords[1]-y;
            distance_vector[2] = coords[2]-z;
            //KRATOS_WATCH(distance_vector)
            
            const double distance = MathUtils<double>::Norm3(distance_vector); // TODO: squared norm is better here

            //KRATOS_WATCH(distance)  
            const double distance_projected_to_dir = distance_vector[0]*unitary_dir[0] + distance_vector[1]*unitary_dir[1] + distance_vector[2]*unitary_dir[2];
            //KRATOS_WATCH(distance_projected_to_dir)   
            const double distance_to_laser_axis = std::sqrt(distance*distance - distance_projected_to_dir*distance_projected_to_dir);

            //KRATOS_WATCH(distance_to_laser_axis) 
            //KRATOS_THROW_ERROR(std::logic_error, "method not implemented", ""); 
            //KRATOS_WATCH(mRadius)
            if(distance_to_laser_axis < mRadius) {
                double& aux= list_of_nodes[k]->FastGetSolutionStepValue(FACE_HEAT_FLUX);
                aux =  mPower / (Globals::Pi * mRadius * mRadius);

		//KRATOS_WATCH(aux)
            }
        }
        //KRATOS_THROW_ERROR(std::logic_error, "method not implemented", ""); 
        KRATOS_CATCH("");
	}

    void ExecuteInitializeSolutionStep() override {
        KRATOS_TRY;


        KRATOS_CATCH("")
    }


    /**
     * @brief This function will be executed at every time step AFTER performing the solve phase
     */
    void ExecuteFinalizeSolutionStep() override {

    }

    /// Turn back information as a string.
    std::string Info() const override
    {
        return "ApplyLaserProcess";
    }

    /// Print information about this object.
    void PrintInfo(std::ostream& rOStream) const override
    {
        rOStream << "ApplyLaserProcess";
    }

    /// Print object's data.
    void PrintData(std::ostream& rOStream) const override
    {
    }

///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

protected:



///----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

private:

    ModelPart& mrModelPart;

    //mLaserProfile.clear();
    //mDirection.clear();
    std::vector<TableType::Pointer> mPositionLaserTable;
    array_1d<int, 3> mPositionLaserTableId;
    double mPower;
    double mRadius;
    double mDirectionx;
    double mDirectiony;
    double mDirectionz;

    /// Assignment operator.
    ApplyLaserProcess& operator=(ApplyLaserProcess const& rOther);

    /// Copy constructor.
    //ApplyLaserProcess(ApplyLaserProcess const& rOther);


}; // Class ApplyLaserProcess

/// input stream function
inline std::istream& operator >> (std::istream& rIStream,
                                  ApplyLaserProcess& rThis);

/// output stream function
inline std::ostream& operator << (std::ostream& rOStream,
                                  const ApplyLaserProcess& rThis)
{
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);

    return rOStream;
}

} // namespace Kratos.

#endif /* KRATOS_APPLY_LASER_PROCESS defined */
