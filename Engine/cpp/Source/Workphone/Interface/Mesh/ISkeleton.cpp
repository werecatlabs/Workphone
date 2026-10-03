#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISkeleton, ISharedObject );

    ISkeleton::~ISkeleton() = default;
}  // namespace workphone
