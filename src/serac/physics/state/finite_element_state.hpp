// Copyright (c) 2019-2022, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/**
 * @file finite_element_state.hpp
 *
 * @brief This file contains the declaration of structure that manages the MFEM objects
 * that make up the state for a given field
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "mfem.hpp"

#include "serac/infrastructure/variant.hpp"
#include "serac/physics/state/finite_element_vector.hpp"

namespace serac {

/**
 * @brief convenience function for querying the type stored in a GeneralCoefficient
 */
inline bool is_scalar_valued(const GeneralCoefficient& coef)
{
  return holds_alternative<std::shared_ptr<mfem::Coefficient>>(coef);
}

/**
 * @brief convenience function for querying the type stored in a GeneralCoefficient
 */
inline bool is_vector_valued(const GeneralCoefficient& coef)
{
  return holds_alternative<std::shared_ptr<mfem::VectorCoefficient>>(coef);
}

/**
 * @brief Class for encapsulating the critical MFEM components of a primal finite element field
 *
 * Namely: Mesh, FiniteElementCollection, FiniteElementState, and the true vector of the solution
 */
class FiniteElementState : public FiniteElementVector {
public:
  /**
   * @brief Use the finite element vector constructors
   */
  using FiniteElementVector::FiniteElementVector;
  using FiniteElementVector::operator=;

  /**
   * @brief Set a finite element state to a constant value
   *
   * @param value The constant to set the finite element state to
   * @return The modified finite element state
   * @note This sets the true degrees of freedom and then broadcasts to the shared grid function entries. This means
   * that if a different value is given on different processors, a shared DOF will be set to the owning processor value.
   */
  FiniteElementState& operator=(const double value)
  {
    FiniteElementVector::operator=(value);
    return *this;
  }

protected:
  /**
   * @brief Set the internal grid function using the true DOF values
   *
   * This distributes true vector dofs to the finite element (local) dofs  by multiplying the true dofs
   * by the prolongation operator.
   *
   * @see <a href="https://mfem.org/pri-dual-vec/">MFEM documentation</a> for details
   *
   */
  void distributeSharedDofs(mfem::ParGridFunction& grid_function) const { grid_function.SetFromTrueDofs(*this); }

  /**
   * @brief Initialize the true vector from the grid function values
   *
   * This initializes the true vector dofs by multiplying the finite element dofs
   * by the restriction operator.
   *
   * @see <a href="https://mfem.org/pri-dual-vec/">MFEM documentation</a> for details
   */
  void initializeTrueVec(const mfem::ParGridFunction& grid_function) { grid_function.GetTrueDofs(*this); }
};

}  // namespace serac
