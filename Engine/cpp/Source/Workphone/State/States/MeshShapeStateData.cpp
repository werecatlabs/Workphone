#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/MeshShapeStateData.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MeshShapeStateData, StateData );

    MeshShapeStateData::MeshShapeStateData() : StateData( MeshShapeStateData::typeInfo() )
    {
    }

    MeshShapeStateData::~MeshShapeStateData() = default;

}  // namespace workphone
