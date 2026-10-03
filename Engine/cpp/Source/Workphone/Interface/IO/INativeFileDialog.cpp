#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IO/INativeFileDialog.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, INativeFileDialog, ISharedObject );

    INativeFileDialog::~INativeFileDialog() = default;
}  // namespace workphone
