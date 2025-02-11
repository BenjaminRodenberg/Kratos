//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 license: OptimizationApplication/license.txt
//
//  Main authors:    Reza Najian Asl, https://github.com/RezaNajian
//

#if !defined(KRATOS_ADD_UTILITIES_TO_PYTHON_H_INCLUDED )
#define  KRATOS_ADD_UTILITIES_TO_PYTHON_H_INCLUDED

#pragma once

// ------------------------------------------------------------------------------
// System includes
// ------------------------------------------------------------------------------
#include <pybind11/pybind11.h>

// ------------------------------------------------------------------------------
// External includes
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
// Project includes
// ------------------------------------------------------------------------------
#include "includes/define.h"

// ==============================================================================

namespace Kratos {
namespace Python {

void  AddCustomUtilitiesToPython(pybind11::module& m);

}  // namespace Python.
}  // namespace Kratos.

#endif // KRATOS_ADD_UTILITIES_TO_PYTHON_H_INCLUDED  defined
