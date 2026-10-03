#pragma once
#include <Workphone/Scene/Components/ProceduralMeshComponent.hpp>
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
namespace workphone::scene
{
    /// A local-space road segment with independently materialled render layers.
    class WPCore_API ProceduralRoad : public ProceduralMeshComponent
    {
    public:
        ProceduralRoad();
        ~ProceduralRoad() override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void setRoadSystem( SmartPtr<procedural::IRoadSystem> service )
        {
            m_service = service;
            requestRegeneration();
        }
        const procedural::RoadSegmentResult &getResult() const
        {
            return m_result;
        }
        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<IMesh> buildMesh() override;

    private:
        procedural::RoadSegmentSpec m_spec;
        procedural::RoadSegmentResult m_result;
        SmartPtr<procedural::IRoadSystem> m_service;
        Vector3<real_Num> m_end{ 0, 0, -20 };
    };
}  // namespace workphone::scene
