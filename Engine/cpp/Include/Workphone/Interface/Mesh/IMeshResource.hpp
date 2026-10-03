#ifndef IMeshResource_h__
#define IMeshResource_h__

#include <Workphone/Interface/System/IResource.hpp>

namespace workphone
{

    /**
     * Interface for a mesh resource.
     */
    class WPCore_API IMeshResource : public IResource
    {
    public:
        /**
         * Enumeration for different types of material naming.
         */
        enum class MaterialNaming : u8
        {
            MaterialName,
            TextureName,
            FileNameMaterial,

            Count
        };

        IMeshResource();

        IMeshResource( u32 poolTypeId );

        /** Destructor. */
        ~IMeshResource() override;

        /** Get the scaling factor for the mesh.
         * @return The scaling factor.
         */
        virtual f32 getScale() const = 0;

        /** Set the scaling factor for the mesh.
         * @param scale The scaling factor to set.
         */
        virtual void setScale( f32 scale ) = 0;

        /** Get the current material naming convention. */
        virtual MaterialNaming getMaterialNaming() const = 0;

        /** Set the material naming convention. */
        virtual void setMaterialNaming( MaterialNaming materialNaming ) = 0;

        /** Check if there are any constraints on the mesh. */
        virtual bool getConstraints() const = 0;

        /** Set the constraints on the mesh. */
        virtual void setConstraints( bool constraints ) = 0;

        /** Check if there is any animation on the mesh.
         * @return True if there is animation, false otherwise.
         */
        virtual bool getAnimation() const = 0;

        /** Set the animation flag for the mesh.
         * @param animation The flag to set.
         */
        virtual void setAnimation( bool animation ) = 0;

        /** Check if the mesh is visible.
         * @return True if the mesh is visible, false otherwise.
         */
        virtual bool getVisibility() const = 0;

        /** Set the visibility flag for the mesh.
         * @param visibility The flag to set.
         */
        virtual void setVisibility( bool visibility ) = 0;

        /** Check if there are any cameras attached to the mesh. */
        virtual bool getCameras() const = 0;

        /** Set the cameras flag for the mesh.
         * @param cameras The flag to set.
         */
        virtual void setCameras( bool cameras ) = 0;

        /** Check if there are any lights attached to the mesh. */
        virtual bool getLights() const = 0;

        /** Set the lights flag for the mesh.
         * @param lights The flag to set.
         */
        virtual void setLights( bool lights ) = 0;

        /** Check if there are any lightmap UVs in the mesh. */
        virtual bool getLightmapUVs() const = 0;

        /** Set the lightmap UVs flag for the mesh.
         * @param lightmapUVs The flag to set.
         */
        virtual void setLightmapUVs( bool lightmapUVs ) = 0;

        /** Check whether identical scene meshes should share one imported mesh resource. */
        virtual bool getUseMeshInstancing() const = 0;

        /** Set whether identical scene meshes should share one imported mesh resource. */
        virtual void setUseMeshInstancing( bool useMeshInstancing ) = 0;

        /** Get a smart pointer to the mesh object. */
        virtual SmartPtr<IMesh> getMesh() const = 0;

        /** Set a smart pointer to the mesh object.
         * @param mesh The mesh object to set.
         */
        virtual void setMesh( SmartPtr<IMesh> mesh ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IMeshResource_h__
