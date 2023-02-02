// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/core/format.h"
#include "scipp/core/string.h"

using namespace scipp;
using namespace scipp::core;

TEST(FormatTest, supports_types) {
  const auto &f = FormatRegistry::instance();
  EXPECT_NO_THROW(f.format(int64_t{1}));
  EXPECT_NO_THROW(f.format(int32_t{1}));
  EXPECT_NO_THROW(f.format(double{1}));
  EXPECT_NO_THROW(f.format(float{1}));
  EXPECT_NO_THROW(f.format(false));
  EXPECT_NO_THROW(f.format(std::string("string")));
  EXPECT_NO_THROW(f.format(Eigen::Vector3d{}));
  EXPECT_NO_THROW(f.format(Eigen::Matrix3d{}));
  EXPECT_NO_THROW(f.format(Eigen::Affine3d{}));
  EXPECT_NO_THROW(f.format(Quaternion{}));
  EXPECT_NO_THROW(f.format(Translation{}));
  EXPECT_NO_THROW(f.format(index_pair{}));
  EXPECT_NO_THROW(f.format(time_point{}, FormatSpec{"", units::s}));
}

TEST(FormatTest, time_point_requires_unit) {
  const auto &f = FormatRegistry::instance();
  EXPECT_THROW(f.format(time_point{}), std::invalid_argument);
}

TEST(FormatTest, produces_expected_result) {
  const auto &f = FormatRegistry::instance();
  EXPECT_EQ(f.format(int64_t{19862}), "19862");
  EXPECT_EQ(f.format(true), "True");
  EXPECT_EQ(f.format(time_point(79819862), FormatSpec{"", units::s}),
            to_iso_date(time_point(79819862), units::s));
}

TEST(FormatTest, raises_for_unsupported_type) {
  const auto &f = FormatRegistry::instance();
  const std::unordered_map<double, int64_t> value;
  EXPECT_THROW(f.format(value), std::invalid_argument);
}

TEST(FormatTest, can_customize_formatters) {
  auto f = FormatRegistry::instance();
  bool called_custom_formatter = false;
  f.set(dtype<int32_t>, [&](const std::any &, const FormatSpec &,
                            const FormatRegistry &formatters) mutable {
    called_custom_formatter = true;
    EXPECT_EQ(&formatters, &f);
    return std::string("custom string");
  });
  EXPECT_EQ(f.format(int32_t{123}), "custom string");
  EXPECT_TRUE(called_custom_formatter);
}

TEST(FormatTest, can_pass_spec) {
  auto f = FormatRegistry::instance();
  f.set(dtype<int32_t>, [&](const std::any &, const FormatSpec &spec,
                            const FormatRegistry &) mutable {
    EXPECT_EQ(spec.full(), "spec:nested");
    EXPECT_EQ(spec.unit.value(), units::kg);
    return std::string("");
  });
  f.format(int32_t{123}, FormatSpec{"spec:nested", units::kg});
}

TEST(FormatTest, spec_iterates_correctly) {
  const FormatSpec s1{"<#2:.5f::s", units::s};
  EXPECT_EQ(s1.full(), "<#2:.5f::s");
  EXPECT_EQ(s1.current(), "<#2");
  EXPECT_EQ(s1.unit.value(), units::s);
  const auto s2 = s1.nested();
  EXPECT_EQ(s2.full(), ".5f::s");
  EXPECT_EQ(s2.current(), ".5f");
  EXPECT_FALSE(s2.unit.has_value());
  const auto s3 = s2.nested();
  EXPECT_EQ(s3.full(), ":s");
  EXPECT_EQ(s3.current(), "");
  EXPECT_FALSE(s3.unit.has_value());
  const auto s4 = s3.nested();
  EXPECT_EQ(s4.full(), "s");
  EXPECT_EQ(s4.current(), "s");
  EXPECT_FALSE(s4.unit.has_value());
  const auto s5 = s4.nested();
  EXPECT_EQ(s5.full(), "");
  EXPECT_EQ(s5.current(), "");
  EXPECT_FALSE(s5.unit.has_value());
  const auto s6 = s5.nested();
  EXPECT_EQ(s6.full(), "");
  EXPECT_EQ(s6.current(), "");
  EXPECT_FALSE(s6.unit.has_value());
}
