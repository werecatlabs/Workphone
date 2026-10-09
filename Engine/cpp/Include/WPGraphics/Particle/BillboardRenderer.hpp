#ifndef BillboardRenderer_h__
#define BillboardRenderer_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Graphics/IParticleRenderer.hpp>
#include "WPGraphics/Particle/CParticleNode.hpp"
#include <list>

namespace workphone
{
    namespace render
    {
        class BillboardRenderer : public CParticleNode<IParticleRenderer>
        {
        public:
            BillboardRenderer( CParticleSystem *particleSystem );
            ~BillboardRenderer() override;

            void initialise( SmartPtr<IBuildDirector> objectTemplate ) override;

            void update() override;

            void addParticle( SmartPtr<IParticle> particle ) override;
            void removeParticle( SmartPtr<IParticle> particle ) override;
            void clearParticles() override;

            virtual void OnUpdateParticles() const;

            void setVisibilityFlags( u32 flags ) override;
            u32 getVisibilityFlags() const override;

            virtual void calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                         void *data = nullptr );

            SmartPtr<IParticleSystem> getParticleSystem() const override;
            void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) override;

            void setMaterialName( const String &materialName, s32 index = -1 ) override;
            String getMaterialName( s32 index = -1 ) const override;

            SmartPtr<IGraphicsSceneNode> getParentSceneNode() const override;
            void setParentSceneNode( SmartPtr<IGraphicsSceneNode> parentSceneNode ) override;

        protected:
            template <typename T>
            struct ParticleUpdate
            {
                ParticleUpdate( BillboardRenderer *renderer );
                void operator()( T &it ) const;

                BillboardRenderer *m_renderer;
            };

            class ParticleSystemListener  //: public IParticleSystemListener
            {
            public:
                ParticleSystemListener( BillboardRenderer *renderer ) : m_renderer( renderer )
                {
                }

                BillboardRenderer *m_renderer;
            };

            SmartPtr<IGraphicsSceneNode> m_parentSceneNode;

            SmartPtr<IParticleSystem> m_particleSystem;
            SmartPtr<IBillboardSet> m_bbSet;
            u32 m_visibilityFlags;

            String m_materialName;

            Array<SmartPtr<IBillboard>> m_bbs;

            std::list<SmartPtr<IParticle>> m_particles;

            static u32 m_nameExt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // BillboardRenderer_h__
