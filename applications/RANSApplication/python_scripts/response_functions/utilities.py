import KratosMultiphysics as Kratos

from KratosMultiphysics.RANSApplication.rans_analysis import RANSAnalysis
from KratosMultiphysics.RANSApplication.adjoint_rans_analysis import AdjointRANSAnalysis

from pathlib import Path
from shutil import copy
import math

def SolvePrimalProblem(kratos_primal_parameters_file_name, log_file_name = "primal_evaluation.log"):
    primal_filled_settings_file = Path(kratos_primal_parameters_file_name[:kratos_primal_parameters_file_name.rfind(".")] + "_final.json")

    if (not primal_filled_settings_file.is_file()):
        with open(kratos_primal_parameters_file_name, "r") as file_input:
            primal_parameters = Kratos.Parameters(file_input.read())

        # set the loggers
        file_logger = Kratos.FileLoggerOutput(log_file_name)
        default_severity = Kratos.Logger.GetDefaultOutput().GetSeverity()
        Kratos.Logger.GetDefaultOutput().SetSeverity(Kratos.Logger.Severity.WARNING)
        Kratos.Logger.AddOutput(file_logger)

        # run the primal analysis
        primal_model = Kratos.Model()
        primal_simulation = RANSAnalysis(primal_model, primal_parameters)
        primal_simulation.Run()

        with open(str(primal_filled_settings_file), "w") as file_output:
            file_output.write(primal_parameters.PrettyPrintJsonString())

        del primal_simulation
        del primal_model

        # flush the primal output
        Kratos.Logger.Flush()
        Kratos.Logger.RemoveOutput(file_logger)
        Kratos.Logger.GetDefaultOutput().SetSeverity(default_severity)
        Kratos.Logger.PrintInfo("SolvePrimalProblem", "Solved primal evaluation at {}.".format(kratos_primal_parameters_file_name))
    else:
        # following is done to initialize default settings
        with open(str(primal_filled_settings_file), "r") as file_input:
            primal_parameters = Kratos.Parameters(file_input.read())
        Kratos.Logger.PrintInfo("SolvePrimalProblem", "Found existing completed primal evaluation at {}.".format(kratos_primal_parameters_file_name))

    return primal_parameters

def SolveAdjointProblem(model, adjoint_parameters_file_name, log_file_name, skip_if_already_run = False):
    if (skip_if_already_run):
        if Path(log_file_name).is_file():
            with open(log_file_name, "r") as log_file_input:
                last_line = log_file_input.readlines()[-1][:-1].strip()

            if (last_line == "AdjointRANSAnalysis: Analysis -END-"):
                Kratos.Logger.PrintInfo("SolveAdjointProblem", "Found existing completed adjoint evaluation at {}.".format(adjoint_parameters_file_name))
                return

    with open(adjoint_parameters_file_name, "r") as file_input:
        adjoint_parameters = Kratos.Parameters(file_input.read())

    # set the lift loggers
    file_logger = Kratos.FileLoggerOutput(log_file_name)
    default_severity = Kratos.Logger.GetDefaultOutput().GetSeverity()
    Kratos.Logger.GetDefaultOutput().SetSeverity(Kratos.Logger.Severity.WARNING)
    Kratos.Logger.AddOutput(file_logger)

    adjoint_simulation = AdjointRANSAnalysis(model, adjoint_parameters)
    adjoint_simulation.Run()

    Kratos.Logger.Flush()

    Kratos.Logger.GetDefaultOutput().SetSeverity(default_severity)
    Kratos.Logger.RemoveOutput(file_logger)

    return adjoint_simulation

def RecursiveCopy(src, dest):
    src_path = Path(src)
    dest_path = Path(dest)
    for item in src_path.iterdir():
        if (item.is_file()):
            copy(str(item), dest)
        elif (item.is_dir()):
            new_path = dest_path / item.relative_to(src)
            new_path.mkdir(exist_ok=True)
            RecursiveCopy(str(item), str(new_path))

def GetDragValues(kratos_parameters, model_part_name):
    output_process = _GetDragResponseFunctionOutputProcess(kratos_parameters, model_part_name)
    if (output_process is not None):
        output_file_name = output_process["Parameters"]["output_file_settings"]["file_name"].GetString()
        time_steps, reactions = ReadDrag(output_file_name)
        return time_steps, reactions
    else:
        raise RuntimeError("No \"compute_body_fitted_drag_process\" found in auxiliar_process_list.")

def CalculateTimeAveragedDrag(kratos_parameters, model_part_name, direction):
    time_steps, reactions = GetDragValues(kratos_parameters, model_part_name)
    total_drag = 0.0
    for reaction in reversed(reactions):
        total_drag += reaction[0] * direction[0] + reaction[1] * direction[1] + reaction[2] * direction[2]
        if (kratos_parameters["solver_settings"]["time_scheme_settings"]["scheme_type"].GetString() == "steady"):
            break
    if len(time_steps) > 1:
        delta_time = time_steps[1] - time_steps[0]
        total_drag *= delta_time
    return total_drag

def CalculateDragFrequencyDistribution(time_steps, reactions, drag_direction, windowing_length):
    delta_time = time_steps[1] - time_steps[0]

    number_of_steps = len(reactions)
    number_of_frequencies = number_of_steps // 2
    windowing_steps = int(windowing_length / delta_time)

    drag_values = [0.0] * number_of_steps
    for index, reaction in enumerate(reactions):
        drag_values[index] = reaction[0] * drag_direction[0] + reaction[1] * drag_direction[1] + reaction[2] * drag_direction[2]

    frequency_real_components = [0.0] * number_of_frequencies
    frequency_imag_components = [0.0] * number_of_frequencies
    frequency_amplitudes = [0.0] * number_of_frequencies
    frequency_list = [0.0] * number_of_frequencies

    for index, drag in enumerate(drag_values[number_of_steps - windowing_steps:]):
        time_step_index = index + number_of_steps - windowing_steps
        window_value = 0.5 * (1.0 - math.cos(2.0 * math.pi * index / windowing_steps))
        for k in range(number_of_frequencies):
            time_step_value = 2.0 * math.pi * time_step_index * k / number_of_steps
            frequency_real_components[k] += window_value * drag * math.cos(time_step_value)
            frequency_imag_components[k] += window_value * drag * math.sin(time_step_value)

    frequency_resolution = 1.0 / (delta_time * number_of_steps)
    for k in range(number_of_frequencies):
        frequency_list[k] = k * frequency_resolution
        frequency_amplitudes[k] = math.sqrt(frequency_real_components[k] ** 2 + frequency_imag_components[k] ** 2) * 2 / windowing_steps

    return frequency_list, frequency_real_components, frequency_imag_components, frequency_amplitudes

def ReadDrag(file_name):
    with open(file_name, "r") as file_input:
        lines = file_input.readlines()
    time_steps = []
    reaction = []
    for line in lines:
        line = line.strip()
        if len(line) == 0 or line[0] == '#':
            continue
        time_step_data = [float(v) for v in line.split()]
        time, fx, fy, fz = time_step_data
        time_steps.append(time)
        reaction.append([fx, fy, fz])
    return time_steps, reaction

def _GetDragResponseFunctionOutputProcess(kratos_parameters, model_part_name):
    auxiliar_process_list = kratos_parameters["processes"]["auxiliar_process_list"]
    for process_settings in auxiliar_process_list:
        if (
            process_settings.Has("python_module") and process_settings["python_module"].GetString() == "compute_body_fitted_drag_process" and
            process_settings.Has("kratos_module") and process_settings["kratos_module"].GetString() == "KratosMultiphysics.FluidDynamicsApplication" and
            process_settings["Parameters"].Has("model_part_name") and process_settings["Parameters"]["model_part_name"].GetString() == model_part_name
            ):
            return process_settings

    return None


