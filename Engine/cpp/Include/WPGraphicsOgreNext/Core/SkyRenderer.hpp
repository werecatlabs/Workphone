#ifndef SkyRenderer_h__
#define SkyRenderer_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        //--------------------------------------------
        class SkyRenderer : public ISharedObject
        {
        public:
            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;
        };

    }  // namespace render
}  // namespace workphone

#endif  // SkyRenderer_h__
