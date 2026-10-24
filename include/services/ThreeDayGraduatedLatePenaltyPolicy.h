
#ifndef THREE_DAY_GRADUATED_LATE_PENALTY_POLICY_H
#define THREE_DAY_GRADUATED_LATE_PENALTY_POLICY_H

#include "LatePenaltyPolicy.h"

class ThreeDayGraduatedLatePenaltyPolicy
    : public LatePenaltyPolicy {
    public:
        double calculateFinalMark(
            double awardedMark,
            double maximumMark,
            int daysLate) const override;
};

#endif
