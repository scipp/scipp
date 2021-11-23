#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/core/spatial_transforms.h"

namespace scipp::core {

SCIPP_CORE_EXPORT eigen_rotation_type eigen_rotation_type::operator*(const eigen_rotation_type &other) const {
    return eigen_rotation_type{(this->mat * other.mat).eval()};
};

SCIPP_CORE_EXPORT Eigen::Matrix3d eigen_rotation_type::operator*(const eigen_scaling_type &other) const {
    return (this->mat * other.mat).eval();
};

SCIPP_CORE_EXPORT Eigen::Matrix3d eigen_rotation_type::operator*(const Eigen::Matrix3d &other) const {
    return (this->mat * other).eval();
};

SCIPP_CORE_EXPORT Eigen::Affine3d eigen_rotation_type::operator*(const eigen_translation_type &other) const {
    return (this->mat * Eigen::Translation<double, 3>(other.vec));
};

SCIPP_CORE_EXPORT Eigen::Affine3d eigen_rotation_type::operator*(const Eigen::Affine3d &other) const {
    return this->mat * other;
};

SCIPP_CORE_EXPORT Eigen::Vector3d eigen_rotation_type::operator*(const Eigen::Vector3d &other) const {
    return this->mat * other;
};



SCIPP_CORE_EXPORT eigen_scaling_type eigen_scaling_type::operator*(const eigen_scaling_type &other) const {
    return eigen_scaling_type{(this->mat * other.mat).eval()};
};

SCIPP_CORE_EXPORT Eigen::Matrix3d eigen_scaling_type::operator*(const eigen_rotation_type &other) const {
    return (this->mat * other.mat).eval();
};

SCIPP_CORE_EXPORT Eigen::Matrix3d eigen_scaling_type::operator*(const Eigen::Matrix3d &other) const {
    return (this->mat * other).eval();
};

SCIPP_CORE_EXPORT Eigen::Affine3d eigen_scaling_type::operator*(const eigen_translation_type &other) const {
    return (this->mat * Eigen::Translation<double, 3>(other.vec));
};

SCIPP_CORE_EXPORT Eigen::Affine3d eigen_scaling_type::operator*(const Eigen::Affine3d &other) const {
    return this->mat * other;
};

SCIPP_CORE_EXPORT Eigen::Vector3d eigen_scaling_type::operator*(const Eigen::Vector3d &other) const {
    return this->mat * other;
};



SCIPP_CORE_EXPORT Eigen::Affine3d eigen_translation_type::operator*(const eigen_scaling_type &other) const {
    return Eigen::Translation<double, 3>(this->vec) * other.mat;
};

SCIPP_CORE_EXPORT Eigen::Affine3d eigen_translation_type::operator*(const eigen_rotation_type &other) const {
    return Eigen::Translation<double, 3>(this->vec) * other.mat;
};

SCIPP_CORE_EXPORT Eigen::Affine3d eigen_translation_type::operator*(const Eigen::Matrix3d &other) const {
    return Eigen::Translation<double, 3>(this->vec) * other;
};

SCIPP_CORE_EXPORT eigen_translation_type eigen_translation_type::operator*(const eigen_translation_type &other) const {
    return eigen_translation_type{(this->vec + other.vec).eval()};
};

SCIPP_CORE_EXPORT Eigen::Affine3d eigen_translation_type::operator*(const Eigen::Affine3d &other) const {
    return Eigen::Translation<double, 3>(this->vec) * other;
};

SCIPP_CORE_EXPORT Eigen::Vector3d eigen_translation_type::operator*(const Eigen::Vector3d &other) const {
    return this->vec + other;
};



SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_translation_type &rhs) {
    return lhs * Eigen::Translation<double, 3>(rhs.vec);
}

SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_rotation_type &rhs) {
    return Eigen::Affine3d(lhs * rhs.mat);
}

SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Affine3d &lhs, const eigen_scaling_type &rhs) {
    return Eigen::Affine3d(lhs * rhs.mat);
}



SCIPP_CORE_EXPORT Eigen::Affine3d operator*(const Eigen::Matrix3d &lhs, const eigen_translation_type &rhs) {
    return lhs * Eigen::Translation<double, 3>(rhs.vec);
}

SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_rotation_type &rhs) {
    return lhs * rhs.mat;
}

SCIPP_CORE_EXPORT Eigen::Matrix3d operator*(const Eigen::Matrix3d &lhs, const eigen_scaling_type &rhs) {
    return lhs * rhs.mat;
}

}