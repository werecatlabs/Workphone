#pragma once
#include <Workphone/Scene/Components/Component.hpp>
namespace workphone::scene
{
    /// Shared lifecycle for CPU-generated meshes. Regeneration is deferred until attached.
    class WPCore_API ProceduralMeshComponent : public Component
    {
    public:
        ProceduralMeshComponent();
        ~ProceduralMeshComponent() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        void setActor( SmartPtr<IGameActor> actor ) override;
        bool regenerate();
        void requestRegeneration()
        {
            m_dirty = true;
        }
        const String &getGenerationError() const
        {
            return m_error;
        }
        WP_CLASS_REGISTER_DECL;

    protected:
        virtual SmartPtr<IMesh> buildMesh() = 0;
        virtual void applyGeneratedMaterials()
        {
        }
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

    private:
        bool m_dirty = true;
        bool m_ownsMesh = false;
        bool m_ownsRenderer = false;
        String m_error;
        SmartPtr<Mesh> m_meshComponent;
        SmartPtr<MeshRenderer> m_renderer;
        SmartPtr<IMeshResource> m_resource;
        SmartPtr<IMeshResource> m_previousResource;
        String m_previousPath;
    };
}  // namespace workphone::scene
