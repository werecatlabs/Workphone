#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/RenderTargetListener.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>

namespace workphone::render
{

    RenderTargetListener::~RenderTargetListener() = default;

    RenderTargetListener::RenderTargetListener() = default;

    bool RenderTargetListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

    bool RenderTargetListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    SmartPtr<render::IRenderTarget> RenderTargetListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void RenderTargetListener::setOwner( SmartPtr<render::IRenderTarget> owner )
    {
        m_owner = owner;
    }

}  // namespace workphone::render
