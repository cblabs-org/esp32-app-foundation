#pragma once
#include "interfaces/IPrecondition.hpp"

struct PreconditionHealthPayload {
    const char* name;
    Cause cause;
    Severity severity;
    bool everMet;
    // NO message/detail — subscribers call describe() into their own buffer (CG-2a)
};
