#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/ViewportStateData.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ViewportStateData, StateData );

    ViewportStateData::ViewportStateData() : StateData( ViewportStateData::typeInfo() )
    {
    }

    ViewportStateData::~ViewportStateData()
    {
        unload( nullptr );
    }

    void ViewportStateData::unload( SmartPtr<ISharedObject> data )
    {
        camera = nullptr;
        texture = nullptr;
        backgroundTexture = nullptr;
        StateData::unload( data );
    }

}  // namespace workphone
