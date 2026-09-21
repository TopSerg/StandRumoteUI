#pragma once

#include <cstdint>
#include <string>

class ResolverCalibrationController {
public:
    void start(float initialThetaCorrection, float gain, float tolerance, float maxStep);
    void stop(const std::string& reason = "idle");

    // Returns true when a new theta-correction command was calculated.
    bool processSample(float fluxPositionError, bool fluxErrorValid);

    bool active() const { return active_; }
    bool converged() const { return converged_; }
    float command() const { return command_; }
    float lastError() const { return lastError_; }
    float bestErrorAbs() const { return bestErrorAbs_; }
    const std::string& status() const { return status_; }

private:
    static float wrapPi(float angle);

    bool active_ = false;
    bool converged_ = false;
    float command_ = 0.0f;
    float gain_ = 0.2f;
    float tolerance_ = 0.01f;
    float maxStep_ = 0.02f;
    float lastError_ = 0.0f;
    float bestErrorAbs_ = 1000.0f;
    float bestCommand_ = 0.0f;
    float errorSum_ = 0.0f;
    unsigned sampleCount_ = 0;
    unsigned stableCount_ = 0;
    unsigned divergenceCount_ = 0;
    std::string status_ = "idle";
};
