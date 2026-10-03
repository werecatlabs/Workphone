#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMeshConverter.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IMeshConverter, ISharedObject );

    IMeshConverter::~IMeshConverter() = default;

}  // namespace workphone::render
