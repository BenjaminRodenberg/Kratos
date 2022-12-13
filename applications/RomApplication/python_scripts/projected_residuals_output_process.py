# Import Python modules
import json
import numpy

# Importing the Kratos Library
import KratosMultiphysics
from KratosMultiphysics.RomApplication.randomized_singular_value_decomposition import RandomizedSingularValueDecomposition
import KratosMultiphysics.RomApplication as KratosROM

def Factory(settings, model):
    if not isinstance(settings, KratosMultiphysics.Parameters):
        raise Exception("Expected input shall be a Parameters object, encapsulating a json string.")
    return ProjectedResidualsOutputProcess(model, settings["Parameters"])

class ProjectedResidualsOutputProcess(KratosMultiphysics.OutputProcess):
    """A process to set the snapshots matrix and calculate the ROM basis from it."""

    def __init__(self, model, settings):
        KratosMultiphysics.OutputProcess.__init__(self)

        # Validate input settings against defaults
        settings.ValidateAndAssignDefaults(self.GetDefaultParameters())

        # Set strategies and intervals
        self.solving_strategy = settings["solving_strategy"].GetString()
        self.output_interval = settings["output_interval"].GetInt()
        self.rom_settings = settings["rom_settings"]
        self.output_step_counter = 0
        self.file_counter = 0
        self.time_step_residual_matrix_container = []

    @classmethod
    def GetDefaultParameters(self):
        default_settings = KratosMultiphysics.Parameters("""{
            "help": "A process to write to disk the snapshots matrix of the projected residuals.",
            "solving_strategy": "Galerkin",
            "output_interval": 1,
            "rom_settings": {
                "nodal_unknowns": [
                    "DISPLACEMENT_X",
                    "DISPLACEMENT_Y"
                ],
                "number_of_rom_dofs": 10,
                "petrov_galerkin_number_of_rom_dofs" : 10
            }
        }""")

        return default_settings

    def ExecuteInitialize(self):
        computing_model_part = self.solver.GetComputingModelPart()
        self.__rom_residuals_utility = KratosROM.RomResidualsUtility(
                computing_model_part,
                self.rom_settings,
                self.solver._GetScheme())

    def ExecuteFinalizeSolutionStep(self):
        self.output_step_counter += 1

        # Generate the matrix of residuals
        if self.echo_level > 0 : KratosMultiphysics.Logger.PrintInfo("HRomTrainingUtility","Generating matrix of residuals.")
        if (self.solving_strategy=="Galerkin"):
                res_mat = self.__rom_residuals_utility.GetProjectedResidualsOntoPhi()
        elif (self.solving_strategy=="Petrov-Galerkin"):
                res_mat = self.__rom_residuals_utility.GetProjectedResidualsOntoPsi()
        else:
            err_msg = "Projection strategy \'{}\' for hrom is not supported.".format(self.solving_strategy)
            raise Exception(err_msg)
        np_res_mat = numpy.array(res_mat, copy=False)
        self.time_step_residual_matrix_container.append(np_res_mat)


        if self.output_step_counter == self.output_interval:
            # Set up the residual snapshots matrix
            n_steps = len(self.time_step_residual_matrix_container)
            residuals_snapshot_matrix = self.time_step_residual_matrix_container[0]
            for i in range(1,n_steps):
                del self.time_step_residual_matrix_container[0] # Avoid having two matrices, numpy does not concatenate references.
                residuals_snapshot_matrix = numpy.c_[residuals_snapshot_matrix,self.time_step_residual_matrix_container[0]]
            self.file_counter += 1
            numpy.save("ROM_BASIS_"+str(int(self.file_counter))+".npy", residuals_snapshot_matrix)
            self.output_step_counter = 0
            self.time_step_residual_matrix_container = []



