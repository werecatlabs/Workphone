#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IPrototype.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::core
{
    WP_CLASS_REGISTER_DERIVED( workphone::core, IPrototype, ISharedObject );

    IPrototype::IPrototype( u32 typeId ) : ISharedObject( typeId )
    {
    }

    IPrototype::IPrototype()
    {
    }

    IPrototype::~IPrototype() = default;
}  // namespace workphone::core
