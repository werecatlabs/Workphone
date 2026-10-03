#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Sound/IAudioBusBuffers.hpp>

namespace workphone
{
    class CAudioBusBuffers : public IAudioBusBuffers
    {
    public:
        CAudioBusBuffers();
        ~CAudioBusBuffers() override;
    };
}  // namespace workphone
