#ifndef IParticle_h__
#define IParticle_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        class WPCore_API IParticle : public ISharedObject
        {
        public:
            ~IParticle() override;

            virtual void *getData() const = 0;
            virtual void setData( void *data ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IParticle_h__
