#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCubemap.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsCubemap, ISharedObject );

    IGraphicsCubemap::IGraphicsCubemap() : ISharedObject( IGraphicsCubemap::typeInfo() )
    {
    }

    IGraphicsCubemap::~IGraphicsCubemap() = default;

    auto IGraphicsCubemap::getTexture() const -> SmartPtr<ITexture>
    {
        return nullptr;
    }

    bool IGraphicsCubemap::getAutoApplyToMaterials() const
    {
        return false;
    }

    void IGraphicsCubemap::setAutoApplyToMaterials( bool autoApply )
    {
    }

    IGraphicsCubemap::ProjectionMode IGraphicsCubemap::getProjectionMode() const
    {
        return ProjectionMode::Box;
    }

    void IGraphicsCubemap::setProjectionMode( ProjectionMode projectionMode )
    {
    }

    IGraphicsCubemap::InfluenceShape IGraphicsCubemap::getInfluenceShape() const
    {
        return InfluenceShape::Sphere;
    }

    void IGraphicsCubemap::setInfluenceShape( InfluenceShape influenceShape )
    {
    }

    f32 IGraphicsCubemap::getSphereRadius() const
    {
        return 0.0f;
    }

    void IGraphicsCubemap::setSphereRadius( f32 sphereRadius )
    {
    }

    Vector3<real_Num> IGraphicsCubemap::getBoxExtents() const
    {
        return Vector3<real_Num>();
    }

    void IGraphicsCubemap::setBoxExtents( const Vector3<real_Num> &boxExtents )
    {
    }

    f32 IGraphicsCubemap::getBlendDistance() const
    {
        return 0.0f;
    }

    void IGraphicsCubemap::setBlendDistance( f32 blendDistance )
    {
    }

    f32 IGraphicsCubemap::getImportance() const
    {
        return 0.0f;
    }

    void IGraphicsCubemap::setImportance( f32 importance )
    {
    }

    f32 IGraphicsCubemap::getIntensity() const
    {
        return 1.0f;
    }

    void IGraphicsCubemap::setIntensity( f32 intensity )
    {
    }

    bool IGraphicsCubemap::getAutoEnableByDistance() const
    {
        return false;
    }

    void IGraphicsCubemap::setAutoEnableByDistance( bool autoEnableByDistance )
    {
    }

    f32 IGraphicsCubemap::getEnableDistanceThreshold() const
    {
        return 0.0f;
    }

    void IGraphicsCubemap::setEnableDistanceThreshold( f32 distanceThreshold )
    {
    }

    IGraphicsCubemap::UpdateMode IGraphicsCubemap::getUpdateMode() const
    {
        return UpdateMode::Automatic;
    }

    void IGraphicsCubemap::setUpdateMode( UpdateMode updateMode )
    {
    }

    IGraphicsCubemap::TimeSlicingMode IGraphicsCubemap::getTimeSlicingMode() const
    {
        return TimeSlicingMode::OneFacePerFrame;
    }

    void IGraphicsCubemap::setTimeSlicingMode( TimeSlicingMode timeSlicingMode )
    {
    }

    u32 IGraphicsCubemap::getFaceMask() const
    {
        return FaceAll;
    }

    void IGraphicsCubemap::setFaceMask( u32 faceMask )
    {
    }

    void IGraphicsCubemap::requestUpdate()
    {
    }

    bool IGraphicsCubemap::isUpdatePending() const
    {
        return false;
    }
}  // namespace workphone::render
