// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

#include <gtest/gtest.h>

#include "scipp/core/dict.h"
#include "scipp/units/dim.h"

#include "test_macros.h"

using namespace scipp::core;
using scipp::Dim;

using DimDict = Dict<Dim, int>;

TEST(Dict, default_constructor_creates_empty) {
  DimDict dict;
  EXPECT_TRUE(dict.empty());
  EXPECT_EQ(dict.size(), 0);
  EXPECT_EQ(dict.capacity(), 0);
}

TEST(Dict, reserve_increases_capacity) {
  DimDict dict;
  dict.reserve(4);
  EXPECT_EQ(dict.capacity(), 4);
}

TEST(Dict, can_insert_and_get_element) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 78461);
  EXPECT_TRUE(dict.contains(Dim::Time));
}

TEST(Dict, key_that_was_not_inserted_does_not_exist) {
  DimDict dict;
  EXPECT_FALSE(dict.contains(Dim::X));
  dict.insert_or_assign(Dim::Event, 5612095);
  EXPECT_FALSE(dict.contains(Dim::X));
}

TEST(Dict, can_get_inserted_element) {
  DimDict dict;
  dict.insert_or_assign(Dim::Group, 561902);
  EXPECT_EQ(dict[Dim::Group], 561902);
  const DimDict const_dict(dict);
  EXPECT_EQ(const_dict[Dim::Group], 561902);
}

TEST(Dict, can_modify_existing_element) {
  DimDict dict;
  dict.insert_or_assign(Dim::X, 561902);
  dict[Dim::X] = -7491;
  EXPECT_EQ(dict[Dim::X], -7491);
}

TEST(Dict, access_operator_throws_if_key_does_not_exist) {
  DimDict dict;
  EXPECT_THROW_DISCARD(dict[Dim::Y], scipp::except::NotFoundError);
}

TEST(Dict, erase_empty_dict_throws) {
  DimDict dict;
  EXPECT_THROW(dict.erase(Dim::Event), scipp::except::NotFoundError);
}

TEST(Dict, erase_invalid_key_throws) {
  DimDict dict;
  dict.insert_or_assign(Dim::Position, 761490);
  EXPECT_THROW(dict.erase(Dim::X), scipp::except::NotFoundError);
}

TEST(Dict, item_is_not_accessible_after_erase_front) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 6148);
  dict.insert_or_assign(Dim::Y, -471);
  dict.insert_or_assign(Dim::Event, 4761);
  dict.erase(Dim::Time);
  EXPECT_FALSE(dict.contains(Dim::Time));
  EXPECT_THROW_DISCARD(dict[Dim::Time], scipp::except::NotFoundError);
}

TEST(Dict, item_is_not_accessible_after_erase_middle) {
  DimDict dict;
  dict.insert_or_assign(Dim::X, 817);
  dict.insert_or_assign(Dim::Row, -9982);
  dict.insert_or_assign(Dim::Time, 7176);
  dict.erase(Dim::Row);
  EXPECT_FALSE(dict.contains(Dim::Row));
  EXPECT_THROW_DISCARD(dict[Dim::Row], scipp::except::NotFoundError);
}

TEST(Dict, item_is_not_accessible_after_erase_back) {
  DimDict dict;
  dict.insert_or_assign(Dim::Event, -773616);
  dict.insert_or_assign(Dim::Position, 41);
  dict.insert_or_assign(Dim::Group, -311);
  dict.erase(Dim::Group);
  EXPECT_FALSE(dict.contains(Dim::Group));
  EXPECT_THROW_DISCARD(dict[Dim::Group], scipp::except::NotFoundError);
}

TEST(Dict, item_is_not_accessible_after_erase_multiple) {
  DimDict dict;
  dict.insert_or_assign(Dim::Z, -2);
  dict.insert_or_assign(Dim::Time, 16);
  dict.insert_or_assign(Dim::Energy, 41);
  dict.erase(Dim::Time);
  dict.erase(Dim::Z);
  EXPECT_FALSE(dict.contains(Dim::Time));
  EXPECT_FALSE(dict.contains(Dim::Z));
  EXPECT_THROW_DISCARD(dict[Dim::Time], scipp::except::NotFoundError);
  EXPECT_THROW_DISCARD(dict[Dim::Z], scipp::except::NotFoundError);
}

TEST(Dict, key_iterator_does_not_produce_erased_element) {
  DimDict dict;
  dict.insert_or_assign(Dim::Energy, 111);
  dict.insert_or_assign(Dim::Z, -2623);
  dict.insert_or_assign(Dim::Row, 61);
  dict.erase(Dim::Energy);
  auto it = dict.keys_begin();
  EXPECT_EQ(*it, Dim::Z);
  EXPECT_EQ(*(++it), Dim::Row);
  EXPECT_EQ(++it, dict.keys_end());
}

TEST(Dict, erasing_all_elements_yieldds_empty_dict) {
  DimDict dict;
  dict.insert_or_assign(Dim::Y, -5151);
  dict.insert_or_assign(Dim::Time, -2);
  dict.insert_or_assign(Dim::Event, 991);
  dict.erase(Dim::Time);
  dict.erase(Dim::Event);
  dict.erase(Dim::Y);
  EXPECT_TRUE(dict.empty());
}

TEST(Dict, extract_throws_if_element_does_not_exist) {
  DimDict dict;
  dict.insert_or_assign(Dim::Row, 999);
  dict.insert_or_assign(Dim::X, 888);
  dict.insert_or_assign(Dim::Time, 777);
  EXPECT_THROW_DISCARD(dict.extract(Dim::Y), scipp::except::NotFoundError);
}

TEST(Dict, extract_returns_element) {
  DimDict dict;
  dict.insert_or_assign(Dim::X, 999);
  dict.insert_or_assign(Dim::Y, 888);
  dict.insert_or_assign(Dim::Z, 777);
  EXPECT_EQ(dict.extract(Dim::Y), 888);
}

TEST(Dict, extract_erases_element) {
  DimDict dict;
  dict.insert_or_assign(Dim::Row, 666);
  dict.insert_or_assign(Dim::Time, 555);
  dict.insert_or_assign(Dim::Energy, 444);
  static_cast<void>(dict.extract(Dim::Time));
  EXPECT_FALSE(dict.contains(Dim::Time));
}

TEST(Dict, key_iterator_of_empty_dict_is_end) {
  DimDict dict;
  EXPECT_EQ(dict.keys_begin(), dict.keys_end());
}

TEST(Dict, key_iterator_produces_correct_keys) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 61892);
  dict.insert_or_assign(Dim::Event, 619);
  auto it = dict.keys_begin();
  EXPECT_EQ(*it, Dim::Time);
  ++it;
  EXPECT_EQ(*it, Dim::Event);
  ++it;
  EXPECT_EQ(it, dict.keys_end());
}

TEST(Dict, key_iterator_can_access_key_via_arrow) {
  Dict<std::string, int> dict;
  dict.insert_or_assign("gak", 7419);
  dict.insert_or_assign("9ana", -919);
  auto it = dict.keys_begin();
  EXPECT_EQ(it->size(), 3);
  EXPECT_EQ((++it)->size(), 4);
}

TEST(Dict, key_iterator_cannot_change_keys) {
  DimDict dict;
  EXPECT_TRUE(
      std::is_const_v<std::remove_reference_t<decltype(*dict.keys_begin())>>);
}

TEST(Dict, key_iterator_throws_if_capacity_changed) {
  DimDict dict;
  dict.reserve(1);
  dict.insert_or_assign(Dim::X, 719);
  auto it = dict.keys_begin();
  dict.reserve(16);
  EXPECT_THROW_DISCARD(*it, std::runtime_error);
  EXPECT_THROW_DISCARD(++it, std::runtime_error);
}

TEST(Dict, key_iterator_throws_if_element_inserted_with_realloc) {
  DimDict dict;
  dict.reserve(1); // Make sure the 2nd insert reallocates.
  dict.insert_or_assign(Dim::X, 719);
  auto it = dict.keys_begin();
  dict.insert_or_assign(Dim::Y, 13);
  EXPECT_THROW_DISCARD(*it, std::runtime_error);
  EXPECT_THROW_DISCARD(++it, std::runtime_error);
}

TEST(Dict, key_iterator_throws_if_element_inserted_in_same_memory) {
  DimDict dict;
  dict.reserve(4); // Make sure the 2nd insert does not reallocate.
  dict.insert_or_assign(Dim::X, 719);
  auto it = dict.keys_begin();
  dict.insert_or_assign(Dim::Y, 13);
  EXPECT_THROW_DISCARD(*it, std::runtime_error);
  EXPECT_THROW_DISCARD(++it, std::runtime_error);
}

TEST(Dict, key_iterator_throws_if_element_erased) {
  DimDict dict;
  dict.insert_or_assign(Dim::Y, -4122);
  dict.insert_or_assign(Dim::Row, 5619);
  auto it = dict.keys_begin();
  dict.erase(Dim::Row);
  EXPECT_THROW_DISCARD(*it, std::runtime_error);
  EXPECT_THROW_DISCARD(++it, std::runtime_error);
}

TEST(Dict, value_iterator_produces_correct_values) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 61892);
  dict.insert_or_assign(Dim::Event, 619);
  auto it = dict.values_begin();
  EXPECT_EQ(*it, 61892);
  ++it;
  EXPECT_EQ(*it, 619);
  ++it;
  EXPECT_EQ(it, dict.values_end());
}

TEST(Dict, const_value_iterator_produces_correct_values) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 4561);
  dict.insert_or_assign(Dim::Event, 76);
  const DimDict const_dict(dict);
  auto it = const_dict.values_begin();
  EXPECT_EQ(*it, 4561);
  ++it;
  EXPECT_EQ(*it, 76);
  ++it;
  EXPECT_EQ(it, const_dict.values_end());
}

TEST(Dict, value_iterator_can_change_values) {
  DimDict dict;
  dict.insert_or_assign(Dim::Y, -816);
  dict.insert_or_assign(Dim::Z, -41);
  auto it = dict.values_begin();
  *it = 923;
  *(++it) = -5289;
  EXPECT_EQ(dict[Dim::Y], 923);
  EXPECT_EQ(dict[Dim::Z], -5289);
}

TEST(Dict, iterator_of_empty_dict_is_end) {
  DimDict dict;
  EXPECT_EQ(dict.begin(), dict.end());
}

TEST(Dict, iterator_produces_correct_keys_and_values) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 61892);
  dict.insert_or_assign(Dim::Event, 619);
  auto it = dict.begin();
  EXPECT_EQ((*it).first, Dim::Time);
  EXPECT_EQ((*it).second, 61892);
  ++it;
  EXPECT_EQ((*it).first, Dim::Event);
  EXPECT_EQ((*it).second, 619);
  ++it;
  EXPECT_EQ(it, dict.end());
}

TEST(Dict, iterator_produces_correct_keys_and_values_via_arrow) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 61892);
  dict.insert_or_assign(Dim::Event, 619);
  auto it = dict.begin();
  EXPECT_EQ(it->first, Dim::Time);
  EXPECT_EQ(it->second, 61892);
  ++it;
  EXPECT_EQ(it->first, Dim::Event);
  EXPECT_EQ(it->second, 619);
  ++it;
  EXPECT_EQ(it, dict.end());
}

TEST(Dict, iterator_can_change_values) {
  DimDict dict;
  dict.insert_or_assign(Dim::Position, -51);
  dict.insert_or_assign(Dim::Row, 827);
  auto it = dict.begin();
  (*it).second = 991;
  (*(++it)).second = -9761;
  EXPECT_EQ(dict[Dim::Position], 991);
  EXPECT_EQ(dict[Dim::Row], -9761);
}

TEST(Dict, iterator_can_change_values_via_arrow) {
  DimDict dict;
  dict.insert_or_assign(Dim::Position, -51);
  dict.insert_or_assign(Dim::Row, 827);
  auto it = dict.begin();
  it->second = 991;
  (++it)->second = -9761;
  EXPECT_EQ(dict[Dim::Position], 991);
  EXPECT_EQ(dict[Dim::Row], -9761);
}

TEST(Dict, iterator_cannot_change_keys) {
  DimDict dict;
  EXPECT_TRUE(std::is_const_v<
              std::remove_reference_t<decltype(*dict.begin())::first_type>>);
}

TEST(Dict, find) {
  DimDict dict;
  dict.insert_or_assign(Dim::X, 7901);
  dict.insert_or_assign(Dim::Y, 515);
  EXPECT_EQ(dict.find(Dim::Y), ++dict.begin());
  EXPECT_EQ(dict.find(Dim::X), dict.begin());
  EXPECT_EQ(dict.find(Dim::Z), dict.end());
}

TEST(Dict, insertion_order_is_Preserved) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 168);
  dict.insert_or_assign(Dim::Y, 144);
  dict.insert_or_assign(Dim::Z, 31);
  dict.erase(Dim::Time);
  dict.insert_or_assign(Dim::Time, -182);
  dict.insert_or_assign(Dim::Row, 25);
  dict.insert_or_assign(Dim::X, -22);
  dict.erase(Dim::X);
  dict.erase(Dim::Row);
  dict.insert_or_assign(Dim::Energy, 3441);
  dict.insert_or_assign(Dim::Event, 123);
  dict.erase(Dim::Z);

  std::vector<std::pair<Dim, int>> result;
  std::transform(dict.begin(), dict.end(), std::back_inserter(result),
                 [](const auto &p) {
                   return std::pair{p.first, p.second};
                 });

  std::vector<std::pair<Dim, int>> expected{
      {Dim::Y, 144}, {Dim::Time, -182}, {Dim::Energy, 3441}, {Dim::Event, 123}};
  EXPECT_EQ(result, expected);
}

TEST(Dict, iterator_with_transform) {
  DimDict dict;
  dict.insert_or_assign(Dim::X, 7476);
  dict.insert_or_assign(Dim::Event, -31);
  dict.insert_or_assign(Dim::Position, 0);

  auto it = dict.begin_transform([](const auto &x) {
    return std::pair{x.first, 2 * x.second};
  });
  EXPECT_EQ(it->first, Dim::X);
  EXPECT_EQ(it->second, 2 * 7476);
  ++it;
  EXPECT_EQ(it->first, Dim::Event);
  EXPECT_EQ(it->second, -2 * 31);
  ++it;
  EXPECT_EQ(it->first, Dim::Position);
  EXPECT_EQ(it->second, 0);
  ++it;
  EXPECT_EQ(it, dict.end());
}
