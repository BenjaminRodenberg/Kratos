import numpy as np
import KratosMultiphysics as KM
from  KratosMultiphysics.NeuralNetworkApplication.preprocessing_process import PreprocessingProcess
from KratosMultiphysics.NeuralNetworkApplication.data_loading_utilities import ImportDictionaryFromText, UpdateDictionaryJson

def Factory(settings):
    if not isinstance(settings, KM.Parameters):
        raise Exception("expected input shall be a Parameters object, encapsulating a json string")
    return ScalingProcess(settings["parameters"])

class ScalingProcess(PreprocessingProcess):

    def __init__(self, settings):
        super().__init__(settings)
        """ The default constructor of the class

        Keyword arguments:
        self -- It signifies an instance of a class.
        model -- the container of the different model parts.
        settings -- Kratos parameters containing process settings.
        """

        if self.load_from_log:
            self.scale = "file"
        elif settings.Has("scale"):
            self.scale = settings["scale"].GetString()
        else:
            self.scale = "std"
        self.objective = settings["objective"].GetString()
       

    def Preprocess(self, data_in, data_out):
        try:
            input_log = ImportDictionaryFromText(self.input_log_name)
            output_log = ImportDictionaryFromText(self.output_log_name)
        except AttributeError:
                    print("No logging.")
                    input_log = {}
                    output_log = {}
        # Scaling from the mean
        if self.scale == "std":
            if self.objective == "input":
                std_in = np.std(data_in, axis = 0)
                data_in = data_in / std_in
                input_log.update({"scaling" : std_in.tolist()})
            if self.objective == "output":
                std_out = np.std(data_out, axis = 0)
                data_out = data_out / std_out
                output_log.update({"scaling" : std_out.tolist()})
            if self.objective == "all":
                std_in = np.std(data_in, axis = 0)
                data_in = data_in / std_in
                input_log.update({"scaling" : std_in.tolist()})
                std_out = np.std(data_out, axis = 0)
                data_out = data_out / std_out
                output_log.update({"scaling" : std_out.tolist()})

        # Scaling from file log
        if self.scale == "file":
            if self.objective == "input":
                data_in = data_in / input_log.get("scaling")
            if self.objective == "output":
                data_out = data_out / output_log.get("scaling")
            if self.objective == "all":
                data_in = data_in / input_log.get("scaling")
                data_out = data_out / output_log.get("scaling")
                
        # Updating the file log
        if not self.load_from_log:
            try:
                UpdateDictionaryJson(self.input_log_name, input_log)
                UpdateDictionaryJson(self.output_log_name, output_log)
            except AttributeError:
                pass
            
        return [data_in, data_out]
