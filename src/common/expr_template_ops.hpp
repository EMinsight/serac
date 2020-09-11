// Copyright (c) 2019-2020, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/**
 * @file expr_template_ops.hpp
 *
 * @brief The operator overloads for mfem::Vector
 */

#ifndef EXPR_TEMPLATE_OPS
#define EXPR_TEMPLATE_OPS

#include "common/expr_template_internal.hpp"

template <typename T>
auto operator-(serac::VectorExpr<T>&& u)
{
  return serac::internal::UnaryNegation<T>(std::move(u.asDerived()));
}

inline auto operator-(const mfem::Vector& u) { return serac::internal::UnaryNegation<mfem::Vector>(u); }

inline auto operator-(mfem::Vector&& u) { return serac::internal::UnaryNegation<mfem::Vector&&>(std::move(u)); }

template <typename T>
auto operator*(serac::VectorExpr<T>&& u, const double a)
{
  using serac::internal::ScalarMultOp;
  return serac::internal::UnaryVectorExpr<T, ScalarMultOp>(std::move(u.asDerived()), ScalarMultOp{a});
}

template <typename T>
auto operator*(const double a, serac::VectorExpr<T>&& u)
{
  return operator*(std::move(u), a);
}

inline auto operator*(const double a, const mfem::Vector& u)
{
  using serac::internal::ScalarMultOp;
  return serac::internal::UnaryVectorExpr<mfem::Vector, ScalarMultOp>(u, ScalarMultOp{a});
}

inline auto operator*(const mfem::Vector& u, const double a) { return operator*(a, u); }

inline auto operator*(const double a, mfem::Vector&& u)
{
  using serac::internal::ScalarMultOp;
  return serac::internal::UnaryVectorExpr<mfem::Vector&&, ScalarMultOp>(std::move(u), ScalarMultOp{a});
}

template <typename T>
auto operator/(serac::VectorExpr<T>&& u, const double a)
{
  using serac::internal::ScalarDivOp;
  return serac::internal::UnaryVectorExpr<T, ScalarDivOp<true>>(std::move(u.asDerived()), ScalarDivOp{a});
}

inline auto operator/(const mfem::Vector& u, const double a)
{
  using serac::internal::ScalarDivOp;
  return serac::internal::UnaryVectorExpr<mfem::Vector, ScalarDivOp<true>>(u, ScalarDivOp{a});
}

inline auto operator/(mfem::Vector&& u, const double a)
{
  using serac::internal::ScalarDivOp;
  return serac::internal::UnaryVectorExpr<mfem::Vector&&, ScalarDivOp<true>>(std::move(u), ScalarDivOp{a});
}

template <typename T>
auto operator/(const double a, serac::VectorExpr<T>&& u)
{
  using serac::internal::ScalarDivOp;
  return serac::internal::UnaryVectorExpr<T, ScalarDivOp<false>>(std::move(u.asDerived()), ScalarDivOp<false>{a});
}

inline auto operator/(const double a, const mfem::Vector& u)
{
  using serac::internal::ScalarDivOp;
  return serac::internal::UnaryVectorExpr<mfem::Vector, ScalarDivOp<false>>(u, ScalarDivOp<false>{a});
}

inline auto operator/(const double a, mfem::Vector&& u)
{
  using serac::internal::ScalarDivOp;
  return serac::internal::UnaryVectorExpr<mfem::Vector&&, ScalarDivOp<false>>(std::move(u), ScalarDivOp<false>{a});
}

template <typename S, typename T>
auto operator+(serac::VectorExpr<S>&& u, serac::VectorExpr<T>&& v)
{
  return serac::internal::VectorAddition<S, T>(std::move(u.asDerived()), std::move(v.asDerived()));
}

template <typename T>
auto operator+(const mfem::Vector& u, serac::VectorExpr<T>&& v)
{
  return serac::internal::VectorAddition<mfem::Vector, T>(u, std::move(v.asDerived()));
}

template <typename T>
auto operator+(serac::VectorExpr<T>&& u, const mfem::Vector& v)
{
  return operator+(v, std::move(u));
}

inline auto operator+(const mfem::Vector& u, const mfem::Vector& v)
{
  return serac::internal::VectorAddition<mfem::Vector, mfem::Vector>(u, v);
}

template <typename T>
auto operator+(mfem::Vector&& u, serac::VectorExpr<T>&& v)
{
  return serac::internal::VectorAddition<mfem::Vector&&, T>(std::move(u), std::move(v.asDerived()));
}

template <typename T>
auto operator+(serac::VectorExpr<T>&& u, mfem::Vector&& v)
{
  return operator+(std::move(v), std::move(u));
}

inline auto operator+(mfem::Vector&& u, mfem::Vector&& v)
{
  return serac::internal::VectorAddition<mfem::Vector&&, mfem::Vector&&>(std::move(u), std::move(v));
}

inline auto operator+(const mfem::Vector& u, mfem::Vector&& v)
{
  return serac::internal::VectorAddition<mfem::Vector, mfem::Vector&&>(u, std::move(v));
}

inline auto operator+(mfem::Vector&& u, const mfem::Vector& v) { return operator+(v, std::move(u)); }

template <typename S, typename T>
auto operator-(serac::VectorExpr<S>&& u, serac::VectorExpr<T>&& v)
{
  return serac::internal::VectorSubtraction<S, T>(std::move(u.asDerived()), std::move(v.asDerived()));
}

template <typename T>
auto operator-(const mfem::Vector& u, serac::VectorExpr<T>&& v)
{
  return serac::internal::VectorSubtraction<mfem::Vector, T>(u, std::move(v.asDerived()));
}

template <typename T>
auto operator-(serac::VectorExpr<T>&& u, const mfem::Vector& v)
{
  return serac::internal::VectorSubtraction<T, mfem::Vector>(std::move(u.asDerived()), v);
}

inline auto operator-(const mfem::Vector& u, const mfem::Vector& v)
{
  return serac::internal::VectorSubtraction<mfem::Vector, mfem::Vector>(u, v);
}

template <typename T>
auto operator-(mfem::Vector&& u, serac::VectorExpr<T>&& v)
{
  return serac::internal::VectorSubtraction<mfem::Vector&&, T>(std::move(u), std::move(v.asDerived()));
}

template <typename T>
auto operator-(serac::VectorExpr<T>&& u, mfem::Vector&& v)
{
  return serac::internal::VectorSubtraction<T, mfem::Vector&&>(std::move(u.asDerived()), std::move(v));
}

inline auto operator-(mfem::Vector&& u, mfem::Vector&& v)
{
  return serac::internal::VectorSubtraction<mfem::Vector&&, mfem::Vector&&>(std::move(u), std::move(v));
}

inline auto operator-(const mfem::Vector& u, mfem::Vector&& v)
{
  return serac::internal::VectorSubtraction<mfem::Vector, mfem::Vector&&>(u, std::move(v));
}

inline auto operator-(mfem::Vector&& u, const mfem::Vector& v)
{
  return serac::internal::VectorSubtraction<mfem::Vector&&, mfem::Vector>(std::move(u), v);
}

template <typename T>
auto operator*(const mfem::Operator& A, serac::VectorExpr<T>&& v)
{
  return serac::internal::OperatorExpr<T>(A, std::move(v.asDerived()));
}

inline auto operator*(const mfem::Operator& A, const mfem::Vector& v)
{
  return serac::internal::OperatorExpr<mfem::Vector>(A, v);
}

#endif
