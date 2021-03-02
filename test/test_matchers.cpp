// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

DataArray sort_bins(DataArray data) {
  for (auto && bin : data.template values<core::bucket<DataArray>>()) {
    const auto &coords = bin.coords();
    if (coords.size() != 1) {
      throw std::invalid_argument(
          "The matcher only works for one dimensional bins.");
    }
    copy(sort(bin, *(coords.values_begin())), bin);
  }
  return data;
}