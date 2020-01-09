//
// Created by igudich on 1/9/20.
//

#ifndef SCIPP_FIX_TYPED_TEST_SUITE_WARNINGS_H
#define SCIPP_FIX_TYPED_TEST_SUITE_WARNINGS_H

#undef TYPED_TEST_SUITE
#define TYPED_TEST_SUITE(CaseName, Types)                                      \
  typedef ::testing::internal::GenerateTypeList<Types>::type                   \
      GTEST_TYPE_PARAMS_(CaseName);                                            \
  typedef ::testing::internal::NameGeneratorSelector<>::type                   \
      GTEST_NAME_GENERATOR_(CaseName)
#endif // SCIPP_FIX_TYPED_TEST_SUITE_WARNINGS_H
