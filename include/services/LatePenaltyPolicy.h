
#ifndef LATE_PENALTY_POLICY_H
#define LATE_PENALTY_POLICY_H

class LatePenaltyPolicy {
    public:
        virtual ~LatePenaltyPolicy() = default;

        virtual double calculateFinalMark(
            double awardedMark,
            double maximumMark,
            int daysLate) const = 0;
};

#endif
