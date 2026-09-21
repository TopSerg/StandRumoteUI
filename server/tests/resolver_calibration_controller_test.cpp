#include "ResolverCalibrationController.h"

#include <cassert>
#include <cmath>

int main()
{
    ResolverCalibrationController controller;
    controller.start(0.5f, 0.2f, 0.01f, 0.02f);
    assert(controller.active());

    for (int i = 0; i < 5; ++i) {
        controller.processSample(0.5f, true);
    }
    assert(controller.command() < 0.5f);
    assert(std::fabs(controller.command() - 0.48f) < 1e-5f);

    const float held = controller.command();
    for (int i = 0; i < 10; ++i) {
        controller.processSample(0.0f, false);
    }
    assert(controller.command() == held);
    assert(controller.status() == "waiting_for_speed");

    for (int i = 0; i < 20; ++i) {
        controller.processSample(0.005f, true);
    }
    assert(controller.converged());
    assert(controller.status() == "converged");

    controller.stop("stopped");
    assert(!controller.active());
    return 0;
}
