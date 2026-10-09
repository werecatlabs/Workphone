#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/Particle/BillboardRenderer.hpp>
#include <WPGraphics/Particle/ParticleData.hpp>
#include <WPGraphics/Particle/ParticleState.hpp>
#include <WPGraphics/Particle/CParticle.hpp>
#include <WPGraphics/Particle/CParticleSystem.hpp>
#include <Workphone/Core/ParallelForEach.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        u32 BillboardRenderer::m_nameExt = 0;

        //-----------------------------------------------------------
        BillboardRenderer::BillboardRenderer( CParticleSystem *particleSystem ) :
            m_parentSceneNode( nullptr ),
            m_particleSystem( particleSystem )
        {
            String name = String( "BillboardRenderer" ) + StringUtil::toString( m_nameExt++ );
            // setName( name );
        }

        //-----------------------------------------------------------
        BillboardRenderer::~BillboardRenderer()
        {
        }

        //-----------------------------------------------------------
        void BillboardRenderer::addParticle( SmartPtr<IParticle> particle )
        {
            // if(m_particles.size() < 10000)
            //{
            //	ParticleData* particleData = (ParticleData*)particle->getData();

            //	particleData->setRenderer(this);

            //	SmartPtr<IBillboard> bb = m_bbSet->createBillboard();
            //	m_bbs.push_back(bb);
            //	particleData->m_graphicsData = bb;

            //	m_particles.push_back(particle);
            //}
        }

        //-----------------------------------------------------------
        void BillboardRenderer::removeParticle( SmartPtr<IParticle> particle )
        {
            // std::list<SmartPtr<IParticle>>::iterator it = std::find(m_particles.begin(),
            // m_particles.end(), particle); if ( it != m_particles.end() )
            //{
            //	ParticleData* particleData = (ParticleData*)particle->getData();
            //	IBillboard* bb = (IBillboard*)particleData->m_graphicsData;

            //	m_bbSet->removeBillboard(bb);
            //	particleData->m_graphicsData = nullptr;

            //	m_particles.erase(it);
            //}
        }

        //-----------------------------------------------------------
        void BillboardRenderer::clearParticles()
        {
            m_particles.clear();
        }

        //-----------------------------------------------------------
        void BillboardRenderer::OnUpdateParticles() const
        {
        }

        //-----------------------------------------------------------
        void BillboardRenderer::initialise( SmartPtr<IBuildDirector> objectTemplate )
        {
            // SmartPtr<ParticleRendererTemplate> particleRendererTemplate = objectTemplate;

            // auto applicationManager = core::ApplicationManager::instance();
            // if(!engine)
            //	WP_EXCEPTION("Unknown error");

            // SmartPtr<IGraphicsSystem>& graphicsSystem = engine->getGraphicsSystem();
            // if(!graphicsSystem)
            //	WP_EXCEPTION("Unknown error");

            // SmartPtr<render::IGraphicsSceneManager> sceneManager =
            // m_particleSystem->getSceneManager(); if(!sceneManager) 	WP_EXCEPTION("Unknown error");

            // m_bbSet = sceneManager->addBillboardSet(String("ParticleSystemTest") +
            // StringUtil::toString(m_nameExt++)); m_bbSet->setDefaultDimensions(Vector3F::unit()
            // * 1.0f);

            // m_bbSet->setMaterialName(particleRendererTemplate->getMaterialName());
        }

        //-----------------------------------------------------------
        void BillboardRenderer::setVisibilityFlags( u32 flags )
        {
            m_visibilityFlags = flags;

            m_bbSet->setVisibilityFlags( flags );
        }

        //-----------------------------------------------------------
        u32 BillboardRenderer::getVisibilityFlags() const
        {
            return m_visibilityFlags;
        }

        //-----------------------------------------------------------
        void BillboardRenderer::update()
        {
            //		switch(task)
            //		{
            //		case TaskId::Render:
            //			{
            //				WP_ASSERT(m_particleSystem);
            //
            //				Vector3F scale = m_particleSystem->getScale();
            //
            //				CParticleSystem* ps = static_cast<CParticleSystem*>(m_particleSystem);
            //
            // #if 1
            //				tbb::parallel_for_each(m_particles.begin(), m_particles.end(),
            // ParticleUpdate<SmartPtr<IParticle>>(this)); #else
            // std::list<SmartPtr<IParticle>>::iterator it = m_particles.begin(); 				for(; it
            // != m_particles.end(); ++it)
            //				{
            //					SmartPtr<IParticle>& particle = (*it);
            //
            //					ParticleData* particleData =
            // static_cast<ParticleData*>(particle->getData()); WP_ASSERT(particleData);
            //
            //					IBillboard* bb = static_cast<IBillboard*>(particleData->m_graphicsData);
            //					WP_ASSERT(bb);
            //
            //					bb->setPosition(particleData->m_currentState->m_position);
            //					bb->setScale(particleData->m_currentState->m_scale * scale);
            //
            //					const Vector4F& colour = particleData->m_currentState->m_colour;
            //					bb->setColour(ColourF(colour[0], colour[1], colour[2], colour[3]));
            //				}
            // #endif
            //			}
            //			break;
            //		default:
            //			{
            //			}
            //		}
        }

        //-----------------------------------------------------------
        void BillboardRenderer::calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                                void *data /*= nullptr */ )
        {
        }

        //-----------------------------------------------------------
        SmartPtr<IParticleSystem> BillboardRenderer::getParticleSystem() const
        {
            return m_particleSystem;
        }

        //-----------------------------------------------------------
        void BillboardRenderer::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
        {
            m_particleSystem = particleSystem;
        }

        //-----------------------------------------------------------
        void BillboardRenderer::setMaterialName( const String &materialName, s32 index /*= -1 */ )
        {
            m_bbSet->setMaterialName( materialName, index );
        }

        //-----------------------------------------------------------
        String BillboardRenderer::getMaterialName( s32 index /*= -1 */ ) const
        {
            return m_bbSet->getMaterialName( index );
        }

        //-----------------------------------------------------------
        void BillboardRenderer::setParentSceneNode( SmartPtr<IGraphicsSceneNode> parentSceneNode )
        {
            m_parentSceneNode = parentSceneNode;

            if( m_parentSceneNode )
            {
                m_bbSet->setOwner( m_parentSceneNode );
            }
        }

        //-----------------------------------------------------------
        SmartPtr<IGraphicsSceneNode> BillboardRenderer::getParentSceneNode() const
        {
            return m_parentSceneNode;
        }

        //-----------------------------------------------------------
        template <typename T>
        void BillboardRenderer::ParticleUpdate<T>::operator()( T &it ) const
        {
            SmartPtr<IParticle> particle = ( it );

            auto particleData = static_cast<ParticleData *>( particle->getData() );
            WP_ASSERT( particleData );

            auto bb = static_cast<IBillboard *>( particleData->m_graphicsData );
            WP_ASSERT( bb );

            bb->setPosition( particleData->m_currentState->m_position );
            bb->setScale( particleData->m_currentState->m_scale *
                          m_renderer->m_particleSystem->getScale() );

            const Vector4F &colour = particleData->m_currentState->m_colour;
            bb->setColour( ColourF( colour[0], colour[1], colour[2], colour[3] ) );
        }

        //-----------------------------------------------------------
        template <typename T>
        BillboardRenderer::ParticleUpdate<T>::ParticleUpdate( BillboardRenderer *renderer ) :
            m_renderer( renderer )
        {
        }
    }  // namespace render
}  // namespace workphone
