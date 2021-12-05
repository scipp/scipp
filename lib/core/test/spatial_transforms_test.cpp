#include <cmath>
#include <gtest/gtest.h>

#include "scipp/core/element/math.h"
#include "scipp/common/constants.h"
#include "scipp/core/spatial_transforms.h"

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

using namespace scipp;

static const double TOLERANCE = pow(10, -10);

TEST(SpatialTransformsTest, combine_to_linear) {
  Eigen::Quaterniond x1;
  x1 = Eigen::AngleAxisd(pi<double>, Eigen::Vector3d::UnitX());
  Eigen::Matrix3d x2(Eigen::Matrix3d::Identity());

  Eigen::Matrix3d result = scipp::core::Quaternion(x1) * x2;
  Eigen::Matrix3d expected(Eigen::Quaterniond(Eigen::AngleAxisd(pi<double>, Eigen::Vector3d::UnitX())));

  for (int row=0; row<3; ++row) {
      for (int col=0; col<3; ++col) {
          ASSERT_NEAR(result(row, col), expected(row, col), TOLERANCE);
      }
  }
}

TEST(SpatialTransformsTest, combine_to_affine) {
  Eigen::Quaterniond x1;
  x1 = Eigen::AngleAxisd(pi<double>, Eigen::Vector3d::UnitX());
  Eigen::Affine3d x2(Eigen::Affine3d::Identity());

  Eigen::Affine3d result = scipp::core::Quaternion(x1) * x2;
  Eigen::Affine3d expected(Eigen::Quaterniond(Eigen::AngleAxisd(pi<double>, Eigen::Vector3d::UnitX())));

  for (int row=0; row<4; ++row) {
      for (int col=0; col<4; ++col) {
          ASSERT_NEAR(result(row, col), expected(row, col), TOLERANCE);
      }
  }
}

TEST(SpatialTransformsTest, apply_rotation_to_vector) {
  Eigen::Quaterniond x1;
  x1 = Eigen::AngleAxisd(pi<double>, Eigen::Vector3d::UnitX());
  Eigen::Vector3d x2(Eigen::Vector3d(1, 2, 3));

  Eigen::Vector3d result = scipp::core::Quaternion(x1) * x2;
  Eigen::Vector3d expected(1, -2, -3);

  for (int i=0; i<3; ++i) {
      ASSERT_NEAR(result(i), expected(i), TOLERANCE);
  }
}

TEST(SpatialTransformsTest, apply_translation_to_vector) {
  scipp::core::Translation x1(Eigen::Vector3d(4, 5, 6));
  Eigen::Vector3d x2(Eigen::Vector3d(1, 2, 3));

  Eigen::Vector3d result = x1 * x2;
  Eigen::Vector3d expected(5, 7, 9);

  for (int i=0; i<3; ++i) {
      ASSERT_NEAR(result(i), expected(i), TOLERANCE);
  }
}

TEST(SpatialTransformsTest, combine_translations) {
  scipp::core::Translation x1(Eigen::Vector3d(1, 2, 3));
  scipp::core::Translation x2(Eigen::Vector3d(-4, 5, 6));

  scipp::core::Translation result = x1 * x2;
  scipp::core::Translation expected(Eigen::Vector3d(-3, 7, 9));

  for (int i=0; i<3; ++i) {
      ASSERT_NEAR(result(i), expected(i), TOLERANCE);
  }
}

TEST(SpatialTransformsTest, combine_rotations) {
  Eigen::Quaterniond x1;
  x1 = Eigen::AngleAxisd(pi<double>/2.0, Eigen::Vector3d::UnitX());
  Eigen::Quaterniond x2;
  x2 = Eigen::AngleAxisd(pi<double>/2.0, Eigen::Vector3d::UnitX());

  scipp::core::Quaternion result = scipp::core::Quaternion(x1) * scipp::core::Quaternion(x2);
  scipp::core::Quaternion expected(Eigen::Quaterniond(Eigen::AngleAxisd(pi<double>, Eigen::Vector3d::UnitX())));

  for (int i=0; i<4; ++i) {
      ASSERT_NEAR(result(i), expected(i), TOLERANCE);
  }
}

