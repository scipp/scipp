#define EXPECT_THROW_MSG(TRY_BLOCK, EXCEPTION_TYPE, MESSAGE)                   \
  EXPECT_THROW({                                                               \
    try {                                                                      \
      TRY_BLOCK;                                                               \
    } catch (const EXCEPTION_TYPE &e) {                                        \
      EXPECT_STREQ(MESSAGE, e.what());                                         \
      throw;                                                                   \
    }                                                                          \
  }, EXCEPTION_TYPE);
