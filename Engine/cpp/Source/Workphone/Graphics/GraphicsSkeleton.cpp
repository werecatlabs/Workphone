#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsSkeleton.hpp>
#include <Workphone/Interface/Graphics/IGraphicsBone.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsSkeleton,
                               SharedGraphicsObject<IGraphicsSkeleton> );

    GraphicsSkeleton::~GraphicsSkeleton()
    {
    }

    GraphicsSkeleton::GraphicsSkeleton()
    {
    }

    SmartPtr<IGraphicsBone> GraphicsSkeleton::createBone()
    {
        return nullptr;
    }

    SmartPtr<IGraphicsBone> GraphicsSkeleton::createBone( u32 handle )
    {
        return nullptr;
    }

    SmartPtr<IGraphicsBone> GraphicsSkeleton::createBone( const String &name )
    {
        return nullptr;
    }

    SmartPtr<IGraphicsBone> GraphicsSkeleton::createBone( const String &name, u32 handle )
    {
        return nullptr;
    }
}  // namespace workphone::render
