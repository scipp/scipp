// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
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
