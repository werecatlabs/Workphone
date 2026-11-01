#pragma once

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <memory>

namespace workphone::ui::frame_retention
{
    class WPGraphics_API Scope
    {
    public:
        explicit Scope( Array<SmartPtr<ISharedObject>> &owners,Array<std::shared_ptr<const void>> &snapshots );
        ~Scope();
        Scope( const Scope & ) = delete;
        Scope &operator=( const Scope & ) = delete;

    private:
        Array<SmartPtr<ISharedObject>> *previous;
        Array<std::shared_ptr<const void>> *previousSnapshots;
    };

    WPGraphics_API void retainFontOwner( IUIElement *owner );
    WPGraphics_API void retainResource(SmartPtr<ISharedObject> owner);
    WPGraphics_API void retainSnapshot(std::shared_ptr<const void> owner);
}
