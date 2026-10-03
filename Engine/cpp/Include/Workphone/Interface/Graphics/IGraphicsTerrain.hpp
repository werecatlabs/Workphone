#ifndef ITerrain_h__
#define ITerrain_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a renderable terrain object in the graphics engine.
         *
         * Provides methods for manipulating and querying terrain properties, such as height data,
         * textures, visibility, and material. This interface is intended to be implemented by concrete
         * terrain classes within the rendering system.
         */
        class WPCore_API IGraphicsTerrain : public ISharedObject
        {
        public:
            /**
             * @brief Types of textures that can be associated with the terrain.
             */
            enum class TextureTypes
            {
                DIFFUSE,           /**< Diffuse texture. */
                DETAIL_WEIGHT,     /**< Detail weight map. */
                DETAIL0,           /**< Detail texture 0. */
                DETAIL1,           /**< Detail texture 1. */
                DETAIL2,           /**< Detail texture 2. */
                DETAIL3,           /**< Detail texture 3. */
                DETAIL0_NM,        /**< Normal map for detail 0. */
                DETAIL1_NM,        /**< Normal map for detail 1. */
                DETAIL2_NM,        /**< Normal map for detail 2. */
                DETAIL3_NM,        /**< Normal map for detail 3. */
                DETAIL_ROUGHNESS0, /**< Roughness map for detail 0. */
                DETAIL_ROUGHNESS1, /**< Roughness map for detail 1. */
                DETAIL_ROUGHNESS2, /**< Roughness map for detail 2. */
                DETAIL_ROUGHNESS3, /**< Roughness map for detail 3. */
                DETAIL_METALNESS0, /**< Metalness map for detail 0. */
                DETAIL_METALNESS1, /**< Metalness map for detail 1. */
                DETAIL_METALNESS2, /**< Metalness map for detail 2. */
                DETAIL_METALNESS3, /**< Metalness map for detail 3. */
                REFLECTION,        /**< Reflection texture. */
                RESERVED0,         /**< Reserved for future use. */
                RESERVED1,         /**< Reserved for future use. */
                RESERVED2,         /**< Reserved for future use. */
                RESERVED3,         /**< Reserved for future use. */
                RESERVED4,         /**< Reserved for future use. */
                COUNT              /**< Number of texture types. */
            };

            IGraphicsTerrain();

            /**
             * @brief Virtual destructor.
             */
            ~IGraphicsTerrain() override;

            /**
             * @brief Gets the world transform of the terrain.
             * @return The world transform of the terrain.
             */
            virtual Transform3<real_Num> getWorldTransform() const = 0;

            /**
             * @brief Sets the world transform of the terrain.
             * @param worldTransform The new world transform to set.
             */
            virtual void setWorldTransform( const Transform3<real_Num> &worldTransform ) = 0;

            /**
             * @brief Gets the position of the terrain in world space.
             * @return A Vector3 representing the position of the terrain.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Sets the position of the terrain in world space.
             * @param position The new position to set.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Gets the height of the terrain at a given world position.
             * @param position The world position to query for the height.
             * @return The height of the terrain at the specified position.
             */
            virtual f32 getHeightAtWorldPosition( const Vector3<real_Num> &position ) const = 0;

            /**
             * @brief Gets the size (width/length) of the terrain.
             * @return The size of the terrain.
             */
            virtual u16 getSize() const = 0;

            /**
             * @brief Converts a world space position to terrain space.
             * @param worldSpace The world space position to convert.
             * @return The corresponding terrain space position.
             */
            virtual Vector3<real_Num> getTerrainSpacePosition(
                const Vector3<real_Num> &worldSpace ) const = 0;

            /**
             * @brief Gets the height data of the terrain.
             * @return An Array containing the height data values.
             */
            virtual Array<f32> getHeightData() const = 0;

            /**
             * @brief Sets the height data for the terrain.
             * @param heightData The new height data to set.
             */
            virtual void setHeightData( const Array<f32> &heightData ) = 0;

            /**
             * @brief Checks if the terrain is visible.
             * @return True if the terrain is visible, false otherwise.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Sets the visibility of the terrain.
             * @param visible True to make the terrain visible, false to hide it.
             */
            virtual void setVisible( bool visible ) = 0;

            /**
             * @brief Checks if the terrain is rendered as a wireframe.
             * @return True if wireframe mode is enabled, false otherwise.
             */
            virtual bool getShowWireframe() const = 0;

            /**
             * @brief Sets whether the terrain should be rendered as a wireframe.
             * @param showWireframe True to enable wireframe mode, false to disable.
             */
            virtual void setShowWireframe( bool showWireframe ) = 0;

            /**
             * @brief Gets the name of the terrain's material.
             * @return The material name as a String.
             */
            virtual String getMaterialName() const = 0;

            /**
             * @brief Sets the name of the terrain's material.
             * @param materialName The new material name to set.
             */
            virtual void setMaterialName( const String &materialName ) = 0;

            /**
             * @brief Gets a terrain layer blend map by index.
             * @param index The index of the blend map to retrieve.
             * @return A SmartPtr to the requested blend map.
             */
            virtual SmartPtr<ITerrainBlendMap> getBlendMap( u32 index ) = 0;

            /**
             * @brief Gets the size of the terrain's layer blend map.
             * @return The blend map size.
             */
            virtual u16 getLayerBlendMapSize() const = 0;

            /**
             * @brief Casts a ray against the terrain and returns intersection information.
             * @param ray The ray to cast.
             * @return A SmartPtr to an ITerrainRayResult containing intersection info, or nullptr if no
             * intersection.
             */
            virtual SmartPtr<ITerrainRayResult> intersects( const Ray3F &ray ) const = 0;

            /**
             * @brief Gets the terrain represented as a mesh.
             * @return A SmartPtr to an IMesh containing the mesh data.
             */
            virtual SmartPtr<IMesh> getMesh() const = 0;

            /**
             * @brief Gets the size of the height map used for the terrain.
             * @return A 2d vector representing the width and height of the height map.
             */
            virtual Vector2I getHeightMapSize() const = 0;

            /**
             * @brief Sets the size of the height map used for the terrain.
             * @param heightMapSize A 2d vector representing the new width and height of the height map.
             */
            virtual void setHeightMapSize( const Vector2I &heightMapSize ) = 0;

            /**
             * @brief Gets the height scale factor for the terrain.
             * @return The height scale value.
             */
            virtual f32 getHeightScale() const = 0;

            /**
             * @brief Sets the height scale factor for the terrain.
             * @param heightScale The new height scale value.
             */
            virtual void setHeightScale( f32 heightScale ) = 0;

            /**
             * @brief Gets the scene manager associated with the terrain.
             * @return A SmartPtr to the scene manager.
             */
            virtual SmartPtr<IGraphicsScene> getSceneManager() const = 0;

            /**
             * @brief Sets the scene manager for the terrain.
             * @param sceneManager A SmartPtr to the new scene manager.
             */
            virtual void setSceneManager( SmartPtr<IGraphicsScene> sceneManager ) = 0;

            /**
             * @brief Gets a pointer to the underlying graphics object (API dependent).
             * @param ppObject Pointer to the pointer that will receive the graphics object.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /**
             * @brief Gets the height map texture for the terrain.
             * @return A SmartPtr to the height map texture.
             */
            virtual SmartPtr<ITexture> getHeightMap() const = 0;

            /**
             * @brief Sets the height map texture for the terrain.
             * @param heightMap A SmartPtr to the new height map texture.
             */
            virtual void setHeightMap( SmartPtr<ITexture> heightMap ) = 0;

            /**
             * @brief Sets a texture for a specific layer.
             * @param layer The layer index.
             * @param textureName The name of the texture to set.
             */
            virtual void setTextureLayer( s32 layer, const String &textureName ) = 0;

            /**
             * @brief Gets all texture layers associated with the terrain.
             * @return An Array of SmartPtrs to the texture layers.
             */
            virtual Array<SmartPtr<ITexture>> getTextures() const = 0;

            /**
             * @brief Sets the texture layers for the terrain.
             * @param textures An Array of SmartPtrs to the texture layers.
             */
            virtual void setTextures( const Array<SmartPtr<ITexture>> &textures ) = 0;

            /**
             * @brief Gets a texture by index.
             * @param index The index of the texture to retrieve.
             * @return A SmartPtr to the requested texture.
             */
            virtual SmartPtr<ITexture> getTexture( u32 index ) const = 0;

            /**
             * @brief Sets a texture by index.
             * @param index The index of the texture to set.
             * @param texture A SmartPtr to the new texture.
             */
            virtual void setTexture( u32 index, SmartPtr<ITexture> texture ) = 0;

            /**
             * @brief Updates the terrain's material, applying any changes to textures or properties.
             */
            virtual void updateMaterial() = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // ITerrain_h__
