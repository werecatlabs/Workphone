#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IInputManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IInputManager, ISharedObject );

    const u32 IInputManager::isAssigningFlag = 1 << 1;
    const u32 IInputManager::isShiftPressedFlag = 1 << 2;
    const u32 IInputManager::useOverrideFlag = 1 << 3;
    const u32 IInputManager::enableInputLogFlag = 1 << 4;
    const u32 IInputManager::runInputLogFlag = 1 << 5;
    const u32 IInputManager::bufferedKeysFlag = 1 << 6;
    const u32 IInputManager::bufferedMouseFlag = 1 << 7;
    const u32 IInputManager::inputCaptureFlag = 1 << 8;
    const u32 IInputManager::isLeftPressedFlag = 1 << 9;
    const u32 IInputManager::isRightPressedFlag = 1 << 10;
    const u32 IInputManager::isMiddlePressedFlag = 1 << 11;
    const u32 IInputManager::cursorVisibleFlag = 1 << 12;

    IInputManager::~IInputManager() = default;
}  // namespace workphone
