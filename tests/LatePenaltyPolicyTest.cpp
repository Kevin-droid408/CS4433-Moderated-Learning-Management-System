#include "services/ThreeDayGraduatedLatePenaltyPolicy.h"

#include <cassert>
#include <cmath>

int main() {
    ThreeDayGraduatedLatePenaltyPolicy policy;

    assert(std::abs(
        policy.calculateFinalMark(80, 100, 0) - 80
    ) < 0.001);

    assert(std::abs(
        policy.calculateFinalMark(80, 100, 1) - 70
    ) < 0.001);

    assert(std::abs(
        policy.calculateFinalMark(80, 100, 2) - 60
    ) < 0.001);

    assert(std::abs(
        policy.calculateFinalMark(80, 100, 3) - 50
    ) < 0.001);

    assert(std::abs(
        policy.calculateFinalMark(80, 100, 4) - 0
    ) < 0.001);

    assert(std::abs(
        policy.calculateFinalMark(15, 100, 2) - 0
    ) < 0.001);

    return 0;
}