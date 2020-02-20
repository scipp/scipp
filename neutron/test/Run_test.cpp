// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset.h"
#include "value_with_delta.h"

using namespace scipp::core;

Dataset makeRun() {
  Dataset run;
  run.insert(Data::Value, "total_counts", {}, {1000});
  run.insert(Coord::Polarization, {}, {"Spin-Up"});
  run.insert(Coord::FuzzyTemperature, {}, {ValueWithDelta<double>(4.2, 0.1)});
  Dataset comment;
  comment.insert(Data::DeprecatedString, "",
                 makeVariable<std::string>({Dim::Row, 1}, {"first run"}));
  run.insert<Dataset>(Data::Value, "comment", {}, {comment});
  Dataset timeSeriesLog;
  timeSeriesLog.insert(Coord::Time, {Dim::Time, 3}, {0, 1000, 1500});
  timeSeriesLog.insert(Data::Value, "pressure1", {Dim::Time, 3},
                       {1013, 900, 800});
  timeSeriesLog.insert(Data::Value, "pressure2", {Dim::Time, 3}, {100, 90, 80});
  run.insert<Dataset>(Data::Value, "pressure_log", {}, {timeSeriesLog});
  Dataset otherLogEntries;
  otherLogEntries.insert<Dataset>(Data::Value, "root", {Dim::Row, 1});
  run.insert<Dataset>(Data::Value, "generic_log", {Dim::Row, 1},
                      {otherLogEntries});
  return run;
}

// A number of tests that is currently disabled. It is not clear how we will
// support attribute propagation in the future, in particular nested datasets
// may not be concatenated automatically.
TEST(Run, DISABLED_meta_data_propagation) {
  Dataset run1 = makeRun();

  Dataset d1;
  d1.insert(Attr::ExperimentLog, "sample_log", {}, {run1});

  EXPECT_NO_THROW(d1 += d1);

  auto run2(run1);
  run2.get(Data::Value, "total_counts")[0] = 1111;
  run2.get(Coord::FuzzyTemperature)[0] = ValueWithDelta<double>(4.15, 0.1);
  run2.span<Dataset>(Data::Value, "comment")[0].get(Data::DeprecatedString)[0] =
      "second run";
  run2.span<Dataset>(Data::Value, "generic_log")[0]
      .span<Dataset>(Data::Value, "root")[0]
      .insert(
          Data::DeprecatedString, "user comment",
          makeVariable<std::string>(
              {},
              {"Spider walked through beam, verify data before publishing."}));

  Dataset d2;
  d2.insert(Attr::ExperimentLog, "sample_log", {}, {run2});

  // Behavior of `Attr` variables is specific to the implementation of each
  // operation. In most cases we simply copy the first one, exceptions are
  // `operator+` and `concatenate`, where we merge the attributes:
  ASSERT_NO_THROW(d1 += d2);

  // Get the combined experiment logs, which are a Dataset. For out-of-the-box
  // handling of various types of log entries, different "types" of log entries
  // are in different places in the internal dataset structure. For more
  // convenient access we should provide a view class that can be instantiated
  // on the fly.
  const auto &run = d1.get(Attr::ExperimentLog, "sample_log")[0];

  // Example of a log entry that is accumulated:
  EXPECT_EQ(run.get(Data::Value, "total_counts").size(), 1);
  EXPECT_EQ(run.get(Data::Value, "total_counts")[0], 2111);

  // Example of a log entry that is verified:
  EXPECT_EQ(run.get(Coord::Polarization).size(), 1);
  EXPECT_EQ(run.get(Coord::Polarization)[0], "Spin-Up");

  // Example of a log entry that is verified with fuzzy matching:
  EXPECT_EQ(run.get(Coord::FuzzyTemperature).size(), 1);
  // Note: No averaging happening here, it is simply checked to be in range.
  EXPECT_EQ(run.get(Coord::FuzzyTemperature)[0],
            ValueWithDelta<double>(4.2, 0.1));

  // Example of a log entry that is concatenated:
  const auto comments =
      run.span<Dataset>(Data::Value, "comment")[0].get(Data::DeprecatedString);
  EXPECT_EQ(comments.size(), 2);
  EXPECT_EQ(comments[0], "first run");
  EXPECT_EQ(comments[1], "second run");

  // Example of a "time series" log entry that is concatenated:
  const auto pressure_log = run.span<Dataset>(Data::Value, "pressure_log")[0];
  EXPECT_EQ(pressure_log.dimensions().count(), 1);
  EXPECT_EQ(pressure_log.dimensions().label(0), Dim::Time);
  EXPECT_EQ(pressure_log.dimensions().size(0), 6);
  // No hidden magic here, it is simply concatenated, can do smarter processing
  // by hand afterwards.
  EXPECT_EQ(pressure_log.get(Data::Value, "pressure1"),
            scipp::span(std::vector<double>({1013, 900, 800, 1013, 900, 800})));

  // Example of an optional log entry, i.e., one that is not present in all
  // operands:
  const auto generic_log = run.span<Dataset>(Data::Value, "generic_log")[0];
  EXPECT_EQ(generic_log.dimensions().count(), 1);
  EXPECT_EQ(generic_log.dimensions().label(0), Dim::Row);
  EXPECT_EQ(generic_log.dimensions().size(0), 2);
  const auto generic_log_run1 =
      generic_log.span<Dataset>(Data::Value, "root")[0];
  // No entries from run 1.
  EXPECT_EQ(generic_log_run1.size(), 0);
  const auto generic_log_run2 =
      generic_log.span<Dataset>(Data::Value, "root")[1];
  // 1 entry from run 2.
  EXPECT_EQ(generic_log_run2.size(), 1);
  EXPECT_NO_THROW(generic_log_run2(Data::DeprecatedString, "user comment"));
  // Again there was no automatic merging, can be done by hand if required.
}

TEST(Run, DISABLED_meta_data_fail_coord_mismatch) {
  Dataset d1;
  d1.insert(Attr::ExperimentLog, "sample_log", {}, {makeRun()});
  Dataset d2(d1);

  auto &run2 = d2.get(Attr::ExperimentLog, "sample_log")[0];
  run2.get(Coord::Polarization)[0] = "Spin-Down";
  EXPECT_THROW_MSG_SUBSTR(d1 += d2, except::VariableMismatchError,
                          "expected to match");
}

TEST(Run, DISABLED_meta_data_fail_fuzzy_coord_mismatch) {
  Dataset d1;
  d1.insert(Attr::ExperimentLog, "sample_log", {}, {makeRun()});
  Dataset d2(d1);

  auto &run2 = d2.get(Attr::ExperimentLog, "sample_log")[0];
  run2.get(Coord::FuzzyTemperature)[0] = ValueWithDelta<double>(4.0, 0.1);
  EXPECT_THROW_MSG_SUBSTR(d1 += d2, except::VariableMismatchError,
                          "expected to match");
}

TEST(Run, DISABLED_meta_data_fail_missing) {
  Dataset d1;
  d1.insert(Attr::ExperimentLog, "sample_log", {}, {makeRun()});
  Dataset d2(d1);

  auto &run2 = d2.get(Attr::ExperimentLog, "sample_log")[0];
  run2.span<Dataset>(Data::Value, "comment")[0].erase(Data::DeprecatedString);
  EXPECT_THROW_MSG(d1 += d2, std::runtime_error,
                   "Cannot add Variable: Nested Dataset dimension must be 1.");
}
