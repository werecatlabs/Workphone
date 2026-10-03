#ifndef _CCamera_H
#define _CCamera_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/GraphicsCamera.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Ogre-based implementation of the project's Camera interface.
         *
         * CCameraOgre wraps an underlying Ogre::Camera and adapts it to the project's
         * Camera / IGraphicsObject / IFrustum APIs. It exposes camera transform,
         * projection and frustum utilities, plus integration with viewports and
         * the graphics-object state system.
         *
         * Threading:
         * - Some cached transform fields are marked mutable to allow lazy updates
         *   in const accessors. Access to the underlying Ogre::Camera pointer
         *   is not guarded here; callers must obey the engine's threading model.
         *
         * @see Ogre::Camera
         */
        class CCameraOgre : public CGraphicsObjectOgre<GraphicsCamera>
        {
        public:
            /**
             * @brief Construct an empty Ogre camera wrapper.
             *
             * The constructor creates the wrapper object only. Actual Ogre objects
             * (m_camera) are created and configured during load().
             */
            CCameraOgre();

            /**
             * @brief Destructor. Unregisters listeners and releases engine resources.
             *
             * Ensure unload() has been called before destruction if the camera was loaded.
             */
            ~CCameraOgre() override;

            /**
             * @brief Initialize or configure this camera from shared data.
             *
             * Called by the engine to create/attach the underlying Ogre resources.
             *
             * @param data Optional initialization data (engine-specific).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Release resources and detach from rendering systems.
             *
             * Called by the engine when the camera should dispose of its render-time
             * resources. Implementations should null out m_camera after releasing it.
             *
             * @param data Optional unload context.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Update cached matrices/derived transforms from the underlying Ogre camera.
             *
             * Internal update used by the engine when the camera or scene changes.
             */
            void _update();

            /**
             * @brief Register or unregister this camera for per-frame updates.
             *
             * When registered the engine will call _update() each frame (or according to
             * the engine's update loop).
             *
             * @param registerObject True to register; false to unregister.
             */
            void registerForUpdates( bool registerObject );

            /**
             * @brief Query whether this camera is registered for updates.
             *
             * @return True if camera is currently registered for updates.
             */
            bool isRegisteredForUpdates();

            /**
             * @brief Associate this camera with a viewport.
             *
             * The camera will use the viewport to calculate aspect ratio, scissor/window
             * regions and for projecting rays/screen positions.
             *
             * @param viewport Smart pointer to the viewport to attach (may be null).
             */
            void setViewport( SmartPtr<IViewport> viewport ) override;

            /**
             * @brief Get the viewport this camera belongs to.
             *
             * @return Smart pointer to the associated viewport (may be null).
             */
            SmartPtr<IViewport> getViewport() const override;

            // IGraphicsObject functions

            /**
             * @brief Set the name of the material used by this graphics object.
             *
             * For cameras this may be unused, but present for interface completeness.
             *
             * @param materialName Material resource name.
             * @param index Optional sub-index for multi-material objects (-1 for default).
             */
            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            /**
             * @brief Get the material name assigned to this object.
             *
             * @param index Optional sub-index for multi-material objects (-1 for default).
             * @return The material name string (empty if none).
             */
            String getMaterialName( s32 index = -1 ) const override;

            /**
             * @brief Enable or disable shadow casting for objects controlled by this camera.
             *
             * The flag is stored locally and may be applied to associated renderables.
             *
             * @param castShadows True to enable casting shadows.
             */
            void setCastShadows( bool castShadows ) override;

            /**
             * @brief Query whether this camera is set to cast shadows.
             *
             * @return True if cast shadows is enabled.
             */
            bool getCastShadows() const override;

            /**
             * @brief Enable or disable receiving shadows for objects controlled by this camera.
             *
             * @param receiveShadows True to enable receiving shadows.
             */
            void setReceiveShadows( bool receiveShadows ) override;

            /**
             * @brief Query whether receiving shadows is enabled.
             *
             * @return True if receive shadows is enabled.
             */
            bool getReceiveShadows() const override;

            /**
             * @brief Set the render queue group for this camera's output (engine-specific).
             *
             * Often used to control rendering order of objects that belong to this camera.
             *
             * @param renderQueue The render-queue index to assign.
             */
            void setRenderQueueGroup( u8 renderQueue );

            /**
             * @brief Set visibility flags used to filter objects visible to this camera.
             *
             * Visibility flags are engine-defined bitmasks used by scene queries.
             *
             * @param flags Bitmask of visibility layers.
             */
            void setVisibilityFlags( u32 flags ) override;

            /**
             * @brief Get the currently configured visibility flags.
             *
             * @return Bitmask of visibility layers.
             */
            u32 getVisibilityFlags() const override;

            /**
             * @brief Create a shallow copy/clone of this graphics object.
             *
             * The returned object should be a new IGraphicsObject instance that duplicates
             * relevant camera settings (not necessarily an independent Ogre::Camera).
             *
             * @param name Optional name for the clone.
             * @return Smart pointer to the new IGraphicsObject.
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Get a raw pointer to the underlying native object.
             *
             * For Ogre integration this returns the pointer to the Ogre::Camera stored in
             * m_camera via the void** out parameter.
             *
             * @param ppObject Address of a void* to receive the native object pointer.
             */
            void _getObject( void **ppObject ) const override;

            //
            // IFrustum functions
            //

            /**
             * @brief Set the near clipping distance for the camera frustum.
             *
             * @param nearDist Distance to the near clipping plane in world units.
             */
            void setNearClipDistance( f32 nearDist ) override;

            /**
             * @brief Get the near clipping distance.
             *
             * @return The near clip distance in world units.
             */
            f32 getNearClipDistance() const override;

            /**
             * @brief Set the far clipping distance for the camera frustum.
             *
             * @param farDist Distance to the far clipping plane in world units.
             */
            void setFarClipDistance( f32 farDist ) override;

            /**
             * @brief Get the far clipping distance.
             *
             * @return The far clip distance in world units.
             */
            f32 getFarClipDistance() const override;

            /**
             * @brief Set the camera's aspect ratio explicitly.
             *
             * If auto aspect ratio is enabled, the viewport will override this value.
             *
             * @param ratio Aspect ratio (width / height).
             */
            void setAspectRatio( f32 ratio ) override;

            /**
             * @brief Get the camera's aspect ratio.
             *
             * @return Aspect ratio (width / height).
             */
            f32 getAspectRatio() const override;

            /**
             * @brief Test whether an axis-aligned bounding box is visible in this camera's frustum.
             *
             * @param bound The AABB to test in world space.
             * @return True if the bound intersects or is inside the frustum.
             */
            bool isObjectVisible( const AABB3F &bound ) const override;

            /**
             * @brief Test whether a bounding sphere is visible in this camera's frustum.
             *
             * @param bound The sphere to test in world space.
             * @return True if the sphere intersects or is inside the frustum.
             */
            bool isObjectVisible( const Sphere3F &bound ) const override;

            /**
             * @brief Test whether a point is visible in this camera's frustum.
             *
             * @param vert The point in world space.
             * @return True if the point is inside the frustum.
             */
            bool isObjectVisible( const Vector3F &vert ) const override;

            //
            // IGraphicsCamera functions
            //

            /**
             * @brief Set the camera world position.
             *
             * @param position New world-space position.
             */
            void setPosition( const Vector3F &position );

            /**
             * @brief Get the camera world position.
             *
             * @return World-space position.
             */
            Vector3F getPosition() const;

            /**
             * @brief Set the camera viewing direction vector.
             *
             * The input is expected to be a normalized direction.
             *
             * @param vec Direction vector in world space.
             */
            void setDirection( const Vector3F &vec );

            /**
             * @brief Get the camera viewing direction.
             *
             * @return Normalized forward direction vector in world space.
             */
            Vector3F getDirection() const override;

            /**
             * @brief Get the world-space ray direction that corresponds to a screen position.
             *
             * @param screenPosition Screen coordinates in normalized device coordinates or viewport space
             *                       (engine convention; see IViewport).
             * @return Normalized world direction for the given screen point.
             */
            Vector3F getDirection( const Vector2F &screenPosition ) const override;

            /**
             * @brief Get direction and world-space origin corresponding to a screen position.
             *
             * @param screenPosition Screen coordinates (engine convention).
             * @param worldPosition Output world-space origin of the ray (usually camera or near-plane position).
             * @return Normalized world direction.
             */
            Vector3F getDirection( const Vector2F &screenPosition,
                                   Vector3F &worldPosition ) const override;

            /**
             * @brief Set camera orientation as a quaternion.
             *
             * @param q Orientation quaternion (world space).
             */
            void setOrientation( const QuaternionF &q );

            /**
             * @brief Get the camera orientation quaternion.
             *
             * @return Orientation quaternion (world space).
             */
            QuaternionF getOrientation() const;

            /**
             * @brief Get the camera's up vector (world space).
             *
             * @return Normalized up vector.
             */
            Vector3F getUp() const override;

            /**
             * @brief Get the camera's right vector (world space).
             *
             * @return Normalized right vector.
             */
            Vector3F getRight() const override;

            /**
             * @brief Orient the camera to look at a point in world space.
             *
             * This modifies camera orientation such that the forward vector points at targetPoint.
             *
             * @param targetPoint World-space target to look at.
             */
            void lookAt( const Vector3F &targetPoint );

            /**
             * @brief Roll the camera around its forward axis.
             *
             * @param angle Angle in radians.
             */
            void roll( const f32 &angle );

            /**
             * @brief Yaw the camera around its up axis.
             *
             * @param angle Angle in radians.
             */
            void yaw( const f32 &angle );

            /**
             * @brief Pitch the camera around its right axis.
             *
             * @param angle Angle in radians.
             */
            void pitch( const f32 &angle );

            /**
             * @brief Rotate the camera around an arbitrary axis.
             *
             * @param axis Axis in world space.
             * @param angle Angle in radians.
             */
            void rotate( const Vector3F &axis, const f32 &angle );

            /**
             * @brief Rotate the camera by a quaternion.
             *
             * @param q Rotation quaternion to apply.
             */
            void rotate( const QuaternionF &q );

            /**
             * @brief Enable or disable a fixed yaw axis for the camera.
             *
             * When enabled, yaw rotations are constrained around the given axis.
             *
             * @param useFixed True to enable fixed yaw axis.
             * @param fixedAxis Axis to use when fixed yaw is enabled (defaults to +Y).
             */
            void setFixedYawAxis( bool useFixed, const Vector3F &fixedAxis = Vector3F::UNIT_Y );

            /**
             * @brief Get the camera's derived orientation (including reflections).
             *
             * @return Derived orientation quaternion.
             */
            QuaternionF getDerivedOrientation() const;

            /**
             * @brief Get the camera's derived position (including reflections).
             *
             * @return Derived world-space position.
             */
            Vector3F getDerivedPosition() const;

            /**
             * @brief Get derived forward direction (including reflections).
             *
             * @return Derived forward vector.
             */
            Vector3F getDerivedDirection() const;

            /**
             * @brief Get derived up vector (including reflections).
             *
             * @return Derived up vector.
             */
            Vector3F getDerivedUp() const;

            /**
             * @brief Get derived right vector (including reflections).
             *
             * @return Derived right vector.
             */
            Vector3F getDerivedRight() const;

            /**
             * @brief Get the real orientation (without reflection adjustments).
             *
             * @return Real orientation quaternion.
             */
            QuaternionF getRealOrientation() const;

            /**
             * @brief Get the real camera position (without reflection adjustments).
             *
             * @return Real position in world space.
             */
            Vector3F getRealPosition() const;

            /**
             * @brief Get the real forward direction (without reflection adjustments).
             *
             * @return Real forward vector.
             */
            Vector3F getRealDirection() const;

            /**
             * @brief Get the real up vector (without reflection adjustments).
             *
             * @return Real up vector.
             */
            Vector3F getRealUp() const;

            /**
             * @brief Get the real right vector (without reflection adjustments).
             *
             * @return Real right vector.
             */
            Vector3F getRealRight() const;

            /**
             * @brief Set the LOD bias multiplier used when selecting LODs.
             *
             * @param factor Bias multiplier (1.0 = default).
             */
            void setLodBias( f32 factor = 1.0 ) override;

            /**
             * @brief Get the LOD bias multiplier.
             *
             * @return Current LOD bias factor.
             */
            f32 getLodBias() const override;

            /**
             * @brief Return a world-space picking Ray from normalized screen coords.
             *
             * @param screenx Horizontal screen coordinate (engine convention).
             * @param screeny Vertical screen coordinate (engine convention).
             * @return The computed Ray in world space.
             */
            Ray3F getRay( f32 screenx, f32 screeny ) const override;

            /**
             * @brief Compute a world-space Ray and store it in outRay.
             *
             * @param screenx Horizontal screen coordinate.
             * @param screeny Vertical screen coordinate.
             * @param outRay Output ray to populate.
             */
            void getRay( f32 screenx, f32 screeny, Ray3F &outRay ) const;

            /**
             * @brief Project a world position into screen space (viewport coordinates).
             *
             * @param position World-space position.
             * @return Screen coordinates as a Vector2F (engine viewport convention).
             */
            Vector2F getScreenPosition( const Vector3F &position ) override;

            /**
             * @brief Set a custom projection window (scissor) for this camera.
             *
             * Coordinates are typically in normalized (0..1) viewport space. Use resetWindow()
             * to clear a custom window.
             *
             * @param left Left edge (0..1).
             * @param top Top edge (0..1).
             * @param right Right edge (0..1).
             * @param bottom Bottom edge (0..1).
             */
            void setWindow( f32 left, f32 top, f32 right, f32 bottom ) override;

            /**
             * @brief Reset any custom projection window to the full viewport.
             */
            void resetWindow();

            /**
             * @brief Query whether a custom projection window is set.
             *
             * @return True if a window was set via setWindow().
             */
            bool isWindowSet() const;

            /**
             * @brief Enable or disable automatic aspect ratio (driven by the viewport).
             *
             * @param autoratio True to compute aspect ratio from viewport dimensions.
             */
            void setAutoAspectRatio( bool autoratio ) override;

            /**
             * @brief Query whether automatic aspect ratio is enabled.
             *
             * @return True if auto aspect ratio is active.
             */
            bool getAutoAspectRatio() const override;

            /**
             * @brief Return the vertical field of view (radians).
             *
             * @return FOVy in radians.
             */
            f32 getFOVy() const override;

            /**
             * @brief Set the vertical field of view (radians).
             *
             * @param fov Vertical FOV in radians.
             */
            void setFOVy( f32 fov ) override;

            /**
             * @brief Get the camera's view matrix.
             *
             * @return 4x4 view matrix (world -> camera).
             */
            Matrix4F getViewMatrix() const override;

            /**
             * @brief Get the camera's projection matrix.
             *
             * @return 4x4 projection matrix.
             */
            Matrix4F getProjectionMatrix() const override;

            /**
             * @brief Get the native render view matrix pointer used by the renderer.
             *
             * The returned pointer is renderer-specific (Ogre) and should be treated as opaque.
             *
             * @return Void* pointer to renderer's view matrix.
             */
            void *getRenderViewMatrix() const override;

            /**
             * @brief Get the native render projection matrix pointer used by the renderer.
             *
             * @return Void* pointer to renderer's projection matrix.
             */
            void *getRenderProjectionMatrix() const override;

            /** @copydoc IGraphicsCamera::setRenderUI */
            void setRenderUI( bool enabled ) override;

            /** @copydoc IGraphicsCamera::getRenderUI */
            bool getRenderUI() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief State listener specialized for camera state messages.
             *
             * This nested listener bridges the engine state/message system with this camera
             * instance. It holds an AtomicSmartPtr to the owning camera to avoid dangling
             * raw pointers when the owner is destroyed asynchronously.
             */
            class CCameraStateListener : public GraphicsObjectOgreStateListener
            {
            public:
                CCameraStateListener();
                ~CCameraStateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 *
                 * Implementations should interpret messages and apply them to the camera.
                 *
                 * @param message Incoming state message.
                 * @return True if the message was handled and no further processing is required.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a full state change (replacement).
                 *
                 * @param state New state to apply.
                 * @return True if the state change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Set the owning camera for this listener.
                 *
                 * @param owner Smart pointer to the CCameraOgre that owns this listener.
                 */
                void setCamera( SmartPtr<CCameraOgre> owner );

                /**
                 * @brief Get the associated camera instance.
                 *
                 * @return Smart pointer to the owner camera (may be null).
                 */
                SmartPtr<CCameraOgre> getCamera() const;

            protected:
                /// Atomic smart pointer to the owning camera to avoid lifetime races.
                AtomicSmartPtr<CCameraOgre> m_owner;
            };

            /**
             * @brief Configure the graphics-object state integration for this camera.
             *
             * Called by the base class during initialization to attach state listeners etc.
             */
            void setupStateObject() override;

            /// The viewport this camera belongs to.
            SmartPtr<IViewport> m_viewport;

            /// Pointer to the underlying Ogre camera. nullptr when not loaded.
            Ogre::Camera *m_camera = nullptr;

            /// Camera position - default (0,0,0). Mutable so const getters can lazily update.
            mutable Vector3F m_position;

            /// Camera orientation, stored as a quaternion.
            mutable QuaternionF m_orientation;

            /// Cached forward direction.
            Vector3F m_direction;

            /// Real (non-derived) direction vector computed from underlying scene node.
            Vector3F m_realDirection;

            /// Real right vector computed from underlying scene node.
            Vector3F m_realRight;

            /// Real up vector computed from underlying scene node.
            Vector3F m_realUp;

            /// Cached inverse view-projection matrix for unprojecting screen positions.
            Matrix4F inverseVP;

            /// Derived orientation (e.g. with reflection) for this camera.
            mutable QuaternionF m_derivedOrientation;

            /// Derived camera position (e.g. with reflection).
            mutable Vector3F m_derivedPosition;

            /// Real world orientation of the camera (without derived/reflection adjustments).
            mutable QuaternionF m_realOrientation;

            /// Real world position of the camera (without derived/reflection adjustments).
            mutable Vector3F m_realPosition;

            /// The aspect ratio of the camera. Stored in an atomic floating type.
            atomic_f32 m_aspectRatio;

            /// Name of a material associated with this graphics object (engine-level usage).
            String m_materialName;

            /// Flags controlling shadow casting/receiving for objects associated with this camera.
            bool m_castShadows = false;

            /// Receive shadows.
            bool m_receiveShadows = false;

            /// Whether UI should be rendered by this camera.
            bool m_renderUI = false;

            /// Static extension used when generating unique camera names.
            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif
