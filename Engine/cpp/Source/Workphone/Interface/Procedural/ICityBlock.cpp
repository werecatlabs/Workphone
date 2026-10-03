#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ICityBlock.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, ICityBlock, IProceduralObject );

    ICityBlock::~ICityBlock() = default;

}  // namespace workphone::procedural
