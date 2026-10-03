#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderTextureOgreNext.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CRenderTextureOgreNext,
                               CRenderTargetOgreNext<RenderTexture> );

    CRenderTextureOgreNext::CRenderTextureOgreNext()
    {
        createStateObject();
    }

    CRenderTextureOgreNext::~CRenderTextureOgreNext()
    {
    }

    void CRenderTextureOgreNext::createStateObject()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto factoryManager = applicationManager->getFactoryManager();

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        WP_ASSERT( getStateContext() == nullptr );

        auto stateContext = stateManager->addStateContext();
        WP_ASSERT( stateContext );
        setStateContext( stateContext );
        stateContext->setOwner( this );

        auto stateListener = workphone::make_ptr<RenderTargetListener>();
        stateListener->setOwner( this );
        stateContext->addStateListener( stateListener );
        setStateListener( stateListener );

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );

        auto textureState = workphone::make_ptr<TextureStateData>();
        state->setData( textureState );

        auto rtState = factoryManager->make_ptr<State>();
        stateContext->addState( rtState );

        auto rtStateData = workphone::make_ptr<RenderTargetStateData>();
        rtState->setData( rtStateData );

        auto rttState = factoryManager->make_ptr<State>();
        stateContext->addState( rttState );

        auto rttStateData = workphone::make_ptr<RenderTextureState>();
        rttState->setData( rttStateData );

        auto stateTask = graphicsSystem->getStateTask();
        stateContext->setTaskId( stateTask );
    }

    bool CRenderTextureOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool CRenderTextureOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        auto stateData = state->getData();
        if( stateData->isDerived<RenderTargetStateData>() )
        {
            auto renderTargetState = workphone::static_pointer_cast<RenderTargetStateData>( stateData );

            if( auto texture = getTexture() )
            {
                auto size = texture->getSize();
                renderTargetState->size = size;

                return true;
            }
        }

        return false;
    }

}  // namespace workphone::render
