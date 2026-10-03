#ifndef ICubemap_h__
#define ICubemap_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a cubemap object.
         */
        class WPCore_API IGraphicsCubemap : public ISharedObject
        {
        public:
            enum class ProjectionMode : u32
            {
                Infinite = 0,
                Box = 1
            };

            enum class InfluenceShape : u32
            {
                Sphere = 0,
                Box = 1
            };

            /**
             * @brief Controls whether the renderer refreshes continuously or only after
             * a request.
             */
            enum class UpdateMode : u32
            {
                Automatic = 0,
                OnDemand = 1
            };

            /**
             * @brief Controls how the faces in a cubemap refresh are distributed across
             * frames.
             */
            enum class TimeSlicingMode : u32
            {
                AllFacesAtOnce = 0,
                OneFacePerFrame = 1
            };

            static constexpr u32 FaceAll = ( 1u << 6u ) - 1u;

            IGraphicsCubemap();

            /**
             * @brief Destructor.
             */
            ~IGraphicsCubemap() override;

            /**
             * @brief Get the name of the texture.
             * @return The texture name.
             */
            virtual String getTextureName() const = 0;

            /**
             * @brief Set the name of the texture.
             * @param textureName The new texture name.
             */
            virtual void setTextureName( const String &textureName ) = 0;

            /**
             * @brief Get the texture produced by this cubemap.
             * @return A texture wrapper for the cubemap, or null if it is not ready.
             */
            virtual SmartPtr<ITexture> getTexture() const;

            /**
             * @brief Get whether this cubemap should be considered for automatic material binding.
             */
            virtual bool getAutoApplyToMaterials() const;

            /**
             * @brief Set whether this cubemap should be considered for automatic material binding.
             */
            virtual void setAutoApplyToMaterials( bool autoApply );

            /**
             * @brief Get the projection mode used by the cubemap.
             */
            virtual ProjectionMode getProjectionMode() const;

            /**
             * @brief Set the projection mode used by the cubemap.
             */
            virtual void setProjectionMode( ProjectionMode projectionMode );

            /**
             * @brief Get the world-space influence shape.
             */
            virtual InfluenceShape getInfluenceShape() const;

            /**
             * @brief Set the world-space influence shape.
             */
            virtual void setInfluenceShape( InfluenceShape influenceShape );

            /**
             * @brief Get the spherical influence radius.
             */
            virtual f32 getSphereRadius() const;

            /**
             * @brief Set the spherical influence radius.
             */
            virtual void setSphereRadius( f32 sphereRadius );

            /**
             * @brief Get half extents for box influence.
             */
            virtual Vector3<real_Num> getBoxExtents() const;

            /**
             * @brief Set half extents for box influence.
             */
            virtual void setBoxExtents( const Vector3<real_Num> &boxExtents );

            /**
             * @brief Get the blend distance used at the influence boundary.
             */
            virtual f32 getBlendDistance() const;

            /**
             * @brief Set the blend distance used at the influence boundary.
             */
            virtual void setBlendDistance( f32 blendDistance );

            /**
             * @brief Get the priority weight used when multiple cubemaps affect a material.
             */
            virtual f32 getImportance() const;

            /**
             * @brief Set the priority weight used when multiple cubemaps affect a material.
             */
            virtual void setImportance( f32 importance );

            /**
             * @brief Get the cubemap intensity metadata.
             */
            virtual f32 getIntensity() const;

            /**
             * @brief Set the cubemap intensity metadata.
             */
            virtual void setIntensity( f32 intensity );

            /**
             * @brief Get whether distance can disable this cubemap for material binding.
             */
            virtual bool getAutoEnableByDistance() const;

            /**
             * @brief Set whether distance can disable this cubemap for material binding.
             */
            virtual void setAutoEnableByDistance( bool autoEnableByDistance );

            /**
             * @brief Get the maximum distance used when distance auto-enable is active.
             */
            virtual f32 getEnableDistanceThreshold() const;

            /**
             * @brief Set the maximum distance used when distance auto-enable is active.
             */
            virtual void setEnableDistanceThreshold( f32 distanceThreshold );

            /**
             * @brief Get the scene manager.
             * @return A smart pointer to the scene manager.
             */
            virtual SmartPtr<IGraphicsScene> getSceneManager() const = 0;

            /**
             * @brief Set the scene manager.
             * @param smgr A smart pointer to the scene manager.
             */
            virtual void setSceneManager( SmartPtr<IGraphicsScene> smgr ) = 0;

            /**
             * @brief Get the visibility mask.
             * @return The visibility mask.
             */
            virtual u32 getVisibilityMask() const = 0;

            /**
             * @brief Set the visibility mask.
             * @param visibilityMask The new visibility mask.
             */
            virtual void setVisibilityMask( u32 visibilityMask ) = 0;

            /**
             * @brief Get the exclusion mask.
             * @return The exclusion mask.
             */
            virtual u32 getExclusionMask() const = 0;

            /**
             * @brief Set the exclusion mask.
             * @param exclusionMask The new exclusion mask.
             */
            virtual void setExclusionMask( u32 exclusionMask ) = 0;

            /**
             * @brief Get the position.
             * @return The position.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Set the position.
             * @param position The new position.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Get the enable state.
             * @return The enable state.
             */
            virtual bool getEnable() const = 0;

            /**
             * @brief Set the enable state.
             * @param enable The new enable state.
             */
            virtual void setEnable( bool enable ) = 0;

            /**
             * @brief Get the update interval.
             * @return The update interval in milliseconds.
             */
            virtual u32 getUpdateInterval() const = 0;

            /**
             * @brief Set the update interval.
             * @param milliseconds The new update interval in milliseconds.
             */
            virtual void setUpdateInterval( u32 milliseconds ) = 0;

            /**
             * @brief Get whether updates are automatic or explicitly requested.
 */
            virtual UpdateMode getUpdateMode() const;

            /**
             * @brief Select automatic or request-driven updates.
             */
            virtual void setUpdateMode( UpdateMode updateMode );

            /**
             * @brief Get how cubemap faces are distributed across render frames.
 */
            virtual TimeSlicingMode getTimeSlicingMode() const;

            /**
             * @brief Select whether all faces render together or one face renders per
             * frame.
             */
            virtual void setTimeSlicingMode( TimeSlicingMode timeSlicingMode );

            /**
             * @brief Get the six-bit mask of faces included in a refresh.
 */
            virtual u32 getFaceMask() const;

            /**
             * @brief Set the six-bit mask of faces included in a refresh.
 */
            virtual void setFaceMask( u32 faceMask );

            /**
             * @brief Queue a cubemap refresh when using UpdateMode::OnDemand.
 *

             * * Repeated requests may be coalesced by the renderer so a slow time-sliced
             *
             * refresh cannot accumulate an unbounded backlog.
             */
            virtual void requestUpdate();

            /**
             * @brief Return true while a requested refresh is queued or in progress.
 */
            virtual bool isUpdatePending() const;

            /**
             * @brief Add an excluded object.
             * @param object A smart pointer to the excluded object.
             */
            virtual void addExcludedObject( SmartPtr<IGraphicsObject> object ) = 0;

            /**
             * @brief Get the excluded objects.
             * @return An array of smart pointers to the excluded objects.
             */
            virtual Array<SmartPtr<IGraphicsObject>> getExcludedObjects() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ICubemap_h__
