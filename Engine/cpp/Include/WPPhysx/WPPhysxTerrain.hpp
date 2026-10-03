#ifndef WPPhysxTerrain_h__
#define WPPhysxTerrain_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <WPPhysx/WPPhysxShape.hpp>
#include <Workphone/Physics/TerrainShape.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <geometry/PxHeightFieldGeometry.h>
#include <PxSimpleTypes.h>
#include <PxQueryReport.h>

namespace workphone
{
    namespace physics
    {

        /**
         * @class PhysxTerrain
         * @brief PhysX-based implementation of a terrain collision shape.
         *
         * This class implements the `ITerrainShape` abstraction using NVIDIA PhysX primitives.
         * It owns and manages terrain-specific data such as the heightfield samples, terrain
         * dimensions and scaling, and constructs the underlying PhysX heightfield geometry
         * and shape objects used for collision and queries.
         *
         * Responsibilities:
         * - Store and provide access to height data, logical size and world size/scale.
         * - Create PhysX `PxHeightFieldGeometry` and `PxShape` instances representing the terrain.
         * - Maintain material and transform information for the terrain collider.
         *
         * Thread-safety:
         * - Instances may be accessed from multiple threads for read operations; mutation
         *   of internal state should be done through the public API and with proper external
         *   synchronization where required.
         *
         * @note The class derives from `PhysxShape<TerrainShape>` which provides common
         *       shape behaviour and integration with the engine's physics abstractions.
         *
         * @see ITerrainShape, PhysxShape
         */
        class PhysxTerrain : public PhysxShape<TerrainShape>
        {
        public:
            /** Default constructor. Initializes members to sensible defaults. */
            PhysxTerrain();

            /** Destructor. Releases PhysX resources owned by this object. */
            ~PhysxTerrain() override;

            /**
             * @brief Load/initialise the terrain from the provided shared object data.
             *
             * This should create any runtime PhysX objects required by the terrain (heightfield,
             * shape, material, etc.) using the data supplied in `data`.
             *
             * @param data Smart pointer to an ISharedObject that contains initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload/release runtime resources associated with this terrain.
             *
             * This should destroy PhysX objects created during `load` and clear any runtime-only
             * state while keeping persistent configuration (if any) intact.
             *
             * @param data Smart pointer to an ISharedObject that may contain unload context.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the raw user-data pointer associated with this terrain.
             *
             * PhysX objects often expose a void* user pointer for engine-specific payloads;
             * this accessor returns the value stored on the wrapper.
             *
             * @return Void pointer previously set by `setUserData` or nullptr if not set.
             */
            void *getUserData() const override;

            /**
             * @brief Set a raw user-data pointer for this terrain instance.
             *
             * The pointer is stored and returned by `getUserData`. Ownership semantics are not
             * managed by this class � callers are responsible for ensuring the pointer remains
             * valid for the required lifetime.
             *
             * @param userData User-provided pointer to associate with this object.
             */
            void setUserData( void *userData ) override;

            /**
             * @brief Retrieve the loaded height samples for the terrain.
             *
             * The returned array contains the height samples in the engine-specific format
             * used to construct the PhysX heightfield. The interpretation (row-major/column-major)
             * follows the conventions used elsewhere in the engine.
             *
             * @return Copy of the height sample array.
             */
            Array<f32> getHeightData() const;

            /**
             * @brief Replace the terrain height samples.
             *
             * Setting new height data typically requires recreating the underlying PhysX
             * heightfield/shape for the change to take effect. Callers should invoke `load`
             * or other rebuild logic as needed after updating heights.
             *
             * @param heightData Array of floats representing per-sample heights.
             */
            void setHeightData( const Array<f32> &heightData );

            /**
             * @brief Get the logical resolution of the heightfield (number of samples).
             *
             * @return Width/height in samples as a `Vector2I` where x is sample count in X
             *         and y is sample count in Y.
             */
            Vector2I getSize() const;

            /**
             * @brief Set the logical resolution of the heightfield.
             *
             * The resolution determines how many height samples are present along each axis.
             * Changing this value typically requires resizing the `heightData` array and
             * rebuilding PhysX resources.
             *
             * @param size Vector2I specifying sample counts in X (x) and Y (y).
             */
            void setSize( const Vector2I &size );

            /**
             * @brief Get the world dimensions of the terrain (size in engine units).
             *
             * This returns the overall extents of the terrain in world space.
             *
             * @return A Vector3<physics_Num> representing (width, height, depth).
             */
            Vector3<physics_Num> getTerrainSize() const;

            /**
             * @brief Set the world-space dimensions of the terrain.
             *
             * The `terrainSize` is used together with the heightfield resolution to compute
             * spacing between samples when creating PhysX heightfield geometry.
             *
             * @param terrainSize World-space dimensions as (width, height, depth).
             */
            void setTerrainSize( const Vector3<physics_Num> &terrainSize );

            /**
             * @brief Get the height scale applied to raw height samples.
             *
             * The height scale maps the stored sample values to world-space heights.
             *
             * @return Vector3<physics_Num> representing height scaling factors.
             */
            Vector3<physics_Num> getHeightScale() const;

            /**
             * @brief Set the height scale applied to raw height samples.
             *
             * Adjusting height scale affects the vertical exaggeration of the terrain.
             *
             * @param heightScale Scaling factors to apply to sample values.
             */
            void setHeightScale( const Vector3<physics_Num> &heightScale );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Construct a PhysX heightfield geometry from the provided height samples.
             *
             * This helper converts the engine's height data, resolution and scaling into
             * a `physx::PxHeightFieldGeometry` that can be attached to a `PxShape`.
             *
             * @param heightData Array of height samples (engine sample format).
             * @param size Logical sample resolution as (width, height).
             * @param heightScale Vertical scale applied to the sample values (per-axis allowed).
             * @param terrainSize World-space size of the terrain (used to compute sample spacing).
             * @param t Local transform to apply to the geometry (position/rotation/scale).
             *
             * @return A configured `physx::PxHeightFieldGeometry`. The underlying `PxHeightField`
             *         referenced by this geometry must be managed and released according to
             *         PhysX ownership rules elsewhere in the class.
             */
            physx::PxHeightFieldGeometry createTerrainGeometry( const Array<f32>      &heightData,
                                                                const Vector2I        &size,
                                                                const Vector3F        &heightScale,
                                                                const Vector3F        &terrainSize,
                                                                const Transform3<f32> &t );

            /**
             * @brief Create a default heightfield-backed PxShape for a simple terrain.
             *
             * Convenience helper which generates a PhysX `PxShape` populated with a default
             * heightfield (filled with a simple ramp/flat distribution) sized `sizeX` by `sizeY`
             * with heights clamped to `maxHeight`. Useful for tests or fallback terrain.
             *
             * @param sizeX Number of samples along the X axis.
             * @param sizeY Number of samples along the Y axis.
             * @param maxHeight Maximum height value for generated samples.
             *
             * @return Pointer to a created `physx::PxShape`. Ownership semantics follow the
             *         surrounding PhysX integration code � caller should not delete directly.
             */
            physx::PxShape *createDefaultTerrain( s32 sizeX, s32 sizeY, f32 maxHeight );

            /**< Vertical scaling applied to the raw height samples to produce world heights. */
            Vector3<physics_Num> m_heightScale;

            /** World-space extents of the terrain (width, height, depth). */
            Vector3<physics_Num> m_terrainSize;

            /** Logical resolution (sample counts) of the heightfield: x = samplesX, y = samplesY. */
            Vector2I m_size;

            /** Raw height samples used to build the PhysX heightfield. */
            Array<f32> m_heightData;

            /**
             * @brief Local transform of the collider relative to its parent actor.
             *
             * Contains position and orientation applied to the shape when attached to an actor.
             */
            Transform3<physics_Num> m_colliderTransform;

            /**
             * @brief Raw pointer to the PhysX material used by the terrain shape.
             *
             * This pointer is a raw PhysX type and must be managed according to PhysX lifecycle
             * rules. A corresponding smart-pointer wrapper `m_physicsMaterial` is provided for
             * engine-level material access.
             */
            RawPtr<physx::PxMaterial> m_material;

            /**
             * @brief Local transform of the terrain shape (duplicate/alias of collider transform
             *        where appropriate).
             *
             * Maintained separately to support cases where the terrain's transform differs
             * from the actor-level collider transform.
             */
            Transform3<physics_Num> m_transform;

            /** Arbitrary user pointer stored on the wrapper (not managed by this class). */
            void *m_userData = nullptr;

            /**
             * @brief Indicates whether this terrain should be treated as static (immovable).
             *
             * When true, the terrain is expected to be static and may be optimized by the
             * physics system accordingly.
             */
            atomic_bool m_isStatic;

            /**
             * @brief Engine-level smart pointer to the physics material for the terrain.
             *
             * This wraps higher-level material properties and lifetime; the underlying PhysX
             * `PxMaterial` is referenced by `m_material`.
             */
            SmartPtr<IPhysicsMaterial3> m_physicsMaterial;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxTerrain_h__
