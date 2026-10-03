#ifndef IMeshConverter_h__
#define IMeshConverter_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * Interface for a mesh converter that writes a mesh.
         */
        class WPCore_API IMeshConverter : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~IMeshConverter() override;

            /**
             * Writes a mesh for a given actor.
             * @param actor A smart pointer to the actor whose mesh should be written.
             */
            virtual void writeMesh( SmartPtr<scene::IGameActor> actor ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IMeshConverter_h__
