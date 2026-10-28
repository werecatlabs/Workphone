#pragma once

#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone::ui::frame_retention
{
    inline thread_local Array<SmartPtr<IUIElement>> *currentOwners = nullptr;

    class Scope
    {
    public:
        explicit Scope( Array<SmartPtr<IUIElement>> &owners ) : previous( currentOwners )
        {
            currentOwners = &owners;
        }
        ~Scope() { currentOwners = previous; }
        Scope( const Scope & ) = delete;
        Scope &operator=( const Scope & ) = delete;

    private:
        Array<SmartPtr<IUIElement>> *previous;
    };

    inline void retainFontOwner( IUIElement *owner )
    {
        if( currentOwners ) currentOwners->push_back( SmartPtr<IUIElement>( owner ) );
    }
}
