#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IVertexBuffer, ISharedObject );

    IVertexBuffer::IVertexBuffer() : ISharedObject( IVertexBuffer::typeInfo() )
    {
    }

    IVertexBuffer::~IVertexBuffer() = default;
}  // namespace workphone
