#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Memory/ISharedObjectListener.hpp>

namespace workphone
{

    ISharedObjectListener::ISharedObjectListener() : ISharedObject( ISharedObjectListener::typeInfo() )
    {
    }

    ISharedObjectListener::~ISharedObjectListener() = default;

}  // namespace workphone
