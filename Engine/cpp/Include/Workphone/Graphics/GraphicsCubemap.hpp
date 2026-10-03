#ifndef __WP_Render_Cubemap_h__
#define __WP_Render_Cubemap_h__

#include <Workphone/Interface/Graphics/IGraphicsCubemap.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Concrete implementation of a cubemap graphics object.
         *
         * This class implements the `ICubemap` interface and represents a
         * cubemap texture used for environment mapping, skyboxes or reflection
         * probes. It is a shared graphics object (managed through the engine's
         * SmartPtr system) and exposes properties such as the underlying texture
         * name, owning scene manager, visibility/exclusion masks, world position,
         * enabled state and update interval. A list of excluded objects can be
         * maintained so specified scene objects are not included when the cubemap
         * is rendered/updated.
         */
        class WPCore_API GraphicsCubemap : public SharedGraphicsObject<IGraphicsCubemap>
        {
        public:
            /**
             * @brief Construct a new Cubemap instance.
             *
             * Initializes the cubemap object to default values. Concrete graphics
             * systems will perform lazy initialization when the cubemap is bound
             * to a scene or a graphics resource.
             */
            GraphicsCubemap();

            /**
             * @brief Destroy the Cubemap instance.
             *
             * Releases any graphics resources owned by the cubemap. Implementations
             * should ensure proper cleanup in the graphics backend.
             */
            ~GraphicsCubemap() override;

            /**
             * @brief Get the texture resource name for this cubemap.
             *
             * The texture name typically corresponds to an engine resource identifier
             * (for example a material/texture asset name or GPU resource name).
             *
             * @return String The name of the cubemap texture resource.
             */
            String getTextureName() const override;

            /**
             * @brief Set the texture resource name for this cubemap.
             *
             * @param textureName The resource identifier to use for the cubemap texture.
             */
            void setTextureName( const String &textureName ) override;

            /**
             * @brief Get the scene manager that owns or manages this cubemap.
             *
             * The scene manager provides context for scene-wide operations, resource
             * lookup and update scheduling. This will return a SmartPtr to the
             * associated `IGraphicsScene` or a null SmartPtr if none is set.
             *
             * @return SmartPtr<IGraphicsScene> The scene manager for this cubemap.
             */
            SmartPtr<IGraphicsScene> getSceneManager() const override;

            /**
             * @brief Set the scene manager for this cubemap.
             *
             * Assigning a scene manager ties the cubemap to a particular graphics
             * scene context and may be required for updates or scene-specific
             * resource lookups.
             *
             * @param smgr SmartPtr<IGraphicsScene> Scene manager to associate.
             */
            void setSceneManager( SmartPtr<IGraphicsScene> smgr ) override;

            /**
             * @brief Get the visibility mask for objects affected by this cubemap.
             *
             * Visibility masks are used to filter which scene objects are visible
             * to or affected by the cubemap (for example during rendering or updates).
             *
             * @return u32 The visibility mask value.
             */
            u32 getVisibilityMask() const override;

            /**
             * @brief Set the visibility mask for the cubemap.
             *
             * @param visibilityMask Bitmask used to control which objects are considered visible.
             */
            void setVisibilityMask( u32 visibilityMask ) override;

            /**
             * @brief Get the exclusion mask for objects that should be ignored.
             *
             * The exclusion mask works together with the visibility mask to determine
             * which objects are explicitly excluded from cubemap rendering or updates.
             *
             * @return u32 The exclusion mask value.
             */
            u32 getExclusionMask() const override;

            /**
             * @brief Set the exclusion mask for this cubemap.
             *
             * @param exclusionMask Bitmask specifying object categories to exclude.
             */
            void setExclusionMask( u32 exclusionMask ) override;

            /**
             * @brief Get the world-space position of the cubemap.
             *
             * For localized reflection probes or dynamic cubemaps, the position
             * determines where the environment sampling occurs. For skybox-style
             * cubemaps the position is often the camera or origin.
             *
             * @return Vector3<real_Num> The cubemap's position in world coordinates.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Set the world-space position of the cubemap.
             *
             * @param position Position in world coordinates where the cubemap is sampled.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Get whether the cubemap is currently enabled.
             *
             * When disabled, the cubemap should not be used for rendering or updates.
             *
             * @return bool True if the cubemap is enabled; false otherwise.
             */
            bool getEnable() const override;

            /**
             * @brief Enable or disable the cubemap.
             *
             * @param enable True to enable the cubemap; false to disable it.
             */
            void setEnable( bool enable ) override;

            /**
             * @brief Get the automatic update interval for the cubemap.
             *
             * The update interval controls how frequently (in milliseconds) the
             * cubemap content is refreshed when using dynamic updates. A value of 0
             * typically indicates no automatic updates.
             *
             * @return u32 Update interval in milliseconds.
             */
            u32 getUpdateInterval() const override;

            /**
             * @brief Set the automatic update interval for the cubemap.
             *
             * @param milliseconds Interval in milliseconds between automatic updates.
             *                     A value of 0 disables automatic updates.
             */
            void setUpdateInterval( u32 milliseconds ) override;

            /**
             * @brief Add a graphics object to the cubemap's exclusion list.
             *
             * Excluded objects will be ignored when the cubemap is rendered or
             * when dynamic updates are performed (for example to avoid self-reflection).
             * Passing a null SmartPtr has no effect.
             *
             * @param object SmartPtr<IGraphicsObject> The object to exclude.
             */
            void addExcludedObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @brief Retrieve the list of objects excluded from cubemap updates.
             *
             * The returned array contains SmartPtr handles to graphics objects that
             * will be ignored during cubemap rendering or updates.
             *
             * @return Array<SmartPtr<IGraphicsObject>> Array of excluded graphics objects.
             */
            Array<SmartPtr<IGraphicsObject>> getExcludedObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_textureName;
            SmartPtr<IGraphicsScene> m_sceneManager;
            u32 m_visibilityMask = 0;
            u32 m_exclusionMask = 0;
            Vector3<real_Num> m_position;
            bool m_enable = false;
            u32 m_updateInterval = 0;
            Array<SmartPtr<IGraphicsObject>> m_excludedObjects;
        };
    }  // namespace render
}  // namespace workphone

#endif  // __WP_Render_Cubemap_h__
