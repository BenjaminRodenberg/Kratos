# Importing the Kratos Library
from typing import List
import KratosMultiphysics

from KratosMultiphysics.python_solver import PythonSolver, PhysicalSolver

# Import applications
import KratosMultiphysics.RomApplication as KratosROM
from KratosMultiphysics.RomApplication.structural_mechanics_static_rom_solver import CreateSolver as CreateStructureSolver

def CreateSolver(solver_class, model, custom_settings, parallelism):
    class ROMSolver(solver_class):
        """ROM solver generic main class.
        This class serves as a generic class to make a standard Kratos solver ROM-compatible.
        It extends the default parameters to include the \'rom_settings\' and overrides the
        creation of the builder and solver to use the ROM one.
        """

        def _RegisterPhysicalSolvers(self):
            super()._RegisterPhysicalSolvers()
            self.AddPhysicalSolver("Structural", PhysicalSolver(
                self.rom_solver_settings,
                CreateStructureSolver,
                [self.model, self.rom_solver_settings]
            ))

        def __init__(self, model, custom_settings, parallelism):
            super().__init__(model, custom_settings)

            self.rom_solver_settings = custom_settings
            self.rom_solver_model = model

            KratosMultiphysics.Logger.PrintInfo("::[ROMSolver]:: ", "Construction finished")

        @classmethod
        def GetDefaultParameters(solver_class):
            default_settings = KratosMultiphysics.Parameters("""{
                "solving_strategy" : "Galerkin",
                "rom_settings": {
                    "nodal_unknowns": [],
                    "number_of_rom_dofs" : 10
                }
            }""")
            default_settings.AddMissingParameters(super().GetDefaultParameters())
            return default_settings

        def _CreateBuilderAndSolver(self):
            linear_solver = self._GetLinearSolver()
            rom_parameters, solving_strategy = self._ValidateAndReturnRomParameters()
            available_solving_strategies = {
                "Galerkin": KratosROM.ROMBuilderAndSolver, 
                "LSPG": KratosROM.LSPGROMBuilderAndSolver
            }
            if solving_strategy in available_solving_strategies:
                return available_solving_strategies[solving_strategy](linear_solver, rom_parameters)
            else:
                err_msg = "\'Solving_strategy\': '" +solving_strategy+"' is not available. Please select one of the following: "+ str(list(available_solving_strategies.keys()))
                raise ValueError(err_msg)

        def _ValidateAndReturnRomParameters(self):
            # Check that the number of ROM DOFs has been provided
            n_rom_dofs = self.settings["rom_settings"]["number_of_rom_dofs"].GetInt()
            if not n_rom_dofs > 0:
                err_msg = "\'number_of_rom_dofs\' in \'rom_settings\' is {}. Please set a larger than zero value.".format(n_rom_dofs)
                raise Exception(err_msg)

            # Check if the nodal unknowns have been provided by the user
            # If not, take the DOFs list from the base solver
            nodal_unknowns = self.settings["rom_settings"]["nodal_unknowns"].GetStringArray()
            if len(nodal_unknowns) == 0:
                solver_dofs_list = self.GetDofsList()
                if not len(solver_dofs_list) == 0:
                    self.settings["rom_settings"]["nodal_unknowns"].SetStringArray(solver_dofs_list)
                else:
                    err_msg = "\'nodal_unknowns\' in \'rom_settings\' is not provided and there is a not-valid implementation in base solver."
                    err_msg += " Please manually set \'nodal_unknowns\' in \'rom_settings\'."
                    raise Exception(err_msg)
            solving_strategy = self.settings["solving_strategy"].GetString()

            # Return the validated ROM parameters
            return self.settings["rom_settings"], solving_strategy

    return ROMSolver(model, custom_settings, "OpenMP")