#pragma once
#include <inc/types.h>

namespace PerformanceLog {
    enum class LogState {
        Idle,
        Waiting,
        Recording,
        Finished
    };

    LogState GetState();

    void Cancel();
    void Start();
    void Update(Vehicle playerVehicle);
    void Finish();
}
