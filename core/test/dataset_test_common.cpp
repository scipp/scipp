// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <algorithm>

#include "dataset_test_common.h"

Variable makeRandom(const Dimensions &dims) {
  Random rand;
  return makeVariable<double>(dims, rand(dims.volume()));
}

DatasetFactory3D::DatasetFactory3D(const scipp::index lx_,
                                   const scipp::index ly_,
                                   const scipp::index lz_)
    : lx(lx_), ly(ly_), lz(lz_) {
  base.setCoord(Dim::Time, makeVariable<double>(rand(1).front()));
  base.setCoord(Dim::X, makeVariable<double>({Dim::X, lx}, rand(lx)));
  base.setCoord(Dim::Y, makeVariable<double>({Dim::Y, ly}, rand(ly)));
  base.setCoord(Dim::Z,
                makeVariable<double>({{Dim::X, lx}, {Dim::Y, ly}, {Dim::Z, lz}},
                                     rand(lx * ly * lz)));

  base.setLabels("labels_x", makeVariable<double>({Dim::X, lx}, rand(lx)));
  base.setLabels("labels_xy", makeVariable<double>({{Dim::X, lx}, {Dim::Y, ly}},
                                                   rand(lx * ly)));
  base.setLabels("labels_z", makeVariable<double>({Dim::Z, lz}, rand(lz)));
  base.setMasks("masks_x", makeVariable<bool>(
                              {Dim::X, lx},
                              makeBools<BoolsGeneratorType::ALTERNATING>(lx)));
  base.setMasks("masks_xy",
                makeVariable<bool>(
                    {{Dim::X, lx}, {Dim::Y, ly}},
                    makeBools<BoolsGeneratorType::ALTERNATING>(lx * ly)));
  base.setMasks("masks_z", makeVariable<bool>(
                              {Dim::Z, lz},
                              makeBools<BoolsGeneratorType::ALTERNATING>(lz)));

  base.setAttr("attr_scalar", makeVariable<double>(rand(1).front()));
  base.setAttr("attr_x", makeVariable<double>({Dim::X, lx}, rand(lx)));
}

Dataset DatasetFactory3D::make() {
  Dataset dataset(base);
  dataset.setData("values_x", makeVariable<double>({Dim::X, lx}, rand(lx)));
  dataset.setData("data_x",
                  makeVariable<double>({Dim::X, lx}, rand(lx), rand(lx)));

  dataset.setData("data_xy",
                  makeVariable<double>({{Dim::X, lx}, {Dim::Y, ly}},
                                       rand(lx * ly), rand(lx * ly)));

  dataset.setData("data_zyx", makeVariable<double>(
                                  {{Dim::Z, lz}, {Dim::Y, ly}, {Dim::X, lx}},
                                  rand(lx * ly * lz), rand(lx * ly * lz)));

  dataset.setData("data_xyz", makeVariable<double>(
                                  {{Dim::X, lx}, {Dim::Y, ly}, {Dim::Z, lz}},
                                  rand(lx * ly * lz)));

  dataset.setData("data_scalar", makeVariable<double>(rand(1).front()));

  return dataset;
}

Dataset make_empty() { return Dataset(); }

Dataset make_simple_sparse(std::initializer_list<double> values,
                           std::string key) {
  Dataset ds;
  auto var = makeVariable<double>({Dim::X, Dimensions::Sparse});
  var.sparseValues<double>()[0] = values;
  ds.setData(key, var);
  return ds;
}

Dataset make_sparse_with_coords_and_labels(
    std::initializer_list<double> values,
    std::initializer_list<double> coords_and_labels, std::string key) {
  Dataset ds;

  {
    auto var = makeVariable<double>({Dim::X, Dimensions::Sparse});
    var.sparseValues<double>()[0] = values;
    ds.setData(key, var);
  }

  {
    auto var = makeVariable<double>({Dim::X, Dimensions::Sparse});
    var.sparseValues<double>()[0] = coords_and_labels;
    ds.setSparseCoord(key, var);
  }

  {
    auto var = makeVariable<double>({Dim::X, Dimensions::Sparse});
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

  Dataset ds;
  ds.setData("data_x", makeVariable<double>({Dim::X, 10}, random(10)));
  ds.setMasks("masks_x", makeVariable<bool>(
                             {Dim::X, 10},
                             makeBools<BoolsGeneratorType::ALTERNATING>(10)));
  return ds;
}
