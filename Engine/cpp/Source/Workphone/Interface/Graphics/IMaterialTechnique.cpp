#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMaterialTechnique.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IMaterialTechnique, IMaterialNode );

    IMaterialTechnique::~IMaterialTechnique() = default;

}  // namespace workphone::render
