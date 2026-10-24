
#include "services/ThreeDayGraduatedLatePenaltyPolicy.h"

#include <stdexcept>

double ThreeDayGraduatedLatePenaltyPolicy::calculateFinalMark(
    double awardedMark,
    double maximumMark,
    int daysLate) const {

    if (maximumMark <= 0.0) {
        throw std::invalid_argument(
            "Maximum mark must be greater than zero.");
    }

    if (awardedMark < 0.0 || awardedMark > maximumMark) {
        throw std::invalid_argument(
            "Awarded mark must be within the valid range.");
    }

    if (daysLate < 0) {
        throw std::invalid_argument(
            "Days late cannot be negative.");
    }

    if (daysLate == 0) {
        return awardedMark;
    }

    if (daysLate > 3) {
        return 0.0;
    }

    const double dailyPenalty = maximumMark * 0.10;
    const double totalPenalty = dailyPenalty * daysLate;

    const double finalMark = awardedMark - totalPenalty;

    return finalMark < 0.0 ? 0.0 : finalMark;
}
