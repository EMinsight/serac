// Copyright (c) 2019-2023, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/**
 * @file equation_solver.hpp
 *
 * @brief This file contains the declaration of an equation solver wrapper
 */

#pragma once

#include <memory>
#include <optional>
#include <variant>

#include "mfem.hpp"

#include "serac/infrastructure/input.hpp"
#include "serac/numerics/solver_config.hpp"

namespace serac::mfem_ext {

/**
 * @brief Wraps a (currently iterative) system solver and handles the configuration of linear
 * or nonlinear solvers.  This class solves a generic global system of (possibly) nonlinear algebraic equations.
 */
class EquationSolver : public mfem::Solver {
public:
  // Allow for the creation of an "empty" EquationSolver to be later overwritten with a "real" constructor
  EquationSolver() = default;
  /**
   * Constructs a new solver wrapper
   * @param[in] comm The MPI communicator object
   * @param[in] lin_options The parameters for the linear solver
   * @param[in] nonlin_options The optional parameters for the optional nonlinear solver
   * @see serac::LinearSolverOptions
   * @see serac::NonlinearSolverOptions
   */
  EquationSolver(std::unique_ptr<mfem::NewtonSolver> nonlinear_solver,
                 std::unique_ptr<mfem::Solver>       linear_solver  = nullptr,
                 std::unique_ptr<mfem::Solver>       preconditioner = nullptr);

  /**
   * Updates the solver with the provided operator
   * @param[in] op The operator (system matrix) to use, "A" in Ax = b
   * @note Implements mfem::Operator::SetOperator
   */
  void SetOperator(const mfem::Operator& op) override;

  /**
   * Solves the system
   * @param[in] b RHS of the system of equations
   * @param[out] x Solution to the system of equations
   * @note Implements mfem::Operator::Mult
   */
  void Mult(const mfem::Vector& b, mfem::Vector& x) const override;

  /**
   * Returns the underlying solver object
   * @return A non-owning reference to the underlying nonlinear solver
   */
  mfem::Solver* NonlinearSolver() { return nonlin_solver_.get(); }

  /**
   * @overload
   */
  const mfem::NewtonSolver* NonlinearSolver() const { return nonlin_solver_.get(); }

  /**
   * Returns the underlying linear solver object
   * @return A non-owning reference to the underlying linear solver
   */
  mfem::Solver* LinearSolver() { return lin_solver_.get(); }

  /**
   * @overload
   */
  const mfem::Solver* LinearSolver() const { return lin_solver_.get(); }

  /**
   * Returns the underlying linear solver object
   * @return A non-owning reference to the underlying linear solver
   */
  mfem::Solver* Preconditioner() { return preconditioner_.get(); }

  /**
   * @overload
   */
  const mfem::Solver* Preconditioner() const { return preconditioner_.get(); }

  /**
   * Input file parameters specific to this class
   **/
  static void DefineInputFileSchema(axom::inlet::Container& container);

private:
  /**
   * @brief The preconditioner (used for an iterative solver only)
   */
  std::unique_ptr<mfem::Solver> preconditioner_;

  /**
   * @brief The linear solver object, either custom, direct (SuperLU), or iterative
   */
  std::unique_ptr<mfem::Solver> lin_solver_;

  /**
   * @brief The optional nonlinear solver object
   */
  std::unique_ptr<mfem::NewtonSolver> nonlin_solver_;

  /**
   * @brief Whether the solver (linear solver) has been configured with the nonlinear solver
   * @note This is a workaround as some nonlinear solvers require SetOperator to be called
   * before SetSolver
   */
  bool nonlin_solver_set_solver_called_ = false;
};

/**
 * @brief A wrapper class for using the MFEM super LU solver with a HypreParMatrix
 */
class SuperLUSolver : public mfem::Solver {
public:
  /**
   * @brief Constructs a wrapper over an mfem::SuperLUSolver
   * @param[in] comm The MPI communicator used by the vectors and matrices in the solve
   * @param[in] options The direct solver configuration parameters struct
   */
  SuperLUSolver(int print_level, MPI_Comm comm) : superlu_solver_(comm)
  {
    superlu_solver_.SetColumnPermutation(mfem::superlu::PARMETIS);
    if (print_level == 0) {
      superlu_solver_.SetPrintStatistics(false);
    }
  }

  /**
   * @brief Factor and solve the linear system y = Op^{-1} x using DSuperLU
   *
   * @param x The input RHS vector
   * @param y The output solution vector
   */
  void Mult(const mfem::Vector& x, mfem::Vector& y) const;

  /**
   * @brief Set the underlying matrix operator to use in the solution algorithm
   *
   * @param op The matrix operator to factorize with SuperLU
   * @pre This operator must be an assembled HypreParMatrix for compatibility with SuperLU
   */
  void SetOperator(const mfem::Operator& op);

private:
  /**
   * @brief The owner of the SuperLU matrix for the gradient, stored
   * as a member variable for lifetime purposes
   */
  mutable std::unique_ptr<mfem::SuperLURowLocMatrix> superlu_mat_;

  /**
   * @brief The underlying MFEM-based superLU solver. It requires a special
   * superLU matrix type which we store in this object. This enables compatibility
   * with HypreParMatrix when used as an input.
   */
  mfem::SuperLUSolver superlu_solver_;
};

std::unique_ptr<EquationSolver> buildEquationSolver(NonlinearSolverOptions nonlinear_opts = {},
                                                    LinearSolverOptions lin_opts = {}, MPI_Comm comm = MPI_COMM_WORLD);

std::unique_ptr<mfem::NewtonSolver> buildNonlinearSolver(NonlinearSolverOptions nonlinear_opts = {},
                                                         MPI_Comm               comm           = MPI_COMM_WORLD);

std::pair<std::unique_ptr<mfem::Solver>, std::unique_ptr<mfem::Solver>> buildLinearSolverAndPreconditioner(
    LinearSolverOptions linear_opts = {}, MPI_Comm comm = MPI_COMM_WORLD);

std::unique_ptr<mfem::Solver> buildPreconditioner(Preconditioner preconditioner, int print_level = 0);

}  // namespace serac::mfem_ext

/**
 * @brief Prototype the specialization for Inlet parsing
 *
 * @tparam The object to be created by inlet
 */
template <>
struct FromInlet<serac::LinearSolverOptions> {
  /// @brief Returns created object from Inlet container
  serac::LinearSolverOptions operator()(const axom::inlet::Container& base);
};

/**
 * @brief Prototype the specialization for Inlet parsing
 *
 * @tparam The object to be created by inlet
 */
template <>
struct FromInlet<serac::NonlinearSolverOptions> {
  /// @brief Returns created object from Inlet container
  serac::NonlinearSolverOptions operator()(const axom::inlet::Container& base);
};

/**
 * @brief Prototype the specialization for Inlet parsing
 *
 * @tparam The object to be created by inlet
 */
template <>
struct FromInlet<serac::mfem_ext::EquationSolver> {
  /// @brief Returns created object from Inlet container
  serac::mfem_ext::EquationSolver operator()(const axom::inlet::Container& base);
};