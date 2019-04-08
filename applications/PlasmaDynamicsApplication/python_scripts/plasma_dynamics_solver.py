from __future__ import print_function, absolute_import, division  # makes KratosMultiphysics backward compatible with python 2.6 and 2.7

# Importing the Kratos Library
from KratosMultiphysics import *
from python_solver import PythonSolver

# Import applications
import KratosMultiphysics.PlasmaDynamicsApplication as PlasmaDynamicsApplication
import plasma_dynamics_procedures 
import CFD_DEM_for_plasma_dynamics_coupling
#import parameters_tools_for_plasma_dynamics as PT
#import derivative_recovery.derivative_recovery_strategy as derivative_recoverer
import math

def Say(*args):
    Logger.PrintInfo("PlasmaDynamics", *args)
    Logger.Flush()

class PlasmaDynamicsSolver(PythonSolver):
    def _ValidateSettings(self, project_parameters):
        #calculate_nodal_area_process
        #TODO: to complete
        default_processes_settings = Parameters("""{
                "python_module" : "gid_output_process",
                "kratos_module" : "KratosMultiphysics",
                "process_name"  : "GiDOutputProcess",
                "Parameters"    : {
                    "model_part_name"        : "Particles_in_a_cylinder",
                    "output_name"            : "Electrically_accelerated_ions",
                    "postprocess_parameters" : {
                        "result_file_configuration" : {
                            "gidpost_flags"       : {
                                "GiDPostMode"           : "GiD_PostBinary",
                                "WriteDeformedMeshFlag" : "WriteUndeformed",
                                "WriteConditionsFlag"   : "WriteElementsOnly",
                                "MultiFileFlag"         : "MultipleFiles"
                            },
                            "file_label"          : "step",
                            "output_control_type" : "step",
                            "output_frequency"    : 1,
                            "body_output"         : true,
                            "node_output"         : true,
                            "skin_output"         : false,
                            "plane_output"        : [],
                            "nodal_results"       : ["DISPLACEMENT","REACTION","VELOCITY","ACCELERATION"],
                            "gauss_point_results" : []
                        },
                    "point_data_configuration"  : []
                    }
                }
            }

        """)

        """         if not project_parameters["processes"].Has('non_optional_solver_processes'):
            project_parameters["processes"].AddEmptyArray("non_optional_solver_processes")

        else: # reconstruct non_optional_solver_processes list making sure calculate_nodal_area_process is not added twice
            non_optional_processes_list = list(project_parameters["processes"]["non_optional_solver_processes"])
            project_parameters["processes"].Remove("non_optional_solver_processes")
            project_parameters["processes"].AddEmptyArray("non_optional_solver_processes")

            for process in non_optional_processes_list:
                if process["python_module"].GetString() != 'calculate_nodal_area_process':
                    project_parameters["processes"]["non_optional_solver_processes"].Append(process)

        non_optional_solver_processes = project_parameters["processes"]["non_optional_solver_processes"]
        non_optional_solver_processes.Append(default_processes_settings)
        nodal_area_process_parameters = non_optional_solver_processes[non_optional_solver_processes.size() -1]["Parameters"] """
        #nodal_area_process_parameters["model_part_name"].SetString(self.fluid_solver.main_model_part.Name)
        #nodal_area_process_parameters["domain_size"].SetInt(self.fluid_domain_dimension)


        #nodal_area_process_parameters["fixed_mesh"].SetBool(True)

        return project_parameters

    def __init__(self, model, project_parameters, field_utility, fluid_solver, dem_solver, variables_manager):
        # Validate settings
        self.field_utility = field_utility
        self.vars_man = variables_manager
        #self.fluid_domain_dimension = project_parameters["fluid_parameters"]["solver_settings"]["domain_size"].GetInt()
        self.fluid_solver = fluid_solver
        self.dem_solver = dem_solver
        self.project_parameters = self._ValidateSettings(project_parameters)
        self.next_time_to_solve_fluid = project_parameters['problem_data']['start_time'].GetDouble()
        self.coupling_level_type = project_parameters["coupling_level_type"].GetInt()
        #self.coupling_scheme_type = project_parameters["coupling_scheme_type"].GetString()
        self.interaction_start_time = project_parameters["interaction_start_time"].GetDouble()
        #self.project_at_every_substep_option = project_parameters["project_at_every_substep_option"].GetBool()
        self.integration_scheme = project_parameters["TranslationalIntegrationScheme"].GetString()
        #self.fluid_dt = fluid_solver.settings["time_stepping"]["time_step"].GetDouble()
        self.do_solve_dem = project_parameters["do_solve_dem"].GetBool()
        self.solve_system = not self.project_parameters["fluid_already_calculated"].GetBool()

        self.fluid_step = 0
        self.calculating_fluid_in_current_step = True
        self.first_DEM_iteration = True
        #self.ConstructStationarityTool()
        self.ConstructDerivativeRecoverer()
        self.ConstructHistoryForceUtility()
        # Call the base Python solver constructor
        super(PlasmaDynamicsSolver, self).__init__(model, project_parameters)

    def ConstructStationarityTool(self):
        """         self.stationarity = False
        self.stationarity_counter = self.GetStationarityCounter()
        self.stationarity_tool = plasma_dynamics_procedures.StationarityAssessmentTool(
            self.project_parameters["max_pressure_variation_rate_tol"].GetDouble(),
            plasma_dynamics_procedures.FunctionsCalculator()
            ) """
        pass

    def _ConstructProjectionModule(self):
        """         # creating a projection module for the fluid-DEM coupling
        self.h_min = 0.01 #TODO: this must be set from interface and the method must be checked for 2D
        n_balls = 1
        fluid_volume = 10
        # the variable n_particles_in_depth is only relevant in 2D problems
        self.project_parameters.AddEmptyValue("n_particles_in_depth").SetInt(int(math.sqrt(n_balls / fluid_volume)))

        projection_module = CFD_DEM_coupling.ProjectionModule(
        self.fluid_solver.main_model_part,
        self.dem_solver.spheres_model_part,
        self.dem_solver.all_model_parts.Get("RigidFacePart"),
        self.project_parameters,
        self.vars_man.coupling_dem_vars,
        self.vars_man.coupling_fluid_vars,
        self.vars_man.time_filtered_vars,
        flow_field=self.field_utility,
        domain_size=self.fluid_domain_dimension
        )

        projection_module.UpdateDatabase(self.h_min)

        return projection_module """
        pass

    def ConstructDerivativeRecoverer(self):
        self.derivative_recovery_counter = self.GetRecoveryCounter()
        self.using_hinsberg_method = bool(self.project_parameters["basset_force_type"].GetInt() >= 3 or
                                          self.project_parameters["basset_force_type"].GetInt() == 1)
        """         self.recovery = derivative_recoverer.DerivativeRecoveryStrategy(
            self.project_parameters,
            self.fluid_solver.main_model_part,
            plasma_dynamics_procedures.FunctionsCalculator(self.fluid_domain_dimension)) """

    def ConstructHistoryForceUtility(self):
        """         self.quadrature_counter = self.GetHistoryForceQuadratureCounter()
        self.basset_force_tool = PlasmaDynamicsApplication.BassetForceTools() """
        pass

    def GetStationarityCounter(self):
        return plasma_dynamics_procedures.Counter(
            steps_in_cycle=self.project_parameters["time_steps_per_stationarity_step"].GetInt(),
            beginning_step=1,
            is_active=self.project_parameters["stationary_problem_option"].GetBool())

    def GetRecoveryCounter(self):
        there_is_something_to_recover = (
            self.project_parameters["coupling_level_type"].GetInt())
        return plasma_dynamics_procedures.Counter(1, 1, there_is_something_to_recover)


    def GetHistoryForceQuadratureCounter(self):
        """         for prop in self.project_parameters["properties"].values():
            if prop["plasma_dynamics_law_parameters"].Has("history_force_parameters"):
                history_force_parameters =  prop["plasma_dynamics_law_parameters"]["history_force_parameters"]
                if history_force_parameters.Has("time_steps_per_quadrature_step"):
                    time_steps_per_quadrature_step = history_force_parameters["time_steps_per_quadrature_step"].GetInt()

                    return plasma_dynamics_procedures.Counter(steps_in_cycle=time_steps_per_quadrature_step, beginning_step=1) """

        return plasma_dynamics_procedures.Counter(is_dead=True)


    def AdvanceInTime(self, time):
        self.time = self.dem_solver.AdvanceInTime(time)
        """         self.calculating_fluid_in_current_step = bool(time >= self.next_time_to_solve_fluid)
        if self.calculating_fluid_in_current_step:
            self.next_time_to_solve_fluid = self.fluid_solver.AdvanceInTime(time)
            self.fluid_step += 1 """

        return self.time

    def UpdateALEMeshMovement(self, time): # TODO: move to derived solver
        pass

    def CalculateMinElementSize(self):
        return self.h_min

    def AssessStationarity(self):
        """         Say("Assessing Stationarity...\n")
        self.stationarity = self.stationarity_tool.Assess(self.fluid_solver.main_model_part)
        self.stationarity_counter.Deactivate(self.stationarity) """
        pass

    # Compute nodal quantities to be printed that are not generated as part of the
    # solution algorithm. For instance, the pressure gradient, which is not used for
    # the coupling but can be of interest.
    def ComputePostProcessResults(self):
        if self.project_parameters["coupling_level_type"].GetInt():
            self._GetProjectionModule().ComputePostProcessResults(self.dem_solver.spheres_model_part.ProcessInfo)

    def CannotIgnoreFluidNow(self):
        #return self.solve_system and self.calculating_fluid_in_current_step
        return False

    def Predict(self):
        """         if self.CannotIgnoreFluidNow():
            self.fluid_solver.Predict() """
        pass

    def ApplyForwardCoupling(self, alpha='None'):
        self._GetProjectionModule().ApplyForwardCoupling(alpha)

    def ApplyForwardCouplingOfVelocityToSlipVelocityOnly(self, time=None):
        #self._GetProjectionModule().ApplyForwardCouplingOfVelocityToSlipVelocityOnly()
        pass

    def _GetProjectionModule(self):
        if not hasattr(self, 'projection_module'):
            self.projection_module = self._ConstructProjectionModule()
        return self.projection_module

    def SolveSolutionStep(self):
        # update possible movements of the fluid mesh
        self.UpdateALEMeshMovement(self.time)

        # Solving the fluid part
        #Say('Solving Fluid... (', self.fluid_solver.main_model_part.NumberOfElements(0), 'elements )\n')
        self.solve_system = not self.project_parameters["fluid_already_calculated"].GetBool() 

        if self.CannotIgnoreFluidNow():
            self.SolveFluidSolutionStep()
        else:
            Say("Skipping solving system for the fluid phase...\n")

        # Check for stationarity: this is useful for steady-state problems, so that
        # the calculation stops after reaching the solution.
        """         if self.stationarity_counter.Tick():
            self.AssessStationarity() """

        """         self.derivative_recovery_counter.Activate(self.time > self.interaction_start_time and self.calculating_fluid_in_current_step)

        if self.derivative_recovery_counter.Tick():
            self.recovery.Recover() """

        # Solving the disperse-phase component
        Say('Solving DEM... (', self.dem_solver.spheres_model_part.NumberOfElements(0), 'elements )')
        self.SolveDEM()

    def SolveFluidSolutionStep(self):
        #self.fluid_solver.SolveSolutionStep()
        pass

    def SolveDEMSolutionStep(self):
        self.dem_solver.SolveSolutionStep()

    def SolveDEM(self):
        #self.PerformEmbeddedOperations() TO-DO: it's crashing

        """         it_is_time_to_forward_couple = (
            self.time >= self.interaction_start_time and
            self.coupling_level_type and
            (self.project_at_every_substep_option or self.calculating_fluid_in_current_step)
        ) """

        """         if it_is_time_to_forward_couple or self.first_DEM_iteration:

            if self.coupling_scheme_type == "UpdatedDEM":
                self.ApplyForwardCoupling()

            else:
                alpha = 1.0 - (self.next_time_to_solve_fluid - self.time) / self.fluid_dt
                self.ApplyForwardCoupling(alpha) """

        """         if self.quadrature_counter.Tick():
            self.AppendValuesForTheHistoryForce() """

        if self.integration_scheme in {'Symplectic_Euler', 'Forward_Euler', 'Taylor_Scheme', 'Velocity_Verlet'}:
            # Advance in space only
            if self.do_solve_dem:
                self.SolveDEMSolutionStep()
            self.ApplyForwardCouplingOfVelocityToSlipVelocityOnly(self.time)

        # Performing the time integration of the DEM part

        self.first_DEM_iteration = False

    def AppendValuesForTheHistoryForce(self):
        if self.using_hinsberg_method:
            self.basset_force_tool.AppendIntegrandsWindow(self.dem_solver.spheres_model_part)
        elif self.project_parameters["basset_force_type"].GetInt() == 2:
            self.basset_force_tool.AppendIntegrands(self.dem_solver.spheres_model_part)

    def ImportModelPart(self): # TODO: implement this
        pass

    def GetComputingModelPart(self):
        return self.dem_solver.spheres_model_part