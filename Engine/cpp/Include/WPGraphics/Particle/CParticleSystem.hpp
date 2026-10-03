#ifndef __CParticleSystem_h__
#define __CParticleSystem_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/ParticleSystem.hpp>
#include <Workphone/Core/Pool.hpp>
#include "CParticleTechnique.hpp"
#include "WPGraphics/Particle/ParticleData.hpp"
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <list>

namespace workphone
{
    namespace render
    {
        class CParticleSystem : public ParticleSystem
        {
        public:
            static const hash32 UPDATE_HASH;

            CParticleSystem();
            ~CParticleSystem() override;

            void initialise( SmartPtr<IBuildDirector> objectTemplate,
                             SmartPtr<Properties> instanceProperties );

            void createComponent( SmartPtr<IParticleNode> particleComponent );

            void update() override;

            SmartPtr<IParticle> createParticle( SmartPtr<IParticleEmitter> emitter );

            void animate();
            void render();

            Vector3F *allocatePosition();
            Vector3F *allocateVelocity();

            void _preRenderUpdate();
            void _update();
            void _postRenderUpdate();

            void registerForUpdates( bool registerObject );
            bool isRegisteredForUpdates();

            void setTestOcclusion( bool testOcclusion );
            bool getTestOcclusion() const;

            void setIsOccluder( bool isOccluder );
            bool isOccluder() const;

            String getName() const override;

            u32 getId() const;

            void detachFromParent();
            void _attachToParent( SmartPtr<IGraphicsSceneNode> parent );

            void setMaterialName( const String &materialName, s32 index = -1 );
            String getMaterialName( s32 index = -1 ) const;

            void setCastShadows( bool castShadows ) override;
            bool getCastShadows() const override;

            void setRecieveShadows( bool recieveShadows );
            bool getRecieveShadows() const;

            void setVisible( bool visible ) override;
            bool isVisible() const override;

            void setZOrder( u32 zOrder ) override;
            u32 getZOrder() const override;

            void setVisibilityFlags( u32 flags ) override;
            u32 getVisibilityFlags() const override;

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            void setPropertyValue( const String &name, const String &value );
            bool getPropertyValue( const String &name, String &value );

            void setProperties( const Properties &propertyGroup );
            bool getProperties( Properties &propertyGroup, u32 flags = AllProperties ) const;

            void setFlag( u32 flag, bool value ) override;
            bool getFlag( u32 flag ) const override;

            //
            // IParticleSystem functions
            //
            void setFastForward( f32 time, f32 interval ) override;
            f32 getFastForwardTime() const override;
            f32 getFastForwardInterval() const override;

            void setTemplateName( const String &templateName ) override;
            String getTemplateName() const override;

            void setScale( const Vector3<real_Num> &scale ) override;
            Vector3<real_Num> getScale() const override;

            SmartPtr<IParticleTechnique> getTechnique( const String &name ) const override;

            void handleEvent( SmartPtr<IEvent> event );

            AABB3<real_Num> getLocalAABB() const override;

            void setLocalAABB( const AABB3<real_Num> &localAABB ) override;

            bool isAttached() const override;

            void setOwner( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            void load( void *data = nullptr );
            void unload();

            static void freeParticle( void *ptr );
            static void freeParticleData( void *ptr );

            Vector3<real_Num> getVelocityScale() const;
            void setVelocityScale( const Vector3<real_Num> &velocityScale );

            hash_type getRenderTechnique() const override;
            void setRenderTechnique( hash_type renderTechnique ) override;

            u32 getPoolSize() const;
            void setPoolSize( u32 poolSize );

        protected:
            u32 m_zOrder;

            Pool<ParticleData> m_particles;
            Pool<Vector3<real_Num>> m_positions;
            Pool<Vector3<real_Num>> m_velocities;

            SmartPtr<IGraphicsScene> m_sceneManager;

            SmartPtr<IMesh> m_particleMesh;
            SmartPtr<IDynamicMesh> m_graphicsMesh;

            Vector3<real_Num> m_scale;

            Vector3<real_Num> m_velocityScale;

            u32 m_poolSize;

            bool m_isPlaying;
            bool m_isVisible;

            String m_templateName;

            ConcurrentQueue<SmartPtr<IParticle>> m_addParticleQueue;
            ConcurrentQueue<SmartPtr<IParticle>> m_removeParticleQueue;

            using ParticleTechniques = HashMap<hash32, SmartPtr<IParticleTechnique>>;
            ParticleTechniques m_particleTechniques;

            static u32 m_nameExt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ParticleSystemManager_h__
