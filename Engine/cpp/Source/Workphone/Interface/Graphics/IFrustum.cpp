#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IFrustum.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFrustum, IGraphicsObject );

    IFrustum::IFrustum() : IGraphicsObject( IFrustum::typeInfo() )
    {
    }

    IFrustum::IFrustum( u32 poolTypeId ) : IGraphicsObject( poolTypeId )
    {
    }

    IFrustum::~IFrustum() = default;

}  // namespace workphone::render
