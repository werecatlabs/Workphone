#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IThreadPool, ISharedObject );

    IThreadPool::~IThreadPool() = default;
}  // namespace workphone
