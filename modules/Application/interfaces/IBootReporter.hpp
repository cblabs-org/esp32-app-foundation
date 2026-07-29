#pragma once

#include "interfaces/IStatusReporter.hpp"

class IBootReporter : public IStatusReporter {
public:
    virtual bool beginEarly() = 0;
};
