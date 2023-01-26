import numpy as np 
def error(obtained, expected):
    return np.linalg.norm(obtained-expected)/np.linalg.norm(expected)

fom_output = np.load("FOM/ExpectedOutput.npy")
rom_output = np.load("ROM/ExpectedOutputROM.npy")
lspg_rom_output = np.load("LSPGROM/ExpectedOutputLSPGROM.npy")
pg_rom_output = np.load("PGROM/ExpectedOutputPGROM.npy")


print("Galerkin", error(rom_output, fom_output))
print("LSPG", error(lspg_rom_output, fom_output))
print("Petrov-Galerkin", error(pg_rom_output, fom_output))