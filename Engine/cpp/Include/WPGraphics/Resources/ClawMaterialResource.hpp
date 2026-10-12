#ifndef WPClawMaterialResource_h__
#define WPClawMaterialResource_h__

#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Database/CatalogResourceAdapter.hpp>

namespace workphone::render
{
    /** Render-owner bridge for one catalog material. All calls, and final releases
     * of returned handles, belong on the constructing render thread. Retain current()
     * through drawing; install its material on the mesh before renderMesh(). A staged
     * candidate never changes the visible bundle. SourceRoot must match ResourceSystem.
     */
    class WPGraphics_API ClawMaterialResource final
    {
    public:
        class WPGraphics_API Candidate final
        {
        public:
            ~Candidate();
            SmartPtr<ClawMaterial> material() const;
            SmartPtr<ClawTexture> texture() const;
            SmartPtr<ClawTexture> normalTexture() const;
            SmartPtr<ClawTexture> ormTexture() const;
            std::shared_ptr<const resource::RuntimeResource> resource() const;

        private:
            friend class ClawMaterialResource;
            struct Data;
            explicit Candidate( std::shared_ptr<Data> data );
            std::shared_ptr<Data> m_data;
        };

        ClawMaterialResource( SmartPtr<AssetDatabaseManager> catalog,
                              std::shared_ptr<resource::IResourceSystem> resources,
                              const String &sourceRoot,
                              const Array<CatalogResourceAdapter::TypeMapping> &mappings,
                              SmartPtr<ClawRendererDX11> renderer, const String &target = "pc",
                              bool packagedBuild = false );
        ~ClawMaterialResource();
        ClawMaterialResource( const ClawMaterialResource & ) = delete;
        ClawMaterialResource &operator=( const ClawMaterialResource & ) = delete;

        std::shared_ptr<const Candidate> stage( const String &uuid, String &error, bool force = false );
        /** Stage an already loaded package resource without a source catalog or compiler.
         * Uses the same typed dependency validation and transactional GPU construction. */
        std::shared_ptr<const Candidate> stageRuntime(
            std::shared_ptr<const resource::RuntimeResource> resource, String &error );
        bool publish( std::shared_ptr<const Candidate> candidate, String &error );
        bool reload( const String &uuid, String &error, bool force = false );
        std::shared_ptr<const Candidate> current() const;
        bool unload( String &error );

    private:
        std::shared_ptr<const Candidate> prepare( std::shared_ptr<Candidate::Data> candidate,
                                                 String &error );
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}  // namespace workphone::render

#endif
