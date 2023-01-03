// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <cmath>
#include <initializer_list>

#include "fix_typed_test_suite_warnings.h"

#define EXPECT_THROW_MSG(TRY_BLOCK, EXCEPTION_TYPE, MESSAGE)                   \
  EXPECT_THROW(                                                                \
      {                                                                        \
        try {                                                                  \
          TRY_BLOCK;                                                           \
        } catch (const EXCEPTION_TYPE &e) {                                    \
          EXPECT_STREQ(MESSAGE, e.what());                                     \
          throw;                                                               \
        }                                                                      \
      },                                                                       \
      EXCEPTION_TYPE);

#define EXPECT_THROW_MSG_SUBSTR(TRY_BLOCK, EXCEPTION_TYPE, SUBSTR)             \
  EXPECT_THROW(                                                                \
      {                                                                        \
        try {                                                                  \
          TRY_BLOCK;                                                           \
        } catch (const EXCEPTION_TYPE &e) {                                    \
          EXPECT_TRUE(std::string(e.what()).find(SUBSTR) !=                    \
                      std::string::npos);                                      \
          throw;                                                               \
        }                                                                      \
      },                                                                       \
      EXCEPTION_TYPE);

template <class T1, class T2>
bool equals(const T1 &a, const std::initializer_list<T2> &b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template <class T1, class T2> bool equals(const T1 &a, const T2 &b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template <class T1, class T2, typename Tol>
bool equals(const T1 &a, const T2 &b, const Tol tolerance) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                    [tolerance](const auto &aa, const auto &bb) {
                      return std::abs(aa - bb) < tolerance;
                    });
}

/// Macros to avoid warnings when testing [[nodiscard]] qualified functions
#define ASSERT_NO_THROW_DISCARD(expr) ASSERT_NO_THROW(static_cast<void>(expr))
#define ASSERT_ANY_THROW_DISCARD(expr) ASSERT_ANY_THROW(static_cast<void>(expr))
#define ASSERT_THROW_DISCARD(expr, type)                                       \
  ASSERT_THROW(static_cast<void>(expr), type)
#define EXPECT_NO_THROW_DISCARD(expr) EXPECT_NO_THROW(static_cast<void>(expr))
#define EXPECT_THROW_DISCARD(expr, type)                                       \
  EXPECT_THROW(static_cast<void>(expr), type)
#define EXPECT_THROW_MSG_DISCARD(expr, type, msg)                              \
  EXPECT_THROW_MSG(static_cast<void>(expr), type, msg)
