import numpy as np 

fom_output = np.load("FOM/ExpectedOutput.npy")
rom_output = np.load("PG/ExpectedOutputPG.npy")

error = np.linalg.norm(rom_output-fom_output)/np.linalg.norm(fom_output)
print(error)