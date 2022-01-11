# Import Python modules
import json
import numpy as np

# Importing the Kratos Library
import KratosMultiphysics

# Import applications
import KratosMultiphysics.RomApplication as KratosROM
from KratosMultiphysics.RomApplication.empirical_cubature_method import EmpiricalCubatureMethod
from KratosMultiphysics.RomApplication.randomized_singular_value_decomposition import RandomizedSingularValueDecomposition

class HRomTrainingUtility(object):
    """Auxiliary utility for the HROM training.
    This class encapsulates all the functions required for the HROM training.
    These are the ROM residuals collection, the calculation and storage of
    the HROM weights and the creation and export of the HROM model parts.
    """

    def __init__(self, solver, custom_settings):
        # Validate and assign the HROM training settings
        settings = custom_settings["hrom_settings"]
        settings.ValidateAndAssignDefaults(self.__GetHRomTrainingDefaultSettings())

        # Create the HROM element selector
        element_selection_type = settings["element_selection_type"].GetString()
        if element_selection_type == "empirical_cubature":
            self.hyper_reduction_element_selector = EmpiricalCubatureMethod()
            self.element_selection_svd_truncation_tolerance = settings["element_selection_svd_truncation_tolerance"].GetDouble()
        else:
            err_msg = "\'{}\' HROM \'element_selection_type\' is not supported.".format(element_selection_type)
            raise Exception(err_msg)

        # Auxiliary member variables
        self.solver = solver
        self.time_step_residual_matrix_container = []
        self.echo_level = settings["echo_level"].GetInt()
        self.rom_settings = custom_settings["rom_settings"]

    def AppendCurrentStepResiduals(self):
        # Get the computing model part from the solver implementing the problem physics
        computing_model_part = self.solver.GetComputingModelPart()

        # If not created yet, create the ROM residuals utility
        # Note that this ensures that the residuals utility is created in the first residuals append call
        # If not, it might happen that the solver scheme is created by the ROM residuals call rather than by the solver one
        if not hasattr(self, '__rom_residuals_utility'):
            self.__rom_residuals_utility = KratosROM.RomResidualsUtility(
                computing_model_part,
                self.rom_settings,
                self.solver._GetScheme())

            if self.echo_level > 0 : KratosMultiphysics.Logger.PrintInfo("HRomTrainingUtility","RomResidualsUtility created.")

        # Generate the matrix of residuals
        if self.echo_level > 0 : KratosMultiphysics.Logger.PrintInfo("HRomTrainingUtility","Generating matrix of residuals.")
        res_mat = self.__rom_residuals_utility.GetResiduals()
        np_res_mat = np.array(res_mat, copy=False)
        self.time_step_residual_matrix_container.append(np_res_mat)

    def CalculateAndSaveHRomWeights(self):
        # Calculate the residuals basis and compute the HROM weights from it
        residual_basis = self.__CalculateResidualBasis()
        self.hyper_reduction_element_selector.SetUp(residual_basis)
        self.hyper_reduction_element_selector.Run()

        # Save the HROM weights in the RomParameters.json
        # Note that in here we are assuming this naming convention for the ROM json file
        self.__AppendHRomWeightsToRomParameters()

    def CreateHRomModelParts(self):
        # Get solver data
        model_part_name = self.solver.settings["model_part_name"].GetString()
        model_part_output_name = self.solver.settings["model_import_settings"]["input_filename"].GetString()
        computing_model_part = self.solver.GetComputingModelPart()

        # Create a new model with the HROM main model part
        # This is intentionally done in order to completely emulate the origin model part
        aux_model = KratosMultiphysics.Model()
        hrom_main_model_part = aux_model.CreateModelPart(model_part_name)

        # Get the weights and fill the HROM computing model part
        with open('RomParameters.json','r') as f:
            rom_parameters = KratosMultiphysics.Parameters(f.read())
        KratosROM.RomAuxiliaryUtilities.SetHRomComputingModelPart(rom_parameters["elements_and_weights"], computing_model_part, hrom_main_model_part)
        if self.echo_level > 0:
            KratosMultiphysics.Logger.PrintInfo("HRomTrainingUtility","HROM computing model part \'{}\' created.".format(hrom_main_model_part.FullName()))

        # Create the HROM visualization model part
        set_visualization_modelpart = False #TODO: Make this user-definable
        if set_visualization_modelpart:
            hrom_visualization_model_part = hrom_main_model_part.CreateSubModelPart("VISUALIZATION_HROM") #TODO: Use the Kratos naming convention
            KratosROM.RomAuxiliaryUtilities.SetHRomVisualizationModelPart(computing_model_part, hrom_visualization_model_part)
            if self.echo_level > 0:
                KratosMultiphysics.Logger.PrintInfo("HRomTrainingUtility","HROM visualization model part \'{}\' created.".format(hrom_visualization_model_part.FullName()))

        # Output the HROM model part in mdpa format
        hrom_output_name = "{}HROM".format(model_part_output_name)
        model_part_io = KratosMultiphysics.ModelPartIO(hrom_output_name, KratosMultiphysics.IO.WRITE | KratosMultiphysics.IO.MESH_ONLY)
        model_part_io.WriteModelPart(hrom_main_model_part)
        KratosMultiphysics.kratos_utilities.DeleteFileIfExisting("{}.time".format(hrom_output_name))
        if self.echo_level > 0:
            KratosMultiphysics.Logger.PrintInfo("HRomTrainingUtility","HROM mesh written in \'{}.mdpa\'".format(hrom_output_name))

    @classmethod
    def __GetHRomTrainingDefaultSettings(cls):
        default_settings = KratosMultiphysics.Parameters("""{
            "element_selection_type": "empirical_cubature",
            "element_selection_svd_truncation_tolerance": 1.0e-6,
            "echo_level" : 0
        }""")
        return default_settings

    def __CalculateResidualBasis(self):
        # Set up the residual snapshots matrix
        n_steps = len(self.time_step_residual_matrix_container)
        residuals_snapshot_matrix = self.time_step_residual_matrix_container[0]
        for i in range(1,n_steps):
            residuals_snapshot_matrix = np.c_[residuals_snapshot_matrix,self.time_step_residual_matrix_container[i]]

        # Calculate the randomized and truncated SVD of the residual snapshots
        u,_,_,_ = RandomizedSingularValueDecomposition(COMPUTE_V=False).Calculate(
            residuals_snapshot_matrix,
            self.element_selection_svd_truncation_tolerance)

        return u

    def __AppendHRomWeightsToRomParameters(self):
        n_elements = self.solver.GetComputingModelPart().NumberOfElements()
        w = np.squeeze(self.hyper_reduction_element_selector.w)
        z = self.hyper_reduction_element_selector.z

        # Create dictionary with HROM weights
        hrom_weights = {}
        hrom_weights["Elements"] = {}
        hrom_weights["Conditions"] = {}

        if type(z)==np.int64 or type(z)==np.int32:
            # Only one element found !
            if z <= n_elements-1:
                hrom_weights["Elements"][int(z)] = float(w)
            else:
                hrom_weights["Conditions"][int(z)-n_elements] = float(w)
        else:
            # Many elements found
            for j in range (0,len(z)):
                if z[j] <=  n_elements -1:
                    hrom_weights["Elements"][int(z[j])] = float(w[j])
                else:
                    hrom_weights["Conditions"][int(z[j])-n_elements] = float(w[j])

        # Append weights to RomParameters.json
        # We first parse the current RomParameters.json to then append and edit the data
        with open('RomParameters.json','r') as f:
            updated_rom_parameters = json.load(f)
            #FIXME: I don't really like to automatically change things without the user realizing...
            #FIXME: However, this leaves the settings ready for the HROM postprocess... something that is cool
            #FIXME: Decide about this
            # updated_rom_parameters["train_hrom"] = False
            # updated_rom_parameters["run_hrom"] = True
            updated_rom_parameters["elements_and_weights"] = hrom_weights #TODO: Rename elements_and_weights to hrom_weights

        with open('RomParameters.json','w') as f:
            json.dump(updated_rom_parameters, f, indent = 4)

        if self.echo_level > 0 : KratosMultiphysics.Logger.PrintInfo("HRomTrainingUtility","\'RomParameters.json\' file updated with HROM weights.")
