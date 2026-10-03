#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Billboards.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Billboards, Component );

    Billboards::Billboards()
    {
        setEventTaskFlags( Thread::Render_Flag );
    }

    Billboards::~Billboards() = default;

    void Billboards::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto sceneManager = applicationManager->getGameManager();

            if( auto scene = sceneManager->getCurrentScene() )
            {
                if( auto graphicsScene = graphicsSystem->getGraphicsScene() )
                {
                    // Create billboard set
                    //m_billboardSet = graphicsScene->addBillboardSet("BillboardSet");
                    if( m_billboardSet )
                    {
                        // Set default properties
                        m_billboardSet->setCullIndividually( true );
                        m_billboardSet->setSortingEnabled( true );
                        m_billboardSet->setUseAccurateFacing( true );
                        m_billboardSet->setDefaultDimensions( Vector3<real_Num>( 1.0f, 1.0f, 1.0f ) );
                    }
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Billboards::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            if( m_billboardSet )
            {
                m_billboardSet = nullptr;
            }

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Billboards::update()
    {
        if( Thread::getTaskFlag( Thread::Render_Flag ) )
        {
            if( auto actor = getActor() )
            {
                if( auto transform = actor->getTransform() )
                {
                    auto position = transform->getPosition();
                    auto orientation = transform->getOrientation();
                    auto scale = transform->getScale();

                    if( m_billboardSet )
                    {
                        // Update billboard set transform
                        //m_billboardSet->setPosition( position );
                        //m_billboardSet->setOrientation( orientation );
                        //m_billboardSet->setScale( scale );
                    }
                }
            }
        }
    }

    SmartPtr<render::IBillboardSet> Billboards::getBillboardSet() const
    {
        return m_billboardSet;
    }

    void Billboards::setBillboardSet( SmartPtr<render::IBillboardSet> billboardSet )
    {
        m_billboardSet = billboardSet;
    }

}  // namespace workphone::scene
