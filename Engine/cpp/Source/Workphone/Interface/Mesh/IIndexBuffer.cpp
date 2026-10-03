#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IIndexBuffer, ISharedObject );

    IIndexBuffer::IIndexBuffer() : ISharedObject( IIndexBuffer::typeInfo() )
    {
    }

    IIndexBuffer::~IIndexBuffer() = default;
}  // namespace workphone
