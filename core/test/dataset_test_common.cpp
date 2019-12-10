// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <algorithm>

#include "dataset_test_common.h"

Variable makeRandom(const Dimensions &dims) {
  Random rand;
  return makeVariable<double>(Dimensions{dims}, Values(rand(dims.volume())));
}

DatasetFactory3D::DatasetFactory3D(const scipp::index lx_,
                                   const scipp::index ly_,
                                   const scipp::index lz_)
    : lx(lx_), ly(ly_), lz(lz_) {
  base.setCoord(Dim::Time, makeVariable<double>(Values{rand(1).front()}));
  base.setCoord(Dim::X,
                makeVariable<double>(Dimensions{Dim::X, lx}, Values(rand(lx))));
  base.setCoord(Dim::Y,
                makeVariable<double>(Dimensions{Dim::Y, ly}, Values(rand(ly))));
  base.setCoord(
      Dim::Z,
      makeVariable<double>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}, {Dim::Z, lz}},
                           Values(rand(lx * ly * lz))));

  base.setLabels("labels_x", makeVariable<double>(Dimensions{Dim::X, lx},
                                                  Values(rand(lx))));
  base.setLabels("labels_xy",
                 makeVariable<double>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
                                      Values(rand(lx * ly))));
  base.setLabels("labels_z", makeVariable<double>(Dimensions{Dim::Z, lz},
                                                  Values(rand(lz))));

  base.setMask("masks_x",
               makeVariable<bool>(
                   Dimensions{Dim::X, lx},
                   Values(makeBools<BoolsGeneratorType::ALTERNATING>(lx))));
  base.setMask(
      "masks_xy",
      makeVariable<bool>(
          Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
          Values(makeBools<BoolsGeneratorType::ALTERNATING>(lx * ly))));
  base.setMask("masks_z",
               makeVariable<bool>(
                   Dimensions{Dim::Z, lz},
                   Values(makeBools<BoolsGeneratorType::ALTERNATING>(lz))));

  base.setAttr("attr_scalar", makeVariable<double>(Values{rand(1).front()}));
  base.setAttr("attr_x",
               makeVariable<double>(Dimensions{Dim::X, lx}, Values(rand(lx))));
}

Dataset DatasetFactory3D::make() {
  Dataset dataset(base);
  dataset.setData("values_x", makeVariable<double>(Dimensions{Dim::X, lx},
                                                   Values(rand(lx))));
  dataset.setData("data_x",
                  makeVariable<double>(Dimensions{Dim::X, lx}, Values(rand(lx)),
                                       Variances(rand(lx))));

  dataset.setData("data_xy",
                  makeVariable<double>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
                                       Values(rand(lx * ly)),
                                       Variances(rand(lx * ly))));

  dataset.setData(
      "data_zyx",
      makeVariable<double>(Dimensions{{Dim::Z, lz}, {Dim::Y, ly}, {Dim::X, lx}},
                           Values(rand(lx * ly * lz)),
                           Variances(rand(lx * ly * lz))));

  dataset.setData(
      "data_xyz",
      makeVariable<double>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}, {Dim::Z, lz}},
                           Values(rand(lx * ly * lz))));

  dataset.setData("data_scalar", makeVariable<double>(Values{rand(1).front()}));

  return dataset;
}

Dataset make_empty() { return Dataset(); }

Dataset make_simple_sparse(std::initializer_list<double> values,
                           std::string key) {
  Dataset ds;
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
  var.sparseValues<double>()[0] = values;
  ds.setData(key, var);
  return ds;
}

Dataset make_sparse_with_coords_and_labels(
    std::initializer_list<double> values,
    std::initializer_list<double> coords_and_labels, std::string key) {
  Dataset ds;

  {
    auto var = makeVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
    var.sparseValues<double>()[0] = values;
    ds.setData(key, var);
  }

  {
    auto var = makeVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
    var.sparseValues<double>()[0] = coords_and_labels;
    ds.setSparseCoord(key, var);
  }

  {
    auto var = makeVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
    var.sparseValues<double>()[0] = coords_and_labels;
    ds.setSparseLabels(key, "l", var);
  }

  return ds;
}

Dataset make_sparse_2d(std::initializer_list<double> values, std::string key) {
  Dataset ds;
  auto var =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2l, Dimensions::Sparse});
  var.sparseValues<double>()[0] = values;
  var.sparseValues<double>()[1] = values;
  ds.setData(key, var);
  return ds;
}

Dataset make_1d_masked() {
  Random random;
  Dataset ds;
  ds.setData("data_x",
             makeVariable<double>(Dimensions{Dim::X, 10}, Values(random(10))));
  ds.setMask("masks_x",
             makeVariable<bool>(
                 Dimensions{Dim::X, 10},
                 Values(makeBools<BoolsGeneratorType::ALTERNATING>(10))));
  return ds;
}
