#ifndef TEST_H_
#define TEST_H_

#include <stdio.h>
#include <setjmp.h>

typedef enum TestResult
{
    RESULT_PASS,
    RESULT_FAIL,
    RESULT_SKIP,

    RESULT__LEN
} TestResult;

typedef struct TestState
{
    TestResult result;
    jmp_buf jbuf;
} TestState;

#define H_LIST(...)     __VA_ARGS__

#define ASSERT_MSG(E,M)   do { if(!(E)) { fprintf(stderr, "FAIL %s: %s\n", NAME_, M); STATE_->result = RESULT_FAIL; longjmp(STATE_->jbuf, 1); } } while(0)

#define ASSERT_EXPR(E)  ASSERT_MSG(E,#E)

#define ASSERT_TRUE(X)  ASSERT_MSG(X, #X)
#define ASSERT_FALSE(X) ASSERT_MSG(!(X), "!" #X)
#define ASSERT_EQ(X,Y)  ASSERT_MSG((X) == (Y), #X " == " #Y)
#define ASSERT_NE(X,Y)  ASSERT_MSG((X) != (Y), #X " != " #Y)
#define ASSERT_LE(X,Y)  ASSERT_MSG((X) <= (Y), #X " <= " #Y)
#define ASSERT_LT(X,Y)  ASSERT_MSG((X) < (Y), #X " < " #Y)
#define ASSERT_GE(X,Y)  ASSERT_MSG((X) >= (Y), #X " >= " #Y)
#define ASSERT_GT(X,Y)  ASSERT_MSG((X) > (Y), #X " > " #Y)
#define ASSERT_STREQ(X,Y)       ASSERT_MSG(!strcmp((X),(Y)), #X " <str==> " #Y)
#define ASSERT_STRNE(X,Y)       ASSERT_MSG(strcmp((X),(Y)), #X " <str!=> " #Y)
#define ASSERT_MEMEQ(X,LX,Y,LY) ASSERT_MSG((LX) == (LY) && !memcmp((X),(Y),(LX)), #X "[" #LX "] <mem==> " #Y "[" #LY "]")
#define ASSERT_MEMNE(X,LX,Y,LY) ASSERT_MSG((LX) != (LY) || memcmp((X),(Y),(LX)), #X "[" #LX "] <mem!=> " #Y "[" #LY "]")

#define ASSERT_NULL(X)      ASSERT_EQ(X,NULL)
#define ASSERT_NOTNULL(X)   ASSERT_NE(X,NULL)

#define TEST(NAME,BODY)                                                        \
void TEST_##NAME(TestState* STATE_) { const char* NAME_ = #NAME; { H_LIST BODY ; } fprintf(stderr, "PASS %s\n", NAME_); }

#define SKIP()      do { fprintf(stderr, "SKIP %s\n", NAME_); STATE_->result = RESULT_SKIP; longjmp(STATE_->jbuf, 1); } while(0)
#define SKIP_MSG(M) do { fprintf(stderr, "SKIP %s: %s\n", NAME_, M); STATE_->result = RESULT_SKIP; longjmp(STATE_->jbuf, 1); } while(0)

#define TESTS_BEGIN()   { unsigned int COUNTS_[RESULT__LEN] = {}
#define TEST_HEADER(STR)    fprintf(stderr, "==== %s ====\n", STR);
#define TEST_EXEC(NAME) do { TestState STATE_; STATE_.result = RESULT_PASS; if(!setjmp(STATE_.jbuf)) TEST_##NAME(&STATE_); COUNTS_[STATE_.result]++; } while(0)
#define TEST_SKIP(NAME) do { fprintf(stderr, "SKIP %s\n", #NAME); COUNTS_[RESULT_SKIP]++; } while(0)
#define TEST_SKIP_MSG(NAME,M)   do { fprintf(stderr, "SKIP %s: %s\n", #NAME, M); COUNTS_[RESULT_SKIP]++; } while(0)
#define TESTS_END() {                                                          \
    unsigned int TOTAL_ = COUNTS_[RESULT_PASS] + COUNTS_[RESULT_FAIL] + COUNTS_[RESULT_SKIP];\
    int TOTAL_LOG10_ = 0;                                                      \
    unsigned int TMP_;                                                         \
    for(TMP_ = TOTAL_; TMP_; TMP_ /= 10) TOTAL_LOG10_++;                       \
    if(TOTAL_LOG10_ < 1) TOTAL_LOG10_ = 1;                                     \
    fprintf(stderr, "----------\n");                                           \
    fprintf(stderr, "Total: %*u\n", TOTAL_LOG10_, TOTAL_);                     \
    fprintf(stderr, "PASS:  %*u (%3.0f%%)\n", TOTAL_LOG10_, COUNTS_[RESULT_PASS], COUNTS_[RESULT_PASS] / (double)TOTAL_ * 100);\
    fprintf(stderr, "FAIL:  %*u (%3.0f%%)\n", TOTAL_LOG10_, COUNTS_[RESULT_FAIL], COUNTS_[RESULT_FAIL] / (double)TOTAL_ * 100);\
    fprintf(stderr, "SKIP:  %*u (%3.0f%%)\n", TOTAL_LOG10_, COUNTS_[RESULT_SKIP], COUNTS_[RESULT_SKIP] / (double)TOTAL_ * 100);\
} }

#endif /* TEST_H_ */
