#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/VehicleGeneratorTypes.hpp>
#include <algorithm>
namespace workphone::procedural
{
    bool VehicleMeshSection::hasGeometry() const
    {
        return !vertices.empty() && indices.size() >= 3u && indices.size() % 3u == 0u;
    }
    bool VehicleGeometry::hasGeometry() const
    {
        return !lods.empty() && !lods.front().sections.empty();
    }
    const VehicleMaterialDescriptor &VehicleAppearanceBundle::material( VehicleMaterialSlot slot ) const
    {
        return materials.at( static_cast<size_t>( slot ) );
    }
    VehicleMaterialDescriptor &VehicleAppearanceBundle::material( VehicleMaterialSlot slot )
    {
        return materials.at( static_cast<size_t>( slot ) );
    }
    bool GeneratedVehicle::isValid() const noexcept
    {
        return std::none_of( issues.begin(), issues.end(),
                             []( const VehicleGenerationIssue &issue ) { return issue.error; } );
    }
}  // namespace workphone::procedural
