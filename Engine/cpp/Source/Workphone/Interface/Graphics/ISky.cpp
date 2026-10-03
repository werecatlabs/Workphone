#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ISky.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, ISky, ISharedObject );

    ISky::ISky() : ISharedObject( ISky::typeInfo() )
    {
    }

    ISky::~ISky() = default;

}  // namespace workphone::render
