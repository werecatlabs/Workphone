#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ICoroutineData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ICoroutineData, ISharedObject );

    ICoroutineData::ICoroutineData() : ISharedObject( ICoroutineData::typeInfo() )
    {
    }

    ICoroutineData::~ICoroutineData() = default;
}  // namespace workphone
