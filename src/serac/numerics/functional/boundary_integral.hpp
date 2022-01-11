// Copyright (c) 2019-2021, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/**
 * @file boundary_integral.hpp
 *
 * @brief This file defines a class, BoundaryIntegral, for integrating q-functions against finite element
 * basis functions on a mesh boundary region.
 */
#pragma once

#include "mfem.hpp"
#include "mfem/linalg/dtensor.hpp"

#include "serac/numerics/functional/tensor.hpp"
#include "serac/numerics/functional/quadrature.hpp"
#include "serac/numerics/functional/tuple_arithmetic.hpp"
#include "serac/numerics/functional/integral_utilities.hpp"
#include "serac/numerics/functional/boundary_integral_kernels.hpp"
#if defined(__CUDACC__)
#include "serac/numerics/functional/boundary_integral_kernels.cuh"
#endif

namespace serac {

template <typename spaces, ExecutionSpace exec>
class BoundaryIntegral;

/**
 * @brief Describes a single boundary integral term in a weak forumulation of a partial differential equation
 * @tparam spaces A @p std::function -like set of template parameters that describe the test and trial
 * function spaces, i.e., @p test(trial)
 * @tparam exec whether or not the calculation and memory will be on the CPU or GPU
 */
template <typename test, typename... trials, ExecutionSpace exec>
class BoundaryIntegral<test(trials...), exec> {
public:

  static constexpr tuple<trials...> trial_spaces{};
  static constexpr int              num_trial_spaces = sizeof...(trials);

  /**
   * @brief Constructs an @p BoundaryIntegral from a user-provided quadrature function
   * @tparam dim The dimension of the element (2 for quad, 3 for hex, etc)
   * @tparam qpt_data_type The type of the data to store for each quadrature point
   * @param[in] num_elements The number of elements in the mesh
   * @param[in] J The Jacobians of the element transformations at all quadrature points
   * @param[in] X The actual (not reference) coordinates of all quadrature points
   * @param[in] normals The unit normals of all quadrature points
   * @see mfem::GeometricFactors
   * @param[in] qf The user-provided quadrature function
   * @note The @p Dimension parameters are used to assist in the deduction of the dim template parameter
   */
  template <int dim, typename lambda_type, typename qpt_data_type = void>
  BoundaryIntegral(size_t num_elements, const mfem::Vector& J, const mfem::Vector& X, const mfem::Vector& N, Dimension<dim>, lambda_type&& qf)
      : J_(J), X_(X), normals_(N)

  {
    using namespace boundary_integral;

    constexpr auto geometry                      = supported_geometries[dim];
    constexpr auto Q                             = std::max({test::order, trials::order...}) + 1;
    constexpr auto quadrature_points_per_element = (dim == 2) ? Q * Q : Q * Q * Q;

    // this is where we actually specialize the finite element kernel templates with
    // our specific requirements (element type, test/trial spaces, quadrature rule, q-function, etc).
    //
    // std::function's type erasure lets us wrap those specific details inside a function with known signature
    if constexpr (exec == ExecutionSpace::CPU) {
      KernelConfig<Q, geometry, test, trials...> eval_config;

      evaluation_ = EvaluationKernel{eval_config, J, X, N, num_elements, qf};

      for_constexpr<num_trial_spaces>([this, num_elements, quadrature_points_per_element, &J, &X, &N, &qf, eval_config](auto i) {
        // allocate memory for the derivatives of the q-function at each quadrature point
        //
        // Note: ptrs' lifetime is managed in an unusual way! It is captured by-value in the
        // action_of_gradient functor below to augment the reference count, and extend its lifetime to match
        // that of the DomainIntegral that allocated it.
        using which_trial_space = decltype(get<i>(trial_spaces));
        using derivative_type   = decltype(get_derivative_type<i, dim, trials...>(qf));
        auto ptr = accelerator::make_shared_array<exec, derivative_type>(num_elements * quadrature_points_per_element);
        ArrayView<derivative_type, 2, exec> qf_derivatives(ptr.get(), num_elements, quadrature_points_per_element);

        evaluation_with_AD_[i] =
            EvaluationKernel{DerivativeWRT<i>{}, eval_config, qf_derivatives, J, X, N, num_elements, qf};

        // note: this lambda function captures ptr by-value to extend its lifetime
        //                        vvv
        action_of_gradient_[i] = [ptr, qf_derivatives, num_elements, J](const mfem::Vector& dU, mfem::Vector& dR) {
          domain_integral::action_of_gradient_kernel<geometry, test, which_trial_space, Q>(dU, dR, qf_derivatives, J,
                                                                                           num_elements);
        };

        element_gradient_[i] = [qf_derivatives, num_elements, J](CPUView<double, 3> K_e) {
          domain_integral::element_gradient_kernel<geometry, test, which_trial_space, Q>(K_e, qf_derivatives, J,
                                                                                         num_elements);
        };
      });
    }

#if 0
    SLIC_ERROR_ROOT_IF(exec == ExecutionSpace::GPU, "BoundaryIntegral doesn't currently support GPU kernels yet");

    constexpr auto geometry                      = supported_geometries[dim];
    constexpr auto Q                             = std::max(test_space::order, trial_space::order) + 1;
    constexpr auto quadrature_points_per_element = detail::pow(Q, dim);

    uint32_t num_quadrature_points = quadrature_points_per_element * uint32_t(num_elements);

    // these lines of code figure out the argument types that will be passed
    // into the quadrature function in the finite element kernel.
    //
    // we use them to observe the output type and allocate memory to store
    // the derivative information at each quadrature point
    using x_t             = tensor<double, dim + 1>;
    using u_du_t          = typename detail::lambda_argument<trial_space, dim, dim + 1>::type;
    using derivative_type = decltype(get_gradient(qf(x_t{}, x_t{}, make_dual(u_du_t{}))));

    // allocate memory for the derivatives of the q-function at each quadrature point
    //
    // Note: ptr's lifetime is managed in an unusual way! It is captured by-value in one of the
    // lambda functions below to augment the reference count, and extend its lifetime to match
    // that of the BoundaryIntegral that allocated it.

    // TODO: change this allocation to use exec, rather than ExecutionSpace::CPU, once
    // we implement GPU boundary kernels
    auto ptr = accelerator::make_shared_array<derivative_type, ExecutionSpace::CPU>(num_quadrature_points);

    size_t                      n1 = static_cast<size_t>(num_elements);
    size_t                      n2 = static_cast<size_t>(quadrature_points_per_element);
    CPUView<derivative_type, 2> qf_derivatives{ptr.get(), n1, n2};

    // this is where we actually specialize the finite element kernel templates with
    // our specific requirements (element type, test/trial spaces, quadrature rule, q-function, etc).
    //
    // std::function's type erasure lets us wrap those specific details inside a function with known signature
    //
    // this lambda function captures ptr by-value to extend its lifetime
    //                   vvv
    evaluation_ = [this, ptr, qf_derivatives, num_elements, qf](const std::array< mfem::Vector, num_trial_spaces > & U, mfem::Vector& R) {
      boundary_integral::evaluation_kernel<geometry, test, trials ..., Q>(U[0], R, qf_derivatives, J_, X_, normals_,
                                                                                 num_elements, qf);
    };

    action_of_gradient_ = [this, qf_derivatives, num_elements](const std::array< mfem::Vector, num_trial_spaces > & dU, mfem::Vector& dR) {
      boundary_integral::action_of_gradient_kernel<geometry, test, trials ... , Q>(dU[0], dR, qf_derivatives, J_,
                                                                                         num_elements);
    };

    element_gradient_ = [this, qf_derivatives, num_elements](ArrayView<double, 3, exec> K_b) {
      boundary_integral::element_gradient_kernel<geometry, test, trials ..., Q>(K_b, qf_derivatives, J_,
                                                                                       num_elements);
    };
#endif
  }

  /**
   * @brief Applies the integral, i.e., @a output_E = evaluate( @a input_E )
   * @param[in] input_E The input to the evaluation; per-element DOF values
   * @param[out] output_E The output of the evalution; per-element DOF residuals
   * @see evaluation_kernel
   */
  void Mult(const std::array<mfem::Vector, num_trial_spaces>& input_E, mfem::Vector& output_E, int which = 0) const
  {
    if (which == -1) {
      evaluation_(input_E, output_E);
    } else {
      evaluation_with_AD_[which](input_E, output_E);
    }
  }

  /**
   * @brief Applies the integral, i.e., @a output_E = gradient( @a input_E )
   * @param[in] input_E The input to the evaluation; per-element DOF values
   * @param[out] output_E The output of the evalution; per-element DOF residuals
   * @see action_of_gradient_kernel
   */
  void GradientMult(const mfem::Vector& input_E, mfem::Vector& output_E, size_t which = 0) const
  {
    action_of_gradient_[which](input_E, output_E);
  }

  /**
   * @brief Computes the derivative of each element's residual with respect to the element values
   * @param[inout] K_b The reshaped vector as a mfem::DeviceTensor of size (test_dim * test_dof, trial_dim * trial_dof,
   * nelems)
   */
  void ComputeElementGradients(ArrayView<double, 3, exec> K_b) const { element_gradient_(K_b); }

private:
  /**
   * @brief Jacobians of the element transformations at all quadrature points
   */
  const mfem::Vector J_;

  /**
   * @brief Mapped (physical) coordinates of all quadrature points
   */
  const mfem::Vector X_;

  /**
   * @brief physical coordinates of surface unit normals at all quadrature points
   */
  const mfem::Vector normals_;


  /**
   * @brief Type-erased handle to evaluation kernel
   * @see evaluation_kernel
   */
  std::function<void(const std::array<mfem::Vector, num_trial_spaces>&, mfem::Vector&)> evaluation_;

  /**
   * @brief Type-erased handle to evaluation kernel
   * @see evaluation_kernel
   */
  std::function<void(const std::array<mfem::Vector, num_trial_spaces>&, mfem::Vector&)>
      evaluation_with_AD_[num_trial_spaces];

  /**
   * @brief Type-erased handle to gradient kernel
   * @see gradient_kernel
   */
  std::function<void(const mfem::Vector&, mfem::Vector&)> action_of_gradient_[num_trial_spaces];

  /**
   * @brief Type-erased handle to gradient matrix assembly kernel
   * @see gradient_matrix_kernel
   */
  std::function<void(ArrayView<double, 3, exec>)> element_gradient_[num_trial_spaces];

};

}  // namespace serac
