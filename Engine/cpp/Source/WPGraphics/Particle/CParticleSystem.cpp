#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Particle/CParticleSystem.hpp"
#include "WPGraphics/Particle/CParticle.hpp"
#include "WPGraphics/Particle/ParticleData.hpp"
#include "WPGraphics/Particle/ParticleState.hpp"
#include "WPGraphics/Particle/Emitters/PointEmitter.hpp"
#include "WPGraphics/Particle/Affectors/ColourAffector.hpp"
#include "WPGraphics/Particle/Affectors/ScaleAffector.hpp"
#include "WPGraphics/Particle/Renderers/BillboardRenderer.hpp"
#include "WPGraphics/Particle/CParticleTechnique.hpp"
#include <Workphone/Workphone.hpp>
#include <cmath>

namespace workphone
{
    namespace render
    {
        u32 CParticleSystem::m_nameExt = 0;
        // using ParticlePool = SingletonPool<CParticle, sizeof( CParticle )>;
        const hash32 CParticleSystem::UPDATE_HASH =
            static_cast<hash32>( StringUtil::getHash( "update" ) );

        //-------------------------------------------------
        CParticleSystem::CParticleSystem() :
            m_poolSize( 1000 ),
            m_isPlaying( false ),
            m_isVisible( true )
        {
            m_scale = Vector3<real_Num>( 1, 1, 1 );
            m_velocityScale = Vector3<real_Num>( 1, 1, 1 );
            u32 maxNumParticles = m_poolSize;
            m_particles.setNextSize( maxNumParticles );
            m_positions.setNextSize( maxNumParticles );
            m_velocities.setNextSize( maxNumParticles );
        }

        //-------------------------------------------------
        CParticleSystem::~CParticleSystem()
        {
            wp_particle_simulation_destroy( m_simulation );
        }

        //-------------------------------------------------
        void CParticleSystem::initialise( SmartPtr<IBuildDirector> objectTemplate,
                                          SmartPtr<Properties> instanceProperties )
        {
            /*
            SmartPtr<ParticleSystemTemplate> particleSystemTemplate;  // = objectTemplate;

            auto applicationManager = core::ApplicationManager::instance();
            if( !applicationManager )
                WP_EXCEPTION( "Unknown error" );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( !graphicsSystem )
                WP_EXCEPTION( "Unknown error" );

            String sceneManagerName( "Default" );
            if( instanceProperties )
            {
                instanceProperties->getPropertyValue( "sceneManagerName", sceneManagerName );
            }

            //m_sceneManager = graphicsSystem->getSceneManager(sceneManagerName).get();
            if( !m_sceneManager )
                WP_EXCEPTION( "Unknown error" );

            if( particleSystemTemplate )
            {
                m_scale = particleSystemTemplate->getScale();
                m_velocityScale = particleSystemTemplate->getScaleVelocity();

                Array<SmartPtr<ParticleComponentTemplate>> particleComponentTemplates =
                    particleSystemTemplate->getParticleComponents();
                for( u32 i = 0; i < particleComponentTemplates.size(); ++i )
                {
                    SmartPtr<ParticleComponentTemplate> particleComponentTemplate =
                        particleComponentTemplates[i];
                    createComponent( particleComponentTemplate, nullptr );
                }
            }
             */
        }

        //-------------------------------------------------
        void CParticleSystem::createComponent( SmartPtr<IParticleNode> particleComponent )
        {
            /*
            String particleComponentType = particleComponentTemplate->getParticleComponentType();
            String particleComponentSubType = particleComponentTemplate->getParticleComponentSubType();

            SmartPtr<IParticleTechnique> technique;  // = particleComponent;

            if( particleComponentType == "Technique" )
            {
                //SmartPtr<IParticleTechnique> particleTechnique(new CParticleTechnique);
                //particleTechnique->setParticleSystem(this);

                //SmartPtr<Handle> handle = particleTechnique->getHandle();
                //hash32 hash = handle->getHash();
                //m_particleTechniques[hash] = particleTechnique;

                //technique = particleTechnique;
            }
            else if( particleComponentType == "Renderer" )
            {
                //SmartPtr<ParticleRendererTemplate> particleRendererTemplate =
            particleComponentTemplate;

                //SmartPtr<IParticleRenderer> particleRenderer(new BillboardRenderer(this));
                //particleRenderer->initialise(particleRendererTemplate);
                //particleRenderer->setParticleSystem(this);
                //particleRenderer->setParent(technique.get());

                //SmartPtr<IParticleTechnique> particleTechnique = particleComponent;
                //particleTechnique->addRenderer(particleRenderer);
            }
            else if( particleComponentType == "Emitter" )
            {
                //IParticleEmitter* emitter = new CPointEmitter;
                //emitter->initialise(particleComponentTemplate);
                //emitter->setParticleSystem(this);
                //emitter->setParent(technique.get());
                //technique->addEmitter(SmartPtr<IParticleEmitter>(emitter));
            }
            else if( particleComponentType == "Affector" )
            {
                //if ( particleComponentSubType == "Colour" )
                //{
                //	ColourAffectorPtr colourAffector(new ColourAffector);
                //	colourAffector->initialise(particleComponentTemplate);
                //	colourAffector->setParticleSystem(this);
                //	colourAffector->setParent(technique.get());
                //	technique->addAffector(colourAffector);
                //}
            }

            Array<SmartPtr<ParticleComponentTemplate>> children =
                particleComponentTemplate->getChildren();
            for( u32 childIdx = 0; childIdx < children.size(); ++childIdx )
            {
                SmartPtr<ParticleComponentTemplate> child = children[childIdx];
                createComponent( child, technique );
            }
             */
        }

        //-------------------------------------------------
        void CParticleSystem::update()
        {
            // GraphicsScene advances particles during preparation, before all view draws.
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
            if( timer && !simulate( static_cast<f32>( timer->getDeltaTime() ) ) && isLoaded() )
                WP_LOG_ERROR( "Claw particles: invalid timestep or settings; frame was not advanced." );
        }

        //-------------------------------------------------
        SmartPtr<IParticle> CParticleSystem::createParticle( SmartPtr<IParticleEmitter> emitter )
        {
            return nullptr;
        }

        //-------------------------------------------------
        void CParticleSystem::animate()
        {
            update();
        }

        //-------------------------------------------------
        void CParticleSystem::render()
        {
            // Rendering consumes getRenderSnapshot() through ClawScene, without advancing time.
        }

        //-------------------------------------------------
        void CParticleSystem::_getObject( void **ppObject ) const
        {
            if( ppObject ) *ppObject = m_simulation;
        }

        void CParticleSystem::handleEvent( SmartPtr<IEvent> event )
        {
        }

        AABB3<real_Num> CParticleSystem::getLocalAABB() const
        {
            auto samples = getRenderSnapshot();
            if( samples.empty() ) return {};
            Vector3<real_Num> minimum( samples[0].position.x, samples[0].position.y, samples[0].position.z );
            auto maximum = minimum;
            for( const auto &p : samples )
            {
                const real_Num half = p.size * 0.5f;
                minimum = Vector3<real_Num>( std::min(minimum.X(), real_Num(p.position.x)-half),
                    std::min(minimum.Y(), real_Num(p.position.y)-half), std::min(minimum.Z(), real_Num(p.position.z)-half) );
                maximum = Vector3<real_Num>( std::max(maximum.X(), real_Num(p.position.x)+half),
                    std::max(maximum.Y(), real_Num(p.position.y)+half), std::max(maximum.Z(), real_Num(p.position.z)+half) );
            }
            return AABB3<real_Num>( minimum, maximum );
        }

        void CParticleSystem::setLocalAABB( const AABB3<real_Num> &localAABB )
        {
        }

        bool CParticleSystem::isAttached() const
        {
            return ParticleSystem::isAttached();
        }

        void CParticleSystem::setOwner( SmartPtr<IGraphicsSceneNode> sceneNode )
        {
            ParticleSystem::setOwner( sceneNode );
            auto it = m_particleTechniques.begin();
            for( ; it != m_particleTechniques.end(); ++it )
            {
                SmartPtr<IParticleTechnique> technique = it->second;

                Array<SmartPtr<IParticleRenderer>> renderers = technique->getParticleRenderers();
                for( u32 i = 0; i < renderers.size(); ++i )
                {
                    SmartPtr<IParticleRenderer> particleRenderer = renderers[i];
                    // particleRenderer->setParentSceneNode(sceneNode);
                }
            }
        }

        void CParticleSystem::load( void *data /*= nullptr */ )
        {
            load( SmartPtr<ISharedObject>() );
        }

        void CParticleSystem::unload()
        {
            unload( SmartPtr<ISharedObject>() );
        }

        SmartPtr<IParticleTechnique> CParticleSystem::getTechnique( const String &name ) const
        {
            return ParticleSystem::getTechnique( name );
        }

        Vector3<real_Num> CParticleSystem::getScale() const
        {
            return m_scale;
        }

        void CParticleSystem::setScale( const Vector3<real_Num> &scale )
        {
            m_scale = scale;
        }

        String CParticleSystem::getTemplateName() const
        {
            return m_templateName;
        }

        void CParticleSystem::setTemplateName( const String &templateName )
        {
            m_templateName = templateName;
        }

        f32 CParticleSystem::getFastForwardInterval() const
        {
            return ParticleSystem::getFastForwardInterval();
        }

        f32 CParticleSystem::getFastForwardTime() const
        {
            return ParticleSystem::getFastForwardTime();
        }

        void CParticleSystem::setFastForward( f32 time, f32 interval )
        {
            ParticleSystem::setFastForward( time, interval );
        }

        bool CParticleSystem::getFlag( u32 flag ) const
        {
            return false;
        }

        void CParticleSystem::setFlag( u32 flag, bool value )
        {
        }

        bool CParticleSystem::getProperties( Properties &propertyGroup,
                                             u32 flags /*= AllProperties*/ ) const
        {
            return false;
        }

        void CParticleSystem::setProperties( const Properties &propertyGroup )
        {
        }

        bool CParticleSystem::getPropertyValue( const String &name, String &value )
        {
            return false;
        }

        void CParticleSystem::setPropertyValue( const String &name, const String &value )
        {
        }

        SmartPtr<IGraphicsObject> CParticleSystem::clone(
            const String &name /*= StringUtil::EmptyString*/ ) const
        {
            return nullptr;
        }

        u32 CParticleSystem::getVisibilityFlags() const
        {
            return m_visibilityFlags;
        }

        void CParticleSystem::setVisibilityFlags( u32 flags )
        {
            m_visibilityFlags = flags;
            ParticleSystem::setVisibilityFlags( flags );
        }

        bool CParticleSystem::isVisible() const
        {
            return m_isVisible;
        }

        void CParticleSystem::setVisible( bool visible )
        {
            m_isVisible = visible;
        }

        bool CParticleSystem::getRecieveShadows() const
        {
            return false;
        }

        void CParticleSystem::setRecieveShadows( bool recieveShadows )
        {
        }

        bool CParticleSystem::getCastShadows() const
        {
            return false;
        }

        void CParticleSystem::setCastShadows( bool castShadows )
        {
        }

        String CParticleSystem::getMaterialName( s32 index /*= -1*/ ) const
        {
            return m_materialName;
        }

        void CParticleSystem::setMaterialName( const String &materialName, s32 index /*= -1*/ )
        {
            m_materialName = materialName;
        }

        void CParticleSystem::_attachToParent( SmartPtr<IGraphicsSceneNode> parent )
        {
            setOwner( parent );
        }

        void CParticleSystem::detachFromParent()
        {
            setOwner( nullptr );
        }

        u32 CParticleSystem::getId() const
        {
            return ParticleSystem::getId();
        }

        String CParticleSystem::getName() const
        {
            return ParticleSystem::getName();
        }

        bool CParticleSystem::isOccluder() const
        {
            return false;
        }

        void CParticleSystem::setIsOccluder( bool isOccluder )
        {
        }

        bool CParticleSystem::getTestOcclusion() const
        {
            return false;
        }

        void CParticleSystem::setTestOcclusion( bool testOcclusion )
        {
        }

        bool CParticleSystem::isRegisteredForUpdates()
        {
            return false;
        }

        void CParticleSystem::registerForUpdates( bool registerObject )
        {
        }

        void CParticleSystem::_postRenderUpdate()
        {
        }

        void CParticleSystem::_update()
        {
        }

        void CParticleSystem::_preRenderUpdate()
        {
        }

        void CParticleSystem::freeParticle( void *ptr )
        {
            auto particle = static_cast<CParticle *>( ptr );
            particle->~CParticle();
            // ParticlePool::free( ptr );
        }

        void CParticleSystem::freeParticleData( void *ptr )
        {
        }

        hash_type CParticleSystem::getRenderTechnique() const
        {
            return 0;
        }

        void CParticleSystem::setRenderTechnique( hash_type renderTechnique )
        {
        }

        void CParticleSystem::setVelocityScale( const Vector3<real_Num> &velocityScale )
        {
            m_velocityScale = velocityScale;
        }

        Vector3<real_Num> CParticleSystem::getVelocityScale() const
        {
            return m_velocityScale;
        }

        u32 CParticleSystem::getPoolSize() const
        {
            return m_poolSize;
        }

        void CParticleSystem::setPoolSize( u32 poolSize )
        {
            std::lock_guard<std::mutex> lock( m_simulationMutex );
            if( m_simulation || !poolSize || poolSize > WP_PARTICLE_SIMULATION_MAX_CAPACITY )
            {
                WP_LOG_ERROR( "Claw particles: capacity must be valid and configured before load." );
                return;
            }
            m_poolSize = poolSize;
        }

        u32 CParticleSystem::getZOrder() const
        {
            return m_zOrder;
        }

        void CParticleSystem::setZOrder( u32 zOrder )
        {
            m_zOrder = zOrder;
        }
    }  // namespace render
}  // namespace workphone
