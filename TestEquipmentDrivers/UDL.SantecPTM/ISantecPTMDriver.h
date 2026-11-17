#pragma once

#include "PTMTypes.h"
#include <vector>
#include <string>

namespace SantecPTM {

class PTM_API ISantecPTMDriver
{
public:
    virtual ~ISantecPTMDriver() {}

    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual bool Initialize() = 0;
    virtual DeviceInfo GetDeviceInfo() = 0;
    virtual ErrorInfo CheckError() = 0;

    virtual int GetChannelCount() = 0;

    // nChannels <= 0 → use CHANnel? full count
    virtual bool TakeMeasurement(int nChannels = 0) = 0;
    virtual std::vector<PolarityChannelResult> GetResults() = 0;
    virtual std::string GetRawResult() = 0;

    virtual int Detect(int outputChannel) = 0;
    virtual void SetLocalMode(bool enabled) = 0;
    virtual bool GetLocalMode() = 0;
    virtual void LaserOff() = 0;
    virtual void SetVfl(int outputChannel, VflMode mode) = 0;
    virtual std::string GetBarcode() = 0;
    virtual void Notify(int icon, const std::string& message) = 0;

    virtual bool TestStart() = 0;
    virtual std::string TestStatus() = 0;
    virtual bool TestNext() = 0;
    virtual bool TestRetry() = 0;

    virtual std::string SendRawQuery(const std::string& command) = 0;
    virtual void SendRawWrite(const std::string& command) = 0;
};

} // namespace SantecPTM
