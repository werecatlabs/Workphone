#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IBone, ISharedObject );

    IBone::~IBone() = default;
}  // namespace workphone
