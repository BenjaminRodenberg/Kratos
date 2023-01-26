import numpy as np 
def error(obtained, expected):
    return np.linalg.norm(obtained-expected)/np.linalg.norm(expected)

fom_output = np.load("FOM/ExpectedOutput.npy")
rom_output = np.load("ROM/ExpectedOutput.npy")
lspg_rom_output = np.load("LSPGROM/ExpectedOutput.npy")
pg_rom_output = np.load("PGROM/ExpectedOutput.npy")


print("Galerkin", error(rom_output, fom_output))
print("LSPG", error(lspg_rom_output, fom_output))
print("Petrov-Galerkin", error(pg_rom_output, fom_output))