#ifndef IDecalCursor_h__
#define IDecalCursor_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for an in-world decal placement cursor.
         *
         * The decal cursor represents an interactive in-world widget used to preview
         * and place decals (projected textures) onto scene geometry (for example,
         * terrain or other meshes). Implementations are responsible for rendering
         * a visual representation, tracking position/size and managing decal-related
         * resources such as the decal texture and terrain material.
         *
         * Ownership notes:
         * - Methods that return a SmartPtr\<T\> return a shared pointer to the resource.
         * - Setter methods that take SmartPtr\<T\> accept shared ownership.
         *
         * Thread-safety:
         * - No thread-safety guarantees are specified by this interface. Callers
         *   should ensure access is serialized if the implementation is used from
         *   multiple threads.
         */
        class WPCore_API IDecalCursor : public ISharedObject
        {
        public:
            /** @brief Virtual destructor. */
            ~IDecalCursor() override;

            /**
             * @brief Query whether the cursor is currently visible.
             * @return true if the cursor is visible and should be rendered, false otherwise.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Set the cursor visibility.
             * @param visible true to show the cursor, false to hide it.
             *
             * Implementations should update internal rendering state and any
             * debug/preview entities accordingly.
             */
            virtual void setVisible( bool visible ) = 0;

            /**
             * @brief Get the world-space position of the cursor.
             * @return 3D position in world coordinates.
             *
             * The position is typically the hit location on geometry where the decal
             * would be projected.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Set the world-space position of the cursor.
             * @param position New world-space position for the cursor.
             *
             * Implementations should update the preview/decal projection origin to
             * reflect the new position.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Get the cursor size (width, height).
             * @return 2D size vector representing the decal's size in world units or
             *         other implementation-defined units.
             */
            virtual Vector2<real_Num> getSize() const = 0;

            /**
             * @brief Set the cursor size.
             * @param size New 2D size for the cursor (width, height).
             *
             * The interpretation of size (world units, pixels, etc.) is implementation-defined;
             * consumers should consult the concrete implementation.
             */
            virtual void setSize( const Vector2<real_Num> &size ) = 0;

            /**
             * @brief Retrieve the material used for terrain decal rendering.
             * @return Shared pointer to the terrain material, or null if none is set.
             *
             * The returned SmartPtr may be used to configure material parameters
             * related to decal blending and projection.
             */
            virtual SmartPtr<IMaterial> getTerrainMaterial() const = 0;

            /**
             * @brief Set the material used when applying decals to terrain.
             * @param material Shared pointer to an IMaterial to use for terrain decals.
             */
            virtual void setTerrainMaterial( SmartPtr<IMaterial> material ) = 0;

            /**
             * @brief Get the name of the decal texture resource.
             * @return String containing the texture resource name or an empty string if unset.
             *
             * The name typically corresponds to an asset identifier used by the
             * resource/graphics system.
             */
            virtual String getDecalTextureName() const = 0;

            /**
             * @brief Set the decal texture by resource name.
             * @param textureName Name or identifier of the decal texture resource.
             *
             * Implementations will usually resolve this name to an ITexture and update
             * the preview/projection.
             */
            virtual void setDecalTextureName( const String &textureName ) = 0;

            /**
             * @brief Get the decal texture object.
             * @return Shared pointer to the ITexture used for the decal, or null if none.
             *
             * Use this to directly access texture properties or to replace the texture
             * with a different ITexture instance.
             */
            virtual SmartPtr<ITexture> getDecalTexture() const = 0;

            /**
             * @brief Set the decal texture object directly.
             * @param texture Shared pointer to an ITexture to use for the decal preview and placement.
             */
            virtual void setDecalTexture( SmartPtr<ITexture> texture ) = 0;

            /**
             * @brief Add a debug entity used to visualise the cursor.
             * @param entityName Name or prototype identifier of the debug entity to add.
             * @param scale Optional scale for the debug entity. Defaults to unit scale.
             *
             * Debug entities are typically lightweight visual helpers (e.g. a mesh or sprite)
             * used to indicate the decal projection orientation/extent. Implementations should
             * ensure only one debug entity is present per cursor or manage multiple appropriately.
             */
            virtual void addDebugEntity( const String &entityName, const Vector3<real_Num> &scale =
                                                                       Vector3<real_Num>::unit() ) = 0;

            /**
             * @brief Remove the debug entity previously added with addDebugEntity.
             *
             * Implementations should safely handle cases where no debug entity exists.
             */
            virtual void removeDebugEntity() = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IDecalCursor_h__
