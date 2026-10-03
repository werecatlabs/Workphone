#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/Vertex.hpp>

namespace workphone
{

    Vertex::Vertex() = default;

    auto Vertex::isFinite() const -> bool
    {
        return MathF::isFinite( position.lengthSquared() ) &&
               MathF::isFinite( normal.lengthSquared() ) &&
               MathF::isFinite( texCoord.lengthSquared() ) &&
               MathF::isFinite( texCoord1.lengthSquared() );
    }

}  // namespace workphone
