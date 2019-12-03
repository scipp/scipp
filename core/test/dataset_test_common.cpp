// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <algorithm>

#include "dataset_test_common.h"

Variable makeRandom(const Dimensions &dims) {
  Random rand;
  auto vect = rand(dims.volume());
  return createVariable<double>(Dimensions{dims},
                                Values(vect.begin(), vect.end()));
}

DatasetFactory3D::DatasetFactory3D(const scipp::index lx_,
                                   const scipp::index ly_,
                                   const scipp::index lz_)
    : lx(lx_), ly(ly_), lz(lz_) {
  auto vX = rand(lx);
  auto vY = rand(ly);
  auto vXYZ = rand(lx * ly * lz);
  base.setCoord(Dim::Time, createVariable<double>(Values{rand(1).front()}));
  base.setCoord(Dim::X, createVariable<double>(Dimensions{Dim::X, lx},
                                               Values(vX.begin(), vX.end())));
  base.setCoord(Dim::Y, createVariable<double>(Dimensions{Dim::Y, ly},
                                               Values(vY.begin(), vY.end())));
  base.setCoord(Dim::Z,
                createVariable<double>(
                    Dimensions{{Dim::X, lx}, {Dim::Y, ly}, {Dim::Z, lz}},
                    Values(vXYZ.begin(), vXYZ.end())));

  vX = rand(lx);
  vY = rand(ly);
  auto vZ = rand(lz);
  auto vXY = rand(lx * ly);
  vXYZ = rand(lx * ly * lz);

  base.setLabels("labels_x",
                 createVariable<double>(Dimensions{Dim::X, lx},
                                        Values(vX.begin(), vX.end())));
  base.setLabels("labels_xy",
                 createVariable<double>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
                                        Values(vXY.begin(), vXY.end())));
  base.setLabels("labels_z",
                 createVariable<double>(Dimensions{Dim::Z, lz},
                                        Values(vZ.begin(), vZ.end())));

  auto bX = makeBools<BoolsGeneratorType::ALTERNATING>(lx);
  auto bXY = makeBools<BoolsGeneratorType::ALTERNATING>(lx * ly);
  auto bZ = makeBools<BoolsGeneratorType::ALTERNATING>(lz);
  base.setMask("masks_x", createVariable<bool>(Dimensions{Dim::X, lx},
                                               Values(bX.begin(), bX.end())));
  base.setMask("masks_xy",
               createVariable<bool>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
                                    Values(bXY.begin(), bXY.end())));
  base.setMask("masks_z", createVariable<bool>(Dimensions{Dim::Z, lz},
                                               Values(bZ.begin(), bZ.end())));

  vX = rand(lx);
  base.setAttr("attr_scalar", createVariable<double>(Values{rand(1).front()}));
  base.setAttr("attr_x", createVariable<double>(Dimensions{Dim::X, lx},
                                                Values(vX.begin(), vX.end())));
}

Dataset DatasetFactory3D::make() {
  Dataset dataset(base);
  auto vX = rand(lx);
  auto dvalX = rand(lx);
  auto dvarX = rand(lx);
  auto dvalXY = rand(lx * ly);
  auto dvarXY = rand(lx * ly);
  auto dvalXYZ = rand(lx * ly * lz);
  auto dvalZYX = rand(lx * ly * lz);
  auto dvarZYX = rand(lx * ly * lz);
  dataset.setData("values_x",
                  createVariable<double>(Dimensions{Dim::X, lx},
                                         Values(vX.begin(), vX.end())));
  dataset.setData(
      "data_x", createVariable<double>(Dimensions{Dim::X, lx},
                                       Values(dvalX.begin(), dvalX.end()),
                                       Variances(dvarX.begin(), dvarX.end())));

  dataset.setData("data_xy", createVariable<double>(
                                 Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
                                 Values(dvalXY.begin(), dvalXY.end()),
                                 Variances(dvarXY.begin(), dvarXY.end())));

  dataset.setData("data_zyx",
                  createVariable<double>(
                      Dimensions{{Dim::Z, lz}, {Dim::Y, ly}, {Dim::X, lx}},
                      Values(dvalZYX.begin(), dvalZYX.end()),
                      Variances(dvarZYX.begin(), dvarZYX.end())));

  dataset.setData("data_xyz",
                  createVariable<double>(
                      Dimensions{{Dim::X, lx}, {Dim::Y, ly}, {Dim::Z, lz}},
                      Values(dvalXYZ.begin(), dvalXYZ.end())));

  dataset.setData("data_scalar", makeVariable<double>(rand(1).front()));

  return dataset;
}

Dataset make_empty() { return Dataset(); }

Dataset make_simple_sparse(std::initializer_list<double> values,
                           std::string key) {
  Dataset ds;
  auto var = createVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
  var.sparseValues<double>()[0] = values;
  ds.setData(key, var);
  return ds;
}

Dataset make_sparse_with_coords_and_labels(
    std::initializer_list<double> values,
    std::initializer_list<double> coords_and_labels, std::string key) {
  Dataset ds;

  {
    auto var = createVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
    var.sparseValues<double>()[0] = values;
    ds.setData(key, var);
  }

  {
    auto var = createVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
    var.sparseValues<double>()[0] = coords_and_labels;
    ds.setSparseCoord(key, var);
  }

  {
    auto var = createVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
    var.sparseValues<double>()[0] = coords_and_labels;
    ds.setSparseLabels(key, "l", var);
  }

  return ds;
}

Dataset make_sparse_2d(std::initializer_list<double> values, std::string key) {
  Dataset ds;
  auto var = makeVariable<double>({Dim::X, Dim::Y}, {2, Dimensions::Sparse});
  var.sparseValues<double>()[0] = values;
  var.sparseValues<double>()[1] = values;
  ds.setData(key, var);
  return ds;
}

Dataset make_1d_masked() {
  Random random;
  auto vect = random(10);
  auto bools = makeBools<BoolsGeneratorType::ALTERNATING>(10);
  Dataset ds;
  ds.setData("data_x",
             createVariable<double>(Dimensions{Dim::X, 10},
                                    Values(vect.begin(), vect.end())));
  ds.setMask("masks_x",
             createVariable<bool>(Dimensions{Dim::X, 10},
                                  Values(bools.begin(), bools.end())));
  return ds;
}
