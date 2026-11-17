#pragma once

#include "PTMTypes.h"
#include <string>

namespace SantecPTM {

class PTM_API ISantecPTMCommAdapter
{
public:
    virtual ~ISantecPTMCommAdapter() {}
    virtual bool Open(const std::string& address, int port, double timeout) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual std::string SendQuery(const std::string& command) = 0;
    virtual void SendWrite(const std::string& command) = 0;
};

} // namespace SantecPTM
