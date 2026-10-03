#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/GraphicsObjectListenerOgreNext.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsObjectListenerOgreNext, IStateListener );

    GraphicsObjectListenerOgreNext::GraphicsObjectListenerOgreNext() = default;

    GraphicsObjectListenerOgreNext::~GraphicsObjectListenerOgreNext() = default;

    void GraphicsObjectListenerOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    void GraphicsObjectListenerOgreNext::setOwner( SmartPtr<IGraphicsObject> owner )
    {
        m_owner = owner;
    }

    SmartPtr<IGraphicsObject> GraphicsObjectListenerOgreNext::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    bool GraphicsObjectListenerOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = this->getOwnerPtr() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    bool GraphicsObjectListenerOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = this->getOwnerPtr() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

}  // namespace workphone::render
