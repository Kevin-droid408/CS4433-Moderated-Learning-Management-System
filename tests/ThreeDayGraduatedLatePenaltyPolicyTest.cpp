#include <gtest/gtest.h>

#include "services/ThreeDayGraduatedLatePenaltyPolicy.h"

#include <stdexcept>

class ThreeDayGraduatedLatePenaltyPolicyTest
    : public ::testing::Test {
protected:
    ThreeDayGraduatedLatePenaltyPolicy policy;
};

// On-time submission: no penalty.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    NoPenaltyWhenSubmittedOnTime
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 0),
        80
    );
}

// One day late: 10% penalty.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    ApplyTenPercentPenaltyAfterOneDay
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 1),
        70
    );
}

// Two days late: 20% penalty.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    ApplyTwentyPercentPenaltyAfterTwoDays
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 2),
        60
    );
}

// Three days late: 30% penalty.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    ApplyThirtyPercentPenaltyAfterThreeDays
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 3),
        50
    );
}

// More than three days late: zero mark.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    ReturnZeroAfterMoreThanThreeDays
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 4),
        0
    );
}

// Maximum mark must be positive.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    RejectZeroMaximumMark
) {
    EXPECT_THROW(
        policy.calculateFinalMark(50, 0, 0),
        std::invalid_argument
    );
}

TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    RejectNegativeMaximumMark
) {
    EXPECT_THROW(
        policy.calculateFinalMark(50, -100, 0),
        std::invalid_argument
    );
}

// Awarded mark must be within the permitted range.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    RejectNegativeAwardedMark
) {
    EXPECT_THROW(
        policy.calculateFinalMark(-1, 100, 0),
        std::invalid_argument
    );
}

TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    RejectAwardedMarkAboveMaximum
) {
    EXPECT_THROW(
        policy.calculateFinalMark(101, 100, 0),
        std::invalid_argument
    );
}

// Boundary values for awarded marks.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    AcceptZeroAwardedMark
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(0, 100, 0),
        0
    );
}

TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    AcceptAwardedMarkEqualToMaximum
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(100, 100, 0),
        100
    );
}

// Negative days late are invalid.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    RejectNegativeDaysLate
) {
    EXPECT_THROW(
        policy.calculateFinalMark(80, 100, -1),
        std::invalid_argument
    );
}

// The final mark cannot become negative.
TEST_F(
    ThreeDayGraduatedLatePenaltyPolicyTest,
    FinalMarkCannotBeNegative
) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(10, 100, 2),
        0
    );
}