#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IVertexDeclaration, ISharedObject );

    IVertexDeclaration::IVertexDeclaration() : ISharedObject( IVertexDeclaration::typeInfo() )
    {
    }

    IVertexDeclaration::~IVertexDeclaration() = default;
}  // namespace workphone
