
#include "services/ThreeDayGraduatedLatePenaltyPolicy.h"

#include <gtest/gtest.h>

#include <stdexcept>

class LatePenaltyPolicyTest : public ::testing::Test {
protected:
    ThreeDayGraduatedLatePenaltyPolicy policy;
};

// On-time submission: no penalty.
TEST_F(LatePenaltyPolicyTest, NoPenaltyForOnTimeSubmission) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 0),
        80.0
    );
}

// One day late: deduct 10% of the maximum mark.
TEST_F(LatePenaltyPolicyTest, AppliesTenPercentPenaltyAfterOneDay) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 1),
        70.0
    );
}

// Two days late: deduct 20% of the maximum mark.
TEST_F(LatePenaltyPolicyTest, AppliesTwentyPercentPenaltyAfterTwoDays) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 2),
        60.0
    );
}

// Three days late: deduct 30% of the maximum mark.
TEST_F(LatePenaltyPolicyTest, AppliesThirtyPercentPenaltyAfterThreeDays) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 3),
        50.0
    );
}

// More than three days late: final mark is zero.
TEST_F(LatePenaltyPolicyTest, GivesZeroAfterThreeDays) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(80, 100, 4),
        0.0
    );
}

// The final mark cannot become negative.
TEST_F(LatePenaltyPolicyTest, FinalMarkCannotBeNegative) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(15, 100, 2),
        0.0
    );
}

// Maximum mark must be greater than zero.
TEST_F(LatePenaltyPolicyTest, RejectsZeroMaximumMark) {
    EXPECT_THROW(
        policy.calculateFinalMark(50, 0, 1),
        std::invalid_argument
    );
}

// Awarded mark cannot be negative.
TEST_F(LatePenaltyPolicyTest, RejectsNegativeAwardedMark) {
    EXPECT_THROW(
        policy.calculateFinalMark(-5, 100, 0),
        std::invalid_argument
    );
}

// Awarded mark cannot exceed the maximum mark.
TEST_F(LatePenaltyPolicyTest, RejectsAwardedMarkAboveMaximum) {
    EXPECT_THROW(
        policy.calculateFinalMark(110, 100, 0),
        std::invalid_argument
    );
}

// Days late cannot be negative.
TEST_F(LatePenaltyPolicyTest, RejectsNegativeDaysLate) {
    EXPECT_THROW(
        policy.calculateFinalMark(80, 100, -1),
        std::invalid_argument
    );
}

// A mark of zero remains zero.
TEST_F(LatePenaltyPolicyTest, AcceptsZeroAwardedMark) {
    EXPECT_DOUBLE_EQ(
        policy.calculateFinalMark(0, 100, 0),
        0.0
    );
}
