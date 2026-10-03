#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IStream, ISharedObject );

    IStream::IStream() : ISharedObject( IStream::typeInfo() )
    {
    }

    IStream::~IStream() = default;
}  // namespace workphone
