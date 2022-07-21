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

TEST(Dict, const_key_iterator_produces_correct_keys) {
  DimDict dict;
  dict.insert_or_assign(Dim::Time, 61892);
  dict.insert_or_assign(Dim::Event, 619);
  const DimDict const_dict(dict);
  auto it = const_dict.keys_begin();
  EXPECT_EQ(*it, Dim::Time);
  ++it;
  EXPECT_EQ(*it, Dim::Event);
  ++it;
  EXPECT_EQ(it, const_dict.keys_end());
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
