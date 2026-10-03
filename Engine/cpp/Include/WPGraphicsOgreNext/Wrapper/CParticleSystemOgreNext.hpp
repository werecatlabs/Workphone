#ifndef _CParticleSystem_H
#define _CParticleSystem_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/ParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleNode.hpp>
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{
    namespace render
    {

        class CParticleSystemOgreNext : public CGraphicsObjectOgreNext<ParticleSystem>
        {
        public:
            CParticleSystemOgreNext();
            CParticleSystemOgreNext( IGraphicsScene *creator );
            ~CParticleSystemOgreNext() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void reload( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            SmartPtr<IGraphicsObject> clone( const String &name = StringUtil::EmptyString ) const;

            void _getObject( void **ppObject ) const;

            void setState( ParticleSystemState state ) override;

            size_t getNumParticles() const override;

            size_t getNumEmitters() const override;

            SmartPtr<IParticle> addParticle();

            Ogre::ParticleSystem *getParticleSystem() const;

            String getResourceGroupName() const;
            void setResourceGroupName( const String &resourceGroupName );

            size_t getInitialParticleQuota() const;
            void setInitialParticleQuota( size_t initialParticleQuota );

            size_t getParticleQuota() const;
            void setParticleQuota( size_t particleQuota );

            size_t getEmittedEmitterQuota() const;
            void setEmittedEmitterQuota( size_t emittedEmitterQuota );

            String getMaterialName() const;
            void setMaterialName( const String &materialName );

            String getRendererName() const;
            void setRendererName( const String &rendererName );

            Vector2<real_Num> getDefaultDimensions() const;
            void setDefaultDimensions( const Vector2<real_Num> &defaultDimensions );

            f32 getSpeedFactor() const;
            void setSpeedFactor( f32 speedFactor );

            f32 getIterationInterval() const;
            void setIterationInterval( f32 iterationInterval );

            f32 getNonVisibleUpdateTimeout() const;
            void setNonVisibleUpdateTimeout( f32 nonVisibleUpdateTimeout );

            bool getCullIndividually() const;
            void setCullIndividually( bool cullIndividually );

            bool getSortingEnabled() const;
            void setSortingEnabled( bool sortingEnabled );

            bool getKeepParticlesInLocalSpace() const;
            void setKeepParticlesInLocalSpace( bool keepParticlesInLocalSpace );

            bool getBoundsAutoUpdated() const;
            void setBoundsAutoUpdated( bool boundsAutoUpdated );

            f32 getBoundsUpdateTime() const;
            void setBoundsUpdateTime( f32 boundsUpdateTime );

            bool getEmitting() const;
            void setEmitting( bool emitting );

            bool getTranslateParticleDirectionIntoWorldSpace() const;
            void setTranslateParticleDirectionIntoWorldSpace(
                bool translateParticleDirectionIntoWorldSpace );

            WP_CLASS_REGISTER_DECL;

            static const String ResourceGroupNameStr;
            static const String InitialParticleQuotaStr;
            static const String ParticleQuotaStr;
            static const String EmittedEmitterQuotaStr;
            static const String MaterialNameStr;
            static const String RendererNameStr;
            static const String DefaultDimensionsStr;
            static const String SpeedFactorStr;
            static const String IterationIntervalStr;
            static const String NonVisibleUpdateTimeoutStr;
            static const String CullIndividuallyStr;
            static const String SortingEnabledStr;
            static const String KeepParticlesInLocalSpaceStr;
            static const String BoundsAutoUpdatedStr;
            static const String BoundsUpdateTimeStr;
            static const String EmittingStr;
            static const String TranslateParticleDirectionIntoWorldSpaceStr;

        protected:
            void setupStateObject() override;

            Ogre::ParticleSystem *createOgreParticleSystem( Ogre::SceneManager *sceneManager ) const;
            void applyParticleSystemSettings();
            void configureDefaultEmitter();

            String m_resourceGroupName = "General";
            String m_materialName = "Workphone/Particles/Default";
            String m_rendererName = "billboard";
            Vector2<real_Num> m_defaultDimensions = Vector2<real_Num>( 1.0f, 1.0f );
            size_t m_initialParticleQuota = 100u;
            size_t m_particleQuota = 1000u;
            size_t m_emittedEmitterQuota = 0u;
            f32 m_speedFactor = 1.0f;
            f32 m_iterationInterval = 0.0f;
            f32 m_nonVisibleUpdateTimeout = 0.0f;
            f32 m_boundsUpdateTime = 10.0f;
            bool m_cullIndividually = false;
            bool m_sortingEnabled = false;
            bool m_keepParticlesInLocalSpace = false;
            bool m_boundsAutoUpdated = true;
            bool m_emitting = true;
            bool m_translateParticleDirectionIntoWorldSpace = false;
        };

    }  // namespace render
}  // namespace workphone

#endif
