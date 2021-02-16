# Importing the Kratos Library
import KratosMultiphysics

# Import applications
import KratosMultiphysics.StructuralMechanicsApplication as StructuralMechanicsApplication
import KratosMultiphysics.RomApplication as RomApplication

# Import base class file
from KratosMultiphysics.StructuralMechanicsApplication.structural_mechanics_analysis import StructuralMechanicsAnalysis

from KratosMultiphysics.RomApplication import python_solvers_wrapper_rom as solver_wrapper_rom

import json

class StructuralMechanicsModalDerivativeAnalysis(StructuralMechanicsAnalysis):

    def __init__(self,model,project_parameters):
        """The class for modal derivative analysis."""
        super().__init__(model,project_parameters)

    #### Internal functions ####
    def _CreateSolver(self):
        """Create the Solver (and create and import the ModelPart if it is not already in the model)."""
        ## Solver construction
        return solver_wrapper_rom.CreateSolver(self.model, self.project_parameters)

    def _GetSimulationName(self):
        return "::[Modal Derivative Simulation]:: "
    
    def OutputSolutionStep(self):
        """Prints / writes output files after the solution of a step."""
        # Creating output
        super().OutputSolutionStep()
        self.WriteRomParameters()
    
    def ModifyInitialGeometry(self):
        """Here is the place where the BASIS_ROM and the AUX_ID are imposed to each node."""
        super().ModifyInitialGeometry()
        computing_model_part = self._solver.GetComputingModelPart()

        rom_parameters_filename = self.project_parameters["solver_settings"]["rom_parameters_filename"].GetString()
        with open(rom_parameters_filename) as rom_parameters_file:
            rom_parameters = json.load(rom_parameters_file)

            self.number_of_initial_rom_dofs = rom_parameters["rom_settings"]["number_of_rom_dofs"]
            
            number_of_extended_rom_dofs = None
            derivative_type = self.project_parameters["solver_settings"]["derivative_type"].GetString()
            derivative_parameter = self.project_parameters["solver_settings"]["derivative_parameter"].GetString()
            if derivative_type == "dynamic" and derivative_parameter == "modal_coordinate":
                number_of_extended_rom_dofs = int(self.number_of_initial_rom_dofs * ( self.number_of_initial_rom_dofs + 1 ))
            elif derivative_type == "static" and derivative_parameter == "modal_coordinate":
                number_of_extended_rom_dofs = int(self.number_of_initial_rom_dofs + self.number_of_initial_rom_dofs * ( self.number_of_initial_rom_dofs + 1 ) / 2)
            elif derivative_type == "dynamic" and derivative_parameter != "modal_coordinate":
                number_of_sub_model_parts = len(self.project_parameters["solver_settings"]["sub_model_parts_list"].GetStringArray())
                number_of_extended_rom_dofs = int(self.number_of_initial_rom_dofs*(1+number_of_sub_model_parts))

            eigenvalues = rom_parameters["eigenvalues"]
            kratos_eigenvalues = KratosMultiphysics.Vector(self.number_of_initial_rom_dofs)
            for i in range(self.number_of_initial_rom_dofs):
                kratos_eigenvalues[i] = eigenvalues[i]
            computing_model_part.ProcessInfo[StructuralMechanicsApplication.EIGENVALUE_VECTOR] = kratos_eigenvalues
                        
            counter = 0
            nodal_dofs = len(rom_parameters["rom_settings"]["nodal_unknowns"])
            nodal_modes = rom_parameters["nodal_modes"]
            for node in computing_model_part.Nodes:
                aux = KratosMultiphysics.Matrix(nodal_dofs, number_of_extended_rom_dofs)
                aux.fill(0.0)
                for i in range(nodal_dofs):
                    Counter=str(node.Id)
                    for j in range(self.number_of_initial_rom_dofs):
                        aux[i,j] = nodal_modes[Counter][i][j]
                node.SetValue(RomApplication.ROM_BASIS, aux ) # ROM basis
                node.SetValue(RomApplication.AUX_ID, counter) # Aux ID
                counter+=1

        KratosMultiphysics.Logger.PrintInfo(self._GetSimulationName(), "Nodal modes imported from ",rom_parameters_filename)

    def WriteRomParameters(self):
        """Writes the extended basis into RomParameters.json file."""
        # Iniate nodal modes dictionary
        rom_parameters = {}
        rom_parameters["rom_settings"] = {}
        rom_parameters["rom_settings"]["nodal_unknowns"] = self.project_parameters["solver_settings"]["rom_settings"]["nodal_unknowns"].GetStringArray()
        rom_parameters["nodal_modes"] = {}
        
        eigenvalues = self._solver.GetComputingModelPart().ProcessInfo[RomApplication.EIGENVALUE_VECTOR]
        rom_parameters["eigenvalues"] = []
        for eigenvalue in eigenvalues:
            rom_parameters["eigenvalues"].append(eigenvalue)

        # Loop over nodes
        for node in self._solver.GetComputingModelPart().GetNodes():

            # Create Node Id for NodalMode
            rom_parameters["nodal_modes"][str(node.Id)] = []

            # Get nodal eigenvector matrix
            nodal_modes_mtx = node.GetValue(RomApplication.ROM_BASIS)
            num_nodal_dofs = nodal_modes_mtx.Size1()
            num_nodal_modes = nodal_modes_mtx.Size2()
            
            # Loop over nodal DOFs
            for iDOF in range(0, num_nodal_dofs):
                
                # Initiate DOF modes
                dof_modes = []

                # Loop over dof modes
                for iMode in range(0, num_nodal_modes):
                    dof_modes.append(nodal_modes_mtx[iDOF, iMode])

                # Set nodal modes
                rom_parameters["nodal_modes"][str(node.Id)].append(dof_modes)

        keys_nodal_modes = list(rom_parameters["nodal_modes"])
        num_modes = len(rom_parameters["nodal_modes"][keys_nodal_modes[0]][0])
        rom_parameters["rom_settings"]["number_of_rom_dofs"] = num_modes
        
        with open('RomParameters.json', 'w') as f:
            json.dump(rom_parameters,f, indent=2)

        KratosMultiphysics.Logger.PrintInfo(self._GetSimulationName(), "Nodal modes printed in JSON format")
