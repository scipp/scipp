// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/shape.h"
#include "scipp/dataset/unaligned.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::dataset;

struct RealignTest : public ::testing::Test {
protected:
  Variable temp = makeVariable<double>(Dims{Dim::Temperature}, Shape{2});
  Variable xbins =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 2, 4});
  Variable ybins =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{0, 2, 4});
  Variable zbins =
      makeVariable<double>(Dims{Dim::Z}, Shape{3}, Values{0, 2, 4});
  Variable temp_mask = makeVariable<bool>(Dims{Dim::Temperature}, Shape{2},
                                          Values{false, false});

  DataArray make_array() {
    const Dim dim = Dim::Position;
    const auto pos = makeVariable<Eigen::Vector3d>(
        Dims{dim}, Shape{4},
        Values{Eigen::Vector3d{1, 1, 1}, Eigen::Vector3d{1, 1, 2},
               Eigen::Vector3d{1, 2, 3}, Eigen::Vector3d{1, 2, 4}});
    const auto x =
        makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 1, 1, 1});
    const auto y =
        makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 1, 2, 2});
    const auto z =
        makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 2, 3, 4});
    const auto pos_mask = makeVariable<bool>(
        Dims{dim}, Shape{4}, Values{false, false, false, false});
    const auto attr = makeVariable<double>(Values{3.14});
    DataArray a(makeVariable<double>(Dims{dim}, Shape{4}, Values{1, 2, 3, 4}),
                {{dim, pos}, {Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}},
                {{"pos", pos_mask}}, {{"attr", attr}});

    a = concatenate(a, a + a, Dim::Temperature);
    a.coords().set(Dim::Temperature, temp);
    a.masks().set("temp", temp_mask);
    return a;
  }

  DataArray make_realigned() {
    return unaligned::realign(
        make_array(), {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  }

  DataArray make_aligned() {
    return DataArray(
        makeVariable<double>(
            Dims{Dim::Temperature, Dim::Z, Dim::Y, Dim::X}, Shape{2, 2, 2, 2},
            Values{1, 0, 0, 0, 2, 0, 3, 0, 2, 0, 0, 0, 4, 0, 6, 0}),
        {{Dim::Temperature, temp},
         {Dim::Z, zbins},
         {Dim::Y, ybins},
         {Dim::X, xbins}},
        {{"temp", temp_mask}});
  }
};

TEST_F(RealignTest, fail_no_unaligned) {
  auto base = make_array();
  EXPECT_THROW(unaligned::realign(base, {}), except::UnalignedError);
}

TEST_F(RealignTest, fail_bad_bin_edge_unit) {
  auto base = make_array();
  xbins.setUnit(units::kg);
  EXPECT_THROW(unaligned::realign(base, {{Dim::X, xbins}}),
               except::UnitMismatchError);
}

TEST_F(RealignTest, fail_missing_event_positions) {
  auto base = make_array();
  // No "row" information in unaligned data
  EXPECT_THROW(
      unaligned::realign(
          base, {{Dim::Row, makeVariable<double>(Dims{Dim::Row}, Shape{2},
                                                 Values{0, 4})}}),
      except::NotFoundError);
}

TEST_F(RealignTest, multiple_unaligned_no_supported_yet) {
  auto base = make_array();
  // Unaligned position and events not supported *yet*.
  base.coords().set(Dim::Tof, makeVariable<event_list<double>>(
                                  Dims{Dim::Position}, Shape{4}));
  EXPECT_THROW(
      unaligned::realign(
          base, {{Dim::Z, zbins},
                 {Dim::Y, ybins},
                 {Dim::X, xbins},
                 {Dim::Tof, makeVariable<double>(Dims{Dim::Tof}, Shape{2},
                                                 Values{0, 1})}}),
      except::UnalignedError);
}

TEST_F(RealignTest, basics) {
  const auto reference = make_aligned();
  auto base = make_array();
  auto realigned = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_FALSE(realigned.hasData());
  EXPECT_EQ(realigned.dims(), reference.dims());
  EXPECT_EQ(realigned.coords(), reference.coords());
  EXPECT_EQ(realigned.unit(), base.unit());
  EXPECT_EQ(realigned.dtype(), base.dtype());

  // Last position is at Z bound and thus excluded by binning in [low, heigh)
  EXPECT_EQ(realigned.unaligned(), base.slice({Dim::Position, 0, 3}));
}

TEST_F(RealignTest, realigned_drop_alignment) {
  auto a = make_realigned();
  a.drop_alignment();
  EXPECT_EQ(a, make_array().slice({Dim::Position, 0, 3}));
}

TEST_F(RealignTest, dataset_change_alignment) {
  auto baseA = make_array();
  auto baseB = concatenate(baseA, baseA, Dim::Position);
  const auto referenceA = unaligned::realign(
      baseA, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  const auto referenceB = unaligned::realign(
      baseB, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  Dataset dataset;
  // Different number of coords and different values
  dataset.setData(
      "a", unaligned::realign(baseA, {{Dim::X, xbins + 0.5 * units::one}}));
  dataset.setData(
      "b", unaligned::realign(baseB, {{Dim::X, xbins + 0.5 * units::one}}));

  const auto realigned = unaligned::realign(
      dataset, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_EQ(realigned["a"], referenceA);
  EXPECT_EQ(realigned["b"], referenceB);
}

TEST_F(RealignTest, rename) {
  auto a = make_realigned();
  a.setName("newname");
  EXPECT_EQ(a.name(), "newname");
  EXPECT_EQ(a.unaligned().name(), "newname");
  a.drop_alignment();
  EXPECT_EQ(a.name(), "newname");
}

TEST_F(RealignTest, dimension_order) {
  auto base = make_array();
  DataArray transposed(Variable(base.data().transpose()), base.coords());
  auto realigned1 = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  auto realigned2 = unaligned::realign(
      transposed, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_FALSE(realigned1.hasData());
  EXPECT_FALSE(realigned2.hasData());
  EXPECT_EQ(
      realigned1.dims(),
      Dimensions({Dim::Temperature, Dim::Z, Dim::Y, Dim::X}, {2, 2, 2, 2}));
  // Dim::Position is outside Dim::Temperature, when mapping position to X, Y,
  // and Z stays the inner dim.
  EXPECT_EQ(
      realigned2.dims(),
      Dimensions({Dim::Z, Dim::Y, Dim::X, Dim::Temperature}, {2, 2, 2, 2}));
}

TEST_F(RealignTest, mask_mapping) {
  auto base = make_array();
  auto realigned = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_EQ(realigned.masks().size(), 1);
  EXPECT_TRUE(realigned.masks().contains("temp"));

  EXPECT_EQ(realigned.unaligned(), base.slice({Dim::Position, 0, 3}));
}

TEST_F(RealignTest, attr_mapping) {
  auto base = make_array();
  base.attrs().set("0-d", makeVariable<double>(Values{1.0}));
  base.attrs().set("pos",
                   makeVariable<bool>(Dims{Dim::Position}, Shape{4},
                                      Values{false, false, false, true}));
  base.attrs().set("temp", makeVariable<bool>(Dims{Dim::Temperature}, Shape{2},
                                              Values{false, true}));
  auto realigned = unaligned::realign(
      base, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});

  EXPECT_FALSE(realigned.hasData());
  EXPECT_EQ(realigned.attrs().size(), 2);
  EXPECT_TRUE(realigned.attrs().contains("0-d"));
  EXPECT_TRUE(realigned.attrs().contains("temp"));

  EXPECT_EQ(realigned.unaligned(), base.slice({Dim::Position, 0, 3}));
}

TEST_F(RealignTest, realigned_bounds) {
  const auto realigned = make_realigned();
  DataArrayConstView view(realigned);

  auto bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 0);

  view = view.slice({Dim::X, 1, 2});
  bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 1);
  EXPECT_EQ(bounds.at(0).first, Dim::X);
  EXPECT_EQ(bounds.at(0).second,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));

  view = view.slice({Dim::Y, 0, 2});
  bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 2);
  EXPECT_EQ(bounds.at(0).first, Dim::X);
  EXPECT_EQ(bounds.at(0).second,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));
  EXPECT_EQ(bounds.at(1).first, Dim::Y);
  EXPECT_EQ(bounds.at(1).second,
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0, 4}));

  // Slice again in same dimension
  view = view.slice({Dim::X, 0});
  bounds = view.slice_bounds();
  EXPECT_EQ(bounds.size(), 2);
  EXPECT_EQ(bounds.at(0).first, Dim::X);
  EXPECT_EQ(bounds.at(0).second,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));
  EXPECT_EQ(bounds.at(1).first, Dim::Y);
  EXPECT_EQ(bounds.at(1).second,
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0, 4}));
}

TEST_F(RealignTest, copy_realigned) {
  const auto realigned = make_realigned();
  EXPECT_EQ(DataArray(realigned), realigned);
  EXPECT_EQ(DataArray(DataArrayConstView(realigned)), realigned);
}

TEST_F(RealignTest, copy_realigned_slice) {
  const auto realigned = make_realigned();
  const auto slice = realigned.slice({Dim::Z, 1});
  // `slice` contains unfiltered unaligned content, but copy drops out-of-bounds
  // content.
  const DataArray copy(slice);
  EXPECT_NE(copy, slice);
  EXPECT_EQ(copy.dims(), slice.dims());
  EXPECT_EQ(copy.coords(), slice.coords());
  EXPECT_EQ(copy.masks(), slice.masks());
  EXPECT_EQ(copy.attrs(), slice.attrs());
  EXPECT_NE(copy.unaligned(), slice.unaligned());
  EXPECT_EQ(copy.unaligned(),
            realigned.unaligned().slice({Dim::Position, 1, 3}));
}

template <class Realigned>
void realign_test_slice(Realigned &&realigned, const DataArray &aligned) {
  for (const auto dim : {Dim::Temperature, Dim::X, Dim::Y, Dim::Z}) {
    for (const auto s : {Slice(dim, 0), Slice(dim, 1), Slice(dim, 0, 1),
                         Slice(dim, 0, 2), Slice(dim, 1, 2)}) {
      const auto slice = realigned.slice(s);
      const auto reference = aligned.slice(s);
      // Same result as when slicing normal array, except for missing data
      EXPECT_FALSE(slice.hasData());
      EXPECT_EQ(slice.dims(), reference.dims());
      EXPECT_EQ(slice.coords(), reference.coords());
      if (dim == Dim::Temperature)
        EXPECT_EQ(slice.unaligned(), realigned.unaligned().slice(s))
            << to_string(s);
      else
        EXPECT_EQ(slice.unaligned(), realigned.unaligned()) << to_string(s);
    }
  }
}

TEST_F(RealignTest, slice) {
  auto realigned = make_realigned();
  const auto aligned = make_aligned();

  realign_test_slice(static_cast<const DataArray &>(realigned), aligned);
  realign_test_slice(realigned, aligned);
}

TEST_F(RealignTest, unaligned_of_slice_along_aligned_dim) {
  const auto realigned = make_realigned();
  const auto unaligned = make_array();

  // Dim::Temperature is a dim of both the wrapper and the unaligned content.
  Slice s(Dim::Temperature, 0);
  EXPECT_EQ(realigned.slice(s).unaligned(),
            unaligned.slice({Dim::Position, 0, 3}).slice(s));
}

TEST_F(RealignTest, unaligned_of_slice_along_realigned_dim) {
  const auto realigned = make_realigned();
  const auto unaligned = make_array();

  // Dim::X is a dim of the wrapper but not the unaligned content. For now
  // slicing the wrapper returns a view on the full unaligned content, *not*
  // filtering any "events".
  Slice s(Dim::X, 0);
  EXPECT_EQ(realigned.slice(s).unaligned(),
            unaligned.slice({Dim::Position, 0, 3}));
}

TEST_F(RealignTest, unaligned_slice_contains_sliced_coords) {
  // This is implied by test `unaligned_of_slice_along_realigned_dim` but
  // demonstrates more explicitly how coordinates (and dimensions) are
  // preserved.
  const auto realigned = make_realigned();
  const auto slice = realigned.slice({Dim::X, 0});
  EXPECT_FALSE(slice.coords().contains(Dim::X));
  // Slicing realigned dimensions does not eagerly slice the unaligned content.
  // Therefore, corresponding coordinates are not removed, even for a non-range
  // slice.
  EXPECT_TRUE(slice.unaligned().coords().contains(Dim::X));
}

TEST_F(RealignTest, slice_unaligned_view) {
  const auto realigned = make_realigned();
  const auto a = make_array();

  Slice s(Dim::Temperature, 0);
  EXPECT_EQ(realigned.unaligned().slice(s),
            a.slice({Dim::Position, 0, 3}).slice(s));
}

TEST_F(RealignTest, histogram) {
  const auto realigned = make_realigned();
  EXPECT_EQ(histogram(realigned), make_aligned());
}

TEST_F(RealignTest, histogram_transposed) {
  auto base = make_array();
  DataArray transposed(Variable(base.data().transpose()), base.coords());
  auto realigned = unaligned::realign(
      transposed, {{Dim::Z, zbins}, {Dim::Y, ybins}, {Dim::X, xbins}});
  EXPECT_NO_THROW(histogram(realigned));
}

TEST_F(RealignTest, histogram_slice) {
  const auto realigned = make_realigned();
  const auto aligned = make_aligned();
  for (const auto dim : {Dim::Temperature, Dim::X, Dim::Y, Dim::Z}) {
    for (const auto s : {Slice(dim, 0), Slice(dim, 1), Slice(dim, 0, 1),
                         Slice(dim, 0, 2), Slice(dim, 1, 2)}) {
      const auto slice = realigned.slice(s);
      EXPECT_EQ(histogram(slice), aligned.slice(s)) << to_string(s);
    }
  }
}

struct RealignEventsTest : public ::testing::Test,
                           public ::testing::WithParamInterface<std::string> {
protected:
  bool scalar_weights() const { return GetParam() == "scalar_weights"; }

  Variable pos = makeVariable<Eigen::Vector3d>(
      Dims{Dim::Position}, Shape{4},
      Values{Eigen::Vector3d{1, 1, 1}, Eigen::Vector3d{1, 1, 2},
             Eigen::Vector3d{1, 2, 3}, Eigen::Vector3d{1, 2, 4}});
  Variable tof_bins =
      makeVariable<double>(Dims{Dim::Tof}, Shape{3}, Values{0, 2, 5});
  Variable pulse_time_bins = makeVariable<int64_t>(
      Dims{Dim::PulseTime}, Shape{3}, Values{100, 200, 300});

  DataArray make_array() {
    const auto tof = makeVariable<event_list<double>>(
        Dims{Dim::Position}, Shape{4},
        Values{event_list<double>{1}, event_list<double>{1, 2},
               event_list<double>{1, 2, 3}, event_list<double>{1, 2, 3, 4}});
    const auto pulse_time = makeVariable<event_list<int64_t>>(
        Dims{Dim::Position}, Shape{4},
        Values{event_list<int64_t>{100}, event_list<int64_t>{100, 200},
               event_list<int64_t>{100, 200, 200},
               event_list<int64_t>{100, 100, 200, 200}});
    return DataArray(
        scalar_weights()
            ? makeVariable<double>(Dims{Dim::Position}, Shape{4}, units::counts,
                                   Values{1, 1, 1, 1}, Variances{1, 1, 1, 1})
            : makeVariable<event_list<double>>(
                  Dims{Dim::Position}, Shape{4}, units::counts,
                  Values{event_list<double>{1}, event_list<double>{1, 1},
                         event_list<double>{1, 1, 1},
                         event_list<double>{1, 1, 1, 1}},
                  Variances{event_list<double>{1}, event_list<double>{1, 1},
                            event_list<double>{1, 1, 1},
                            event_list<double>{1, 1, 1, 1}}),
        {{Dim::Position, pos}, {Dim::Tof, tof}, {Dim::PulseTime, pulse_time}});
  }

  DataArray make_realigned() {
    return unaligned::realign(make_array(), {{Dim::Tof, tof_bins}});
  }

  DataArray make_aligned() {
    return DataArray(makeVariable<double>(Dims{Dim::Position, Dim::Tof},
                                          Shape{4, 2}, units::counts,
                                          Values{1, 0, 1, 1, 1, 2, 1, 3},
                                          Variances{1, 0, 1, 1, 1, 2, 1, 3}),
                     {{Dim::Position, pos}, {Dim::Tof, tof_bins}});
  }

  DataArray make_realigned_2d() {
    return unaligned::realign(make_array(), {{Dim::PulseTime, pulse_time_bins},
                                             {Dim::Tof, tof_bins}});
  }

  DataArray make_aligned_2d() {
    return DataArray(
        makeVariable<double>(
            Dims{Dim::Position, Dim::PulseTime, Dim::Tof}, Shape{4, 2, 2},
            units::counts,
            Values{1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 2, 1, 1, 0, 2},
            Variances{1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 2, 1, 1, 0, 2}),
        {{Dim::Position, pos},
         {Dim::PulseTime, pulse_time_bins},
         {Dim::Tof, tof_bins}});
  }
};

INSTANTIATE_TEST_SUITE_P(WeightType, RealignEventsTest,
                         testing::Values("scalar_weights",
                                         "event_list_weights"));

TEST_P(RealignEventsTest, basics) {
  const auto reference = make_aligned();
  auto base = make_array();
  auto realigned = unaligned::realign(base, {{Dim::Tof, tof_bins}});

  EXPECT_FALSE(realigned.hasData());
  EXPECT_EQ(realigned.dims(), reference.dims());
  EXPECT_EQ(realigned.coords(), reference.coords());
  EXPECT_EQ(realigned.unit(), base.unit());
  EXPECT_EQ(realigned.dtype(), reference.dtype());

  EXPECT_EQ(realigned.unaligned(), base);
}

TEST_P(RealignEventsTest, realigned_drop_alignment) {
  auto a = make_realigned();
  a.drop_alignment();
  EXPECT_EQ(a, make_array());
}

TEST_P(RealignEventsTest, dimension_order) {
  auto base = make_array();
  auto realigned1 = unaligned::realign(
      base, {{Dim::PulseTime, pulse_time_bins}, {Dim::Tof, tof_bins}});
  auto realigned2 = unaligned::realign(
      base, {{Dim::Tof, tof_bins}, {Dim::PulseTime, pulse_time_bins}});

  // Dimensions derived form realigned events are always the inner dimensions
  EXPECT_EQ(realigned1.dims(),
            Dimensions({Dim::Position, Dim::PulseTime, Dim::Tof}, {4, 2, 2}));
  EXPECT_EQ(realigned2.dims(),
            Dimensions({Dim::Position, Dim::Tof, Dim::PulseTime}, {4, 2, 2}));
}

TEST_P(RealignEventsTest, copy_realigned_slice) {
  const auto realigned = make_realigned();
  const auto slice = realigned.slice({Dim::Tof, 1});
  // `slice` contains unfiltered unaligned content, but copy drops out-of-bounds
  // content.
  const DataArray copy(slice);
  EXPECT_NE(copy, slice);
  EXPECT_EQ(copy.dims(), slice.dims());
  EXPECT_EQ(copy.coords(), slice.coords());
  EXPECT_EQ(copy.masks(), slice.masks());
  EXPECT_EQ(copy.attrs(), slice.attrs());
  EXPECT_NE(copy.unaligned(), slice.unaligned());
  EXPECT_EQ(copy.unaligned(), event::filter(realigned.unaligned(), Dim::Tof,
                                            realigned.coords()[Dim::Tof].slice(
                                                {Dim::Tof, 1, 3})));
}

TEST_P(RealignEventsTest, histogram) {
  EXPECT_EQ(histogram(make_realigned()), make_aligned());
}

TEST_P(RealignEventsTest, histogram_slices_of_2d) {
  // Full 2d histogram not supported yet, but we can do it slice-by-slice
  auto realigned = make_realigned_2d();
  const auto expected = make_aligned_2d();
  EXPECT_EQ(histogram(realigned.slice({Dim::PulseTime, 0})),
            expected.slice({Dim::PulseTime, 0}));
  EXPECT_EQ(histogram(realigned.slice({Dim::PulseTime, 1})),
            expected.slice({Dim::PulseTime, 1}));
}

TEST_P(RealignEventsTest, dtype) {
  auto realigned = make_realigned();
  EXPECT_EQ(realigned.unaligned().dtype(),
            scalar_weights() ? dtype<double> : dtype<event_list<double>>);
  EXPECT_EQ(realigned.dtype(), dtype<double>);
}
