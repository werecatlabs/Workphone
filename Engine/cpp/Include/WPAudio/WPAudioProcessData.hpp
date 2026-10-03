#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Sound/IAudioProcessData.hpp>

namespace workphone
{
    class CAudioProcessData : public IAudioProcessData
    {
    public:
        CAudioProcessData();
        ~CAudioProcessData() override;
    };
}  // namespace workphone
