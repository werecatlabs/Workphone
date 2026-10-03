#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IO/IArchive.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IArchive, ISharedObject );

    IArchive::IArchive() : ISharedObject( IArchive::typeInfo() )
    {
    }

    IArchive::~IArchive() = default;
}  // namespace workphone
