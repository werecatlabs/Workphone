#ifndef DecalCursor_h__
#define DecalCursor_h__

#include <Workphone/Interface/Graphics/IDecalCursor.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief A runtime cursor represented as a decal projected onto the world/terrain.
         *
         * The `DecalCursor` provides a simple interface to control a projected decal that can
         * be used as an editor or gameplay cursor (placement indicator, brush preview, etc.).
         *
         * The implementation owns or references rendering resources through `SmartPtr` types.
         * All methods are lightweight accessors/mutators and are safe to call from the main
         * rendering thread. Lifetime and threading guarantees follow the owning renderer's rules.
         *
         * @see IDecalCursor
         */
        class WPCore_API DecalCursor : public SharedGraphicsObject<IDecalCursor>
        {
        public:
            /**
             * @brief Construct a new DecalCursor.
             *
             * Initializes internal state to defaults (invisible, zero position/size).
             */
            DecalCursor();

            /**
             * @brief Destroy the DecalCursor.
             *
             * Releases any held rendering references. Destruction should occur on the same
             * thread/context that owns rendering resources.
             */
            ~DecalCursor();

            /**
             * @brief Query whether the decal cursor is currently visible.
             * @return true if visible, false otherwise.
             */
            bool isVisible() const override;

            /**
             * @brief Set the visibility of the decal cursor.
             * @param visible Pass true to show the decal cursor, false to hide it.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Get the world-space position of the decal cursor.
             * @return The current `Vector3<real_Num>` position of the decal projection center.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Set the world-space position of the decal cursor.
             * @param position The new `Vector3<real_Num>` position for the decal projection center.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Get the size of the decal projection.
             * @return A `Vector2<real_Num>` describing width and height of the decal area.
             */
            Vector2<real_Num> getSize() const override;

            /**
             * @brief Set the size of the decal projection.
             * @param size A `Vector2<real_Num>` specifying width and height to project on the surface.
             */
            void setSize( const Vector2<real_Num> &size ) override;

            /**
             * @brief Get the terrain material used when projecting the decal onto terrain.
             * @return SmartPtr to an `IMaterial` instance, or null if none is set.
             */
            SmartPtr<IMaterial> getTerrainMaterial() const override;

            /**
             * @brief Set the terrain material used for decal projection.
             * @param material SmartPtr to an `IMaterial` instance. Passing a null `SmartPtr`
             *                 clears the terrain material.
             */
            void setTerrainMaterial( SmartPtr<IMaterial> material ) override;

            /**
             * @brief Get the name of the texture used for the decal.
             * @return The texture resource name (engine-specific string) for the decal texture.
             */
            String getDecalTextureName() const override;

            /**
             * @brief Set the texture resource name used for the decal.
             * @param textureName Resource identifier for the decal texture.
             */
            void setDecalTextureName( const String &textureName ) override;

            /**
             * @brief Get the runtime texture object used for the decal.
             * @return SmartPtr to an `ITexture` instance, or null if not set.
             */
            SmartPtr<ITexture> getDecalTexture() const override;

            /**
             * @brief Set the runtime texture object used for the decal.
             * @param texture SmartPtr to an `ITexture` instance. Passing a null `SmartPtr`
             *                clears the current decal texture reference.
             */
            void setDecalTexture( SmartPtr<ITexture> texture ) override;

            /**
             * @brief Add a debug entity to visualize the decal cursor in the scene.
             *
             * This is intended for editor/debug use: it creates/attaches a visible entity
             * identified by `entityName` and scaled by `scale`. Implementations may reuse a
             * cached debug entity when called multiple times.
             *
             * @param entityName Name of the debug entity resource/mesh to instantiate.
             * @param scale Optional uniform/non-uniform scale to apply to the debug entity.
             */
            void addDebugEntity( const String &entityName,
                                 const Vector3<real_Num> &scale = Vector3<real_Num>::unit() ) override;

            /**
             * @brief Remove any previously added debug entity.
             *
             * If no debug entity is present this is a no-op. Removal should free any
             * temporary rendering resources created by `addDebugEntity`.
             */
            void removeDebugEntity() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace render
}  // namespace workphone

#endif  // DecalCursor_h__
