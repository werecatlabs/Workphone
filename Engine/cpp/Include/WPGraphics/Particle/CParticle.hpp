#ifndef CParticle_h__
#define CParticle_h__

#include "WPGraphics/WPClawHammerPrerequisites.hpp"
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        class CParticle : public IParticle
        {
        public:
            CParticle();
            ~CParticle() override;

            void *getData() const override;
            void setData( void *data ) override;

        protected:
            ParticleData *m_data;
        };

        using CParticlePtr = SmartPtr<CParticle>;
    }  // namespace render
}  // namespace workphone

#endif  // CParticle_h__
