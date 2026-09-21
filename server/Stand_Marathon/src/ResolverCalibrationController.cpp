#include "ResolverCalibrationController.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr unsigned kSamplesPerAdjustment = 5;
constexpr unsigned kStableSamplesForConvergence = 20;
}

float ResolverCalibrationController::wrapPi(float angle)
{
    while (angle > kPi) angle -= kTwoPi;
    while (angle < -kPi) angle += kTwoPi;
    return angle;
}

void ResolverCalibrationController::start(
    float initialThetaCorrection,
    float gain,
    float tolerance,
    float maxStep)
{
    active_ = true;
    converged_ = false;
    command_ = wrapPi(initialThetaCorrection);
    bestCommand_ = command_;
    gain_ = std::clamp(gain, 0.01f, 1.0f);
    tolerance_ = std::clamp(tolerance, 0.0005f, 0.25f);
    maxStep_ = std::clamp(maxStep, 0.0005f, 0.2f);
    lastError_ = 0.0f;
    bestErrorAbs_ = 1000.0f;
    errorSum_ = 0.0f;
    sampleCount_ = 0;
    stableCount_ = 0;
    divergenceCount_ = 0;
    status_ = "waiting_for_valid_flux";
}

void ResolverCalibrationController::stop(const std::string& reason)
{
    active_ = false;
    converged_ = false;
    errorSum_ = 0.0f;
    sampleCount_ = 0;
    stableCount_ = 0;
    divergenceCount_ = 0;
    status_ = reason;
}

bool ResolverCalibrationController::processSample(float fluxPositionError, bool fluxErrorValid)
{
    if (!active_) return false;
    if (!fluxErrorValid || !std::isfinite(fluxPositionError)) {
        errorSum_ = 0.0f;
        sampleCount_ = 0;
        stableCount_ = 0;
        status_ = "waiting_for_speed";
        return false;
    }

    lastError_ = wrapPi(fluxPositionError);
    const float errorAbs = std::fabs(lastError_);
    if (errorAbs < bestErrorAbs_) {
        bestErrorAbs_ = errorAbs;
        bestCommand_ = command_;
        divergenceCount_ = 0;
    } else if (errorAbs > bestErrorAbs_ + std::max(0.02f, bestErrorAbs_ * 0.35f)) {
        ++divergenceCount_;
    }

    if (errorAbs <= tolerance_) {
        ++stableCount_;
        status_ = "stabilizing";
        if (stableCount_ >= kStableSamplesForConvergence) {
            converged_ = true;
            command_ = bestCommand_;
            status_ = "converged";
        }
        return false;
    }

    converged_ = false;
    stableCount_ = 0;
    errorSum_ += lastError_;
    ++sampleCount_;
    if (sampleCount_ < kSamplesPerAdjustment) {
        status_ = "measuring";
        return false;
    }

    if (divergenceCount_ >= 10) {
        command_ = bestCommand_;
        gain_ = std::max(0.01f, gain_ * 0.5f);
        divergenceCount_ = 0;
        errorSum_ = 0.0f;
        sampleCount_ = 0;
        status_ = "rollback_step_reduced";
        return true;
    }

    const float averageError = errorSum_ / static_cast<float>(sampleCount_);
    const float step = std::clamp(gain_ * averageError, -maxStep_, maxStep_);

    // With phase_inverse == 0, positive flux error means the electrical angle
    // estimate leads the rotor flux, so the correction must be reduced.
    command_ = wrapPi(command_ - step);
    errorSum_ = 0.0f;
    sampleCount_ = 0;
    status_ = "adjusting";
    return true;
}
