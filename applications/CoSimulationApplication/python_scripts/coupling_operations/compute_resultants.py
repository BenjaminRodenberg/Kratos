# Importing the Kratos Library
import KratosMultiphysics as KM
import KratosMultiphysics.FluidDynamicsApplication as FDA

# Importing the base class
from KratosMultiphysics.CoSimulationApplication.base_classes.co_simulation_coupling_operation import CoSimulationCouplingOperation

def Create(*args):
    return ComputeResultantsOperation(*args)

class ComputeResultantsOperation(CoSimulationCouplingOperation):
    """This operation computes the Normals (NORMAL) on a given ModelPart
    """
    def __init__(self, settings, solver_wrappers, process_info):
        super().__init__(settings, process_info)
        solver_name = self.settings["solver"].GetString()
        data_name = self.settings["data_name"].GetString()
        self.reference_point = self.settings["reference_point"].GetVector()
        self.interface_data = solver_wrappers[solver_name].GetInterfaceData(data_name)

    def Initialize(self):
        pass

    def Finalize(self):
        pass

    def InitializeSolutionStep(self):
        pass

    def FinalizeSolutionStep(self):
        pass

    def InitializeCouplingIteration(self):
        pass

    def FinalizeCouplingIteration(self):
        pass

    def Execute(self):
        
        force_and_moment_calculator = self.interface_data.GetModelPart()

        force = [0.0, 0.0, 0.0]
        moment = [0.0, 0.0, 0.0]

        for node in force_and_moment_calculator.GetCommunicator().LocalMesh().Nodes:

            # Sign is flipped to go from reaction to action -> force
            nodal_force = (-1) * node.GetSolutionStepValue(KM.REACTION, 0)

            # Summing up nodal contributions to get resultant for model_part
            force[0] += nodal_force[0]
            force[1] += nodal_force[1]
            force[2] += nodal_force[2]

            # Summing up nodal contributions to the resultant moment
            x = node.X - self.reference_point[0]
            y = node.Y - self.reference_point[1]
            z = node.Z - self.reference_point[2]
            moment[0] += y * nodal_force[2] - z * nodal_force[1]
            moment[1] += z * nodal_force[0] - x * nodal_force[2]
            moment[2] += x * nodal_force[1] - y * nodal_force[0]

        # here does it on rank 0, which is ok for output
        # force = force_and_moment_calculator.GetCommunicator().GetDataCommunicator().SumDoubles(force,0)
        # moment = force_and_moment_calculator.GetCommunicator().GetDataCommunicator().SumDoubles(moment,0)

        # this will have it on all ranks
        force = force_and_moment_calculator.GetCommunicator().GetDataCommunicator().SumAllDoubles(force)
        moment = force_and_moment_calculator.GetCommunicator().GetDataCommunicator().SumAllDoubles(moment)

        resultant = force + moment

        # here [] is the equivalent of .SetValue()
        force_and_moment_calculator[FDA.RESULTANT_FORCE] = force
        force_and_moment_calculator[FDA.RESULTANT_MOMENT] = moment
        force_and_moment_calculator[FDA.RESULTANT_FORCE_MOMENT] = resultant

    def PrintInfo(self):
        pass

    def Check(self):
        pass

    @classmethod
    def _GetDefaultParameters(cls):
        this_defaults = KM.Parameters("""{
            "solver"    : "UNSPECIFIED",
            "data_name" : "UNSPECIFIED",
            "reference_point" : [0.0, 0.0, 0.0]
        }""")
        this_defaults.AddMissingParameters(super()._GetDefaultParameters())
        return this_defaults



