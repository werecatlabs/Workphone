#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IInputConverter.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IInputConverter, ISharedObject );

    IInputConverter::~IInputConverter() = default;

}  // namespace workphone
