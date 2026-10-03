#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IVertexElement, ISharedObject );

    IVertexElement::IVertexElement() : ISharedObject( IVertexElement::typeInfo() )
    {
    }

    IVertexElement::~IVertexElement() = default;
}  // namespace workphone
