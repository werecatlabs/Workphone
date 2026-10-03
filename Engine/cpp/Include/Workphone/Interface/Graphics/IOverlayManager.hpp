#ifndef _IOverlayManager_H
#define _IOverlayManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for an overlay manager, used to create and manage overlays.
         */
        class WPCore_API IOverlayManager : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~IOverlayManager() override = default;

            /** Adds a new overlay with the specified name. */
            virtual SmartPtr<IOverlay> addOverlay( const String &instanceName ) = 0;

            /** Removes the overlay with the specified name. */
            virtual bool removeOverlay( const String &instanceName ) = 0;

            /** Removes the specified overlay. */
            virtual bool removeOverlay( SmartPtr<IOverlay> overlay ) = 0;

            /** Returns an array of all the overlays managed by this manager. */
            virtual Array<SmartPtr<IOverlay>> getOverlays() const = 0;

            /** Finds and returns the overlay with the specified name. */
            virtual SmartPtr<IOverlay> findOverlay( const String &instanceName ) = 0;

            /** Returns true if an overlay with the specified name exists. */
            virtual bool hasOverlay( const String &instanceName ) = 0;

            /** Adds a new overlay element with the specified type and name. */
            virtual SmartPtr<IOverlayElement> addElement( const String &typeName,
                                                          const String &instanceName ) = 0;

            /** Finds and returns the overlay element with the specified name. */
            virtual SmartPtr<IOverlayElement> findElement( const String &instanceName ) = 0;

            /** Removes the specified overlay element. */
            virtual bool removeElement( SmartPtr<IOverlayElement> element ) = 0;

            /** Gets a pointer to the underlying object. This is dependent on the graphics library used.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
