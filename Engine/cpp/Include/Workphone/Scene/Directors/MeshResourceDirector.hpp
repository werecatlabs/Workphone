#ifndef MeshResourceDirector_h__
#define MeshResourceDirector_h__

#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class MeshResourceDirector
         * @brief Director that governs mesh import/build settings.
         *
         * MeshResourceDirector exposes a variety of import-time and build-time options
         * for mesh resources such as scaling, axis corrections, normal/tangent generation,
         * triangulation and other preprocessing flags. These options are serialized via
         * the Properties API and used by the asset pipeline when preparing meshes for
         * runtime consumption.
         */
        class WPCore_API MeshResourceDirector : public ResourceDirector
        {
        public:
            /** Property key for scale factor applied to imported meshes. */
            static const String scaleStr;

            /** Property key for whether constraints (bones/weights) are enabled. */
            static const String constraintsStr;

            /** Property key for enabling animation data import. */
            static const String animationStr;

            /** Property key for whether the mesh is visible by default. */
            static const String visibilityStr;

            /** Property key for importing camera objects from the source file. */
            static const String camerasStr;

            /** Property key to swap Z and Y axes during import. */
            static const String swapZYStr;

            /** Property key to rotate the mesh 90 degrees around the X axis. */
            static const String rotate90DegreesXStr;

            /** Property key for importing light objects from the source file. */
            static const String lightsStr;

            /** Property key for generating lightmap UV coordinates. */
            static const String lightmapUVsStr;

            /** Property key for generating normals if they are missing. */
            static const String generateNormalsStr;

            /** Property key for generating smooth normals across smoothing groups. */
            static const String generateSmoothNormalsStr;

            /** Property key for generating tangent vectors required for normal mapping. */
            static const String generateTangentsStr;

            /** Property key for generating secondary UV coordinates. */
            static const String genUVCoordsStr;

            /** Property key for forcing triangulation of polygons. */
            static const String triangulateStr;

            /** Property key for reversing imported face index order. */
            static const String flipWindingOrderStr;

            /** Property key for joining identical vertices to reduce mesh size. */
            static const String joinIdenticalVerticesStr;

            /** Property key indicating the mesh uses shared vertex data. */
            static const String hasSharedVertexDataStr;

            /** Property key indicating the mesh uses instancing. */
            static const String useMeshInstancingStr;

            /** Property group containing progressive mesh import defaults. */
            static const String progressiveMeshOptionsStr;

            /**
             * @brief Construct a MeshResourceDirector with sensible defaults.
             */
            MeshResourceDirector();

            /**
             * @brief Destructor.
             */
            ~MeshResourceDirector() override;

            /**
             * @brief Return a Properties object describing current import/build options.
             * @copydoc IBuildDirector::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply import/build options from the provided Properties object.
             * @copydoc IBuildDirector::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get uniform scale applied to the imported mesh geometry.
             * @return Scale multiplier (default: 1.0).
             */
            f32 getScale() const;

            /**
             * @brief Set uniform scale to apply to imported mesh geometry.
             * @param scale Scale multiplier to apply during import.
             */
            void setScale( f32 scale );

            /**
             * @brief Query whether constraint/bone data should be imported.
             * @return True if constraints are imported (default: true).
             */
            bool getConstraints() const;

            /**
             * @brief Enable or disable importing of constraint/bone data.
             * @param constraints True to import constraints, false to ignore them.
             */
            void setConstraints( bool constraints );

            /**
             * @brief Query whether mesh visibility objects should be imported.
             * @return True if visibility is respected (default: true).
             */
            bool getVisibility() const;

            /**
             * @brief Set whether visibility flags in the source file should be imported.
             * @param visibility True to respect source visibility, false to ignore.
             */
            void setVisibility( bool visibility );

            /**
             * @brief Query whether animation data should be imported.
             * @return True if animation import is enabled (default: true).
             */
            bool getAnimation() const;

            /**
             * @brief Enable or disable importing of animation data.
             * @param animation True to import animations, false to skip them.
             */
            void setAnimation( bool animation );

            /**
             * @brief Query whether camera objects should be imported from the mesh file.
             * @return True if camera import is enabled (default: true).
             */
            bool getCameras() const;

            /**
             * @brief Enable or disable importing cameras from the source file.
             * @param cameras True to import cameras, false to ignore them.
             */
            void setCameras( bool cameras );

            /**
             * @brief Query whether to swap Z and Y axes during import (useful for different coordinate
             * conventions).
             * @return True if axes are swapped (default: false).
             */
            bool getSwapZY() const;

            /**
             * @brief Enable or disable swapping Z and Y axes during import.
             * @param swapZY True to swap Z and Y axes.
             */
            void setSwapZY( bool swapZY );

            /**
             * @brief Query whether to rotate the mesh 90 degrees around the X axis on import.
             * @return True if rotation is enabled (default: false).
             */
            bool getRotate90DegreesX() const;

            /**
             * @brief Enable or disable a 90-degree X rotation during import.
             * @param rotate90DegreesX True to apply the rotation.
             */
            void setRotate90DegreesX( bool rotate90DegreesX );

            /**
             * @brief Query whether light objects embedded in the source should be imported.
             * @return True if lights are imported (default: true).
             */
            bool getLights() const;

            /**
             * @brief Enable or disable importing of light objects from the source file.
             * @param lights True to import lights, false to ignore them.
             */
            void setLights( bool lights );

            /**
             * @brief Query whether to generate lightmap UVs for the mesh.
             * @return True if lightmap UV generation is enabled (default: false).
             */
            bool getLightmapUVs() const;

            /**
             * @brief Enable or disable generation of lightmap UV coordinates.
             * @param lightmapUVs True to generate lightmap UVs.
             */
            void setLightmapUVs( bool lightmapUVs );

            /**
             * @brief Query whether normals should be generated if missing.
             * @return True if normal generation is enabled (default: false).
             */
            bool getGenNormals() const;

            /**
             * @brief Enable or disable generation of vertex normals when absent.
             * @param genNormals True to generate normals.
             */
            void setGenNormals( bool genNormals );

            /**
             * @brief Query whether smooth normals should be generated across smoothing groups.
             * @return True if smooth normal generation is enabled (default: false).
             */
            bool getGenSmoothNormals() const;

            /**
             * @brief Enable or disable smooth normal generation.
             * @param genSmoothNormals True to generate smooth normals.
             */
            void setGenSmoothNormals( bool genSmoothNormals );

            /**
             * @brief Query whether tangent vectors should be generated (required for normal mapping).
             * @return True if tangent generation is enabled (default: false).
             */
            bool getGenTangents() const;

            /**
             * @brief Enable or disable tangent generation.
             * @param genTangents True to generate tangents.
             */
            void setGenTangents( bool genTangents );

            /**
             * @brief Query whether secondary UV coordinates should be generated.
             * @return True if generation of additional UVs is enabled (default: false).
             */
            bool getGenUVCoords() const;

            /**
             * @brief Enable or disable generation of additional UV coordinates.
             * @param genUVCoords True to generate extra UV sets.
             */
            void setGenUVCoords( bool genUVCoords );

            /**
             * @brief Query whether polygons should be triangulated during import.
             * @return True if triangulation is enabled (default: false).
             */
            bool getTriangulate() const;

            /**
             * @brief Enable or disable triangulation of polygonal faces.
             * @param triangulate True to force triangulation.
             */
            void setTriangulate( bool triangulate );

            /** Whether imported face index order is reversed. Default: false. */
            bool getFlipWindingOrder() const;

            /** Reverse imported winding without changing vertex positions or normals. */
            void setFlipWindingOrder( bool flipWindingOrder );

            /**
             * @brief Query whether identical vertices should be merged to reduce vertex count.
             * @return True if joining is enabled (default: false).
             */
            bool getJoinIdenticalVertices() const;

            /**
             * @brief Enable or disable joining of identical vertices.
             * @param joinIdenticalVertices True to merge identical vertices.
             */
            void setJoinIdenticalVertices( bool joinIdenticalVertices );

            /**
             * @brief Query whether the mesh uses shared vertex data (index/vertex sharing).
             * @return True if shared vertex data is used (default: false).
             */
            bool hasSharedVertexData() const;

            /**
             * @brief Indicate whether the mesh uses shared vertex data.
             * @param hasSharedVertexData True if shared vertex data should be used.
             */
            void setHasSharedVertexData( bool hasSharedVertexData );

            bool getUseMeshInstancing() const;

            void setUseMeshInstancing( bool useMeshInstancing );

            const ProgressiveMeshOptions &getProgressiveMeshOptions() const;

            void setProgressiveMeshOptions( const ProgressiveMeshOptions &options );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Reverse imported face indices. Default: false. */
            bool m_flipWindingOrder = false;

            /** Uniform scale applied to imported geometry. Default: 1.0f. */
            f32 m_scale = 1.0f;

            /** Import constraint/bone data. Default: true. */
            bool m_constraints = true;

            /** Import animation data. Default: true. */
            bool m_animation = true;

            /** Respect visibility flags from the source file. Default: true. */
            bool m_visibility = true;

            /** Import camera objects from the source file. Default: true. */
            bool m_cameras = true;

            /** Swap Z and Y axes during import. Default: false. */
            bool m_swapZY = false;

            /** Rotate the mesh 90 degrees around X during import. Default: false. */
            bool m_rotate90DegreesX = false;

            /** Import light objects. Default: true. */
            bool m_lights = true;

            /** Generate UVs suitable for lightmapping. Default: false. */
            bool m_lightmapUVs = false;

            /** Force triangulation of faces. Default: false. */
            bool m_triangulate = false;

            /** Generate normals when absent. Default: false. */
            bool m_genNormals = false;

            /** Generate smooth normals across smoothing groups. Default: false. */
            bool m_genSmoothNormals = false;

            /** Generate tangent vectors for normal mapping. Default: false. */
            bool m_genTangents = false;

            /** Generate additional UV coordinate sets. Default: false. */
            bool m_genUVCoords = false;

            /** Join identical vertices to reduce vertex count. Default: false. */
            bool m_joinIdenticalVertices = false;

            /** Whether the mesh uses shared vertex/index data. Default: false. */
            bool m_hasSharedVertexData = false;

            /** Whether to use mesh instancing for rendering. Default: false. */
            bool m_useMeshInstancing = false;

            /** Progressive mesh defaults copied to components created by this import. */
            ProgressiveMeshOptions m_progressiveMeshOptions;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // MeshResourceDirector_h__
