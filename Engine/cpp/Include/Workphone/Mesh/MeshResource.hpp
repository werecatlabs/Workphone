#ifndef CMeshResource__H
#define CMeshResource__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{

    /**
     * @class MeshResource
     * @brief Implements a mesh resource, providing access and management for mesh data and related
     * properties.
     *
     * This class encapsulates mesh data, LOD levels, skeleton information, animation, visibility, and
     * other mesh-related properties. It provides methods for loading, saving, importing, and managing
     * mesh resources, as well as controlling various mesh features.
     */
    class MeshResource : public Resource<IMeshResource>
    {
    public:
        static const String scaleStr;
        static const String constraintsStr;
        static const String animationStr;
        static const String visibilityStr;
        static const String camerasStr;
        static const String lightsStr;
        static const String lightmapUVsStr;
        static const String useMeshInstancingStr;
        static const String saveStr;
        static const String importStr;

        /**
         * @brief Default constructor.
         *
         * Initializes a new MeshResource instance with default values.
         */
        MeshResource();

        /**
         * @brief Destructor.
         *
         * Cleans up the MeshResource instance.
         */
        ~MeshResource() override;

        /**
         * @brief Saves the mesh resource to persistent storage.
         *
         * Implements the IResource::save interface.
         */
        void save() override;

        /**
         * @brief Imports mesh data from an external source.
         *
         * Implements the IResource::import interface.
         */
        void import() override;

        /**
         * @brief Loads the mesh resource from the given data.
         * @param data Shared pointer to the data used for loading.
         *
         * Implements the IResource::load interface.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Reloads the mesh resource from the given data.
         * @param data Shared pointer to the data used for reloading.
         *
         * Implements the IResource::reload interface.
         */
        void reload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unloads the mesh resource, releasing associated data.
         * @param data Shared pointer to the data used for unloading.
         *
         * Implements the IResource::unload interface.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Checks if the mesh has an associated skeleton.
         * @return True if the mesh has a skeleton, false otherwise.
         */
        bool hasSkeleton() const;

        /**
         * @brief Gets the name of the skeleton associated with the mesh.
         * @return The skeleton name as a String.
         */
        String getSkeletonName() const;

        /**
         * @brief Gets the number of LOD (Level of Detail) levels for the mesh.
         * @return The number of LOD levels.
         */
        u32 getNumLodLevels() const;

        /**
         * @brief Checks if the edge list for the mesh has been built.
         * @return True if the edge list is built, false otherwise.
         */
        bool isEdgeListBuilt() const;

        /**
         * @brief Checks if the mesh has vertex animation data.
         * @return True if vertex animation is present, false otherwise.
         */
        bool hasVertexAnimation() const;

        /**
         * @brief Gets the properties of the mesh resource.
         * @return Shared pointer to the Properties object.
         *
         * Implements the IResource::getProperties interface.
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Sets the properties of the mesh resource.
         * @param properties Shared pointer to the Properties object.
         *
         * Implements the IResource::setProperties interface.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        /**
         * @brief Gets the underlying mesh object pointer.
         * @param ppObject Double pointer to receive the mesh object.
         *
         * Implements the IMeshResource::_getObject interface.
         */
        void _getObject( void **ppObject ) const override;

        /**
         * @brief Gets the scale factor applied to the mesh.
         * @return The scale as a floating-point value.
         */
        f32 getScale() const override;

        /**
         * @brief Sets the scale factor for the mesh.
         * @param scale The scale value to set.
         */
        void setScale( f32 scale ) override;

        /**
         * @brief Gets the material naming convention used for the mesh.
         * @return The MaterialNaming enum value.
         */
        MaterialNaming getMaterialNaming() const override;

        /**
         * @brief Sets the material naming convention for the mesh.
         * @param materialNaming The MaterialNaming enum value to set.
         */
        void setMaterialNaming( MaterialNaming materialNaming ) override;

        /**
         * @brief Gets whether constraints are enabled for the mesh.
         * @return True if constraints are enabled, false otherwise.
         */
        bool getConstraints() const override;

        /**
         * @brief Sets whether constraints are enabled for the mesh.
         * @param constraints True to enable constraints, false to disable.
         */
        void setConstraints( bool constraints ) override;

        /**
         * @brief Gets whether animation is enabled for the mesh.
         * @return True if animation is enabled, false otherwise.
         */
        bool getAnimation() const override;

        /**
         * @brief Sets whether animation is enabled for the mesh.
         * @param animation True to enable animation, false to disable.
         */
        void setAnimation( bool animation ) override;

        /**
         * @brief Gets the visibility state of the mesh.
         * @return True if the mesh is visible, false otherwise.
         */
        bool getVisibility() const override;

        /**
         * @brief Sets the visibility state of the mesh.
         * @param visibility True to make the mesh visible, false to hide it.
         */
        void setVisibility( bool visibility ) override;

        /**
         * @brief Gets whether cameras are enabled for the mesh.
         * @return True if cameras are enabled, false otherwise.
         */
        bool getCameras() const override;

        /**
         * @brief Sets whether cameras are enabled for the mesh.
         * @param cameras True to enable cameras, false to disable.
         */
        void setCameras( bool cameras ) override;

        /**
         * @brief Gets whether lights are enabled for the mesh.
         * @return True if lights are enabled, false otherwise.
         */
        bool getLights() const override;

        /**
         * @brief Sets whether lights are enabled for the mesh.
         * @param lights True to enable lights, false to disable.
         */
        void setLights( bool lights ) override;

        /**
         * @brief Gets whether lightmap UVs are enabled for the mesh.
         * @return True if lightmap UVs are enabled, false otherwise.
         */
        bool getLightmapUVs() const override;

        /**
         * @brief Sets whether lightmap UVs are enabled for the mesh.
         * @param lightmapUVs True to enable lightmap UVs, false to disable.
         */
        void setLightmapUVs( bool lightmapUVs ) override;

        bool getUseMeshInstancing() const override;

        void setUseMeshInstancing( bool useMeshInstancing ) override;

        /**
         * @brief Gets the mesh object associated with this resource.
         * @return Shared pointer to the IMesh object.
         */
        SmartPtr<IMesh> getMesh() const override;

        /**
         * @brief Sets the mesh object for this resource.
         * @param mesh Shared pointer to the IMesh object to set.
         */
        void setMesh( SmartPtr<IMesh> mesh ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The mesh object associated with this resource.
        AtomicSmartPtr<IMesh> m_mesh;

        /// The material naming convention used for the mesh.
        AtomicValue<MaterialNaming> m_materialNaming = MaterialNaming::MaterialName;

        /// The scale factor applied to the mesh.
        atomic_f32 m_scale = 1.0f;

        /// The number of LOD (Level of Detail) levels for the mesh.
        atomic_u32 m_numLodLevels = 0;

        /// True if the edge list for the mesh has been built.
        atomic_bool m_edgeListBuilt = false;

        /// True if the mesh has vertex animation data.
        atomic_bool m_hasVertexAnimation = false;

        /// True if constraints are enabled for the mesh.
        atomic_bool m_constraints = true;

        /// True if animation is enabled for the mesh.
        atomic_bool m_animation = true;

        /// True if the mesh is visible.
        atomic_bool m_visibility = true;

        /// True if cameras are enabled for the mesh.
        atomic_bool m_cameras = true;

        /// True if lights are enabled for the mesh.
        atomic_bool m_lights = true;

        /// True if lightmap UVs are enabled for the mesh.
        atomic_bool m_lightmapUVs = false;

        /// True if identical scene meshes should share one imported mesh resource.
        atomic_bool m_useMeshInstancing = false;

        /// True if the mesh has shared vertex data.
        atomic_bool m_hasSharedVertexData = false;

        /// True if the mesh has an associated skeleton.
        atomic_bool m_hasSkeleton = false;

        /// The name of the skeleton associated with the mesh.
        FixedString<256> m_skeletonName;
    };
}  // namespace workphone

#endif
