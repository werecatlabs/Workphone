// ProceduralModel.hpp
#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Core/Properties.hpp>
#include <vector>
#include <memory>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        // ---------------------------------------------------------------

        /**
         * @class ProceduralVertex
         * @brief A single vertex in a procedurally generated mesh.
         */
        struct WPProcedural_API ProceduralVertex
        {
            Vector3F position;  ///< World-space or local-space position.
            Vector3F normal;    ///< Per-vertex surface normal (unit length after computeNormals).
            Vector2F uv;        ///< Texture coordinates.
            Vector3F mask;      ///< r=wear, g=grime, b=AO
        };

        // ---------------------------------------------------------------

        /**
         * @class ProceduralMesh
         * @brief CPU-side triangle mesh built incrementally by ModelRule instances.
         *
         * Vertices are stored as a flat array; triangles are expressed as index
         * triples.  All mutating operations validate their arguments and log
         * errors without throwing so that partial results are preserved.
         */
        struct WPProcedural_API ProceduralMesh
        {
            std::vector<ProceduralVertex> vertices;  ///< Ordered vertex data.
            std::vector<uint32_t> indices;           ///< Triangle index list (groups of 3).

            /** Remove all vertices and indices, resetting to an empty state. */
            void clear();

            /**
             * @brief Append a vertex and return its index.
             * @return The zero-based index of the new vertex, or UINT32_MAX on overflow.
             */
            uint32_t addVertex( const Vector3F &p );
            uint32_t addVertex( const ProceduralVertex &v );

            /**
             * @brief Append one triangle to the index list.
             *        Validates that a, b and c are in range; logs and skips on failure.
             */
            void addTriangle( uint32_t a, uint32_t b, uint32_t c );

            /**
             * @brief Append another mesh, offsetting every vertex by @p offset.
             *        Skips safely if @p other is empty.
             */
            void append( const ProceduralMesh &other, const Vector3F &offset = {} );

            /**
             * @brief Compute smooth per-vertex normals from the triangle list.
             *        Tolerates index lists whose size is not a multiple of three by
             *        ignoring the trailing incomplete triangle.
             */
            void computeNormals();

            /** @return true when the mesh contains at least one triangle. */
            bool hasGeometry() const;

            /** Compute an axis-aligned bounding box of the mesh (no-op stub for now). */
            void computeBoundingBox();
        };

        // ---------------------------------------------------------------

        /**
         * @class ModelRule
         * @brief Abstract base for all procedural geometry rules.
         *
         * Each concrete subclass generates mesh geometry via build() and
         * exposes its parameters through getProperties() / setProperties()
         * for game-editor access.
         */
        class WPProcedural_API ModelRule
        {
        public:
            virtual ~ModelRule();

            /** Generate geometry into @p outMesh.  Must not throw. */
            virtual void build( ProceduralMesh &outMesh ) const = 0;

            /** Return all editable parameters as a Properties object. */
            virtual SmartPtr<Properties> getProperties() const;

            /** Apply values from @p properties to this rule's parameters. */
            virtual void setProperties( SmartPtr<Properties> properties );
        };

        // ---------------------------------------------------------------

        /**
         * @class BoxRule
         * @brief Emits a six-sided box centred at @p offset.
         */
        class WPProcedural_API BoxRule final : public ModelRule
        {
        public:
            Vector3F size{ 1.0f, 1.0f, 1.0f };    ///< Full extents along each axis (must be > 0).
            Vector3F offset{ 0.0f, 0.0f, 0.0f };  ///< Centre position in local space.

            BoxRule();
            BoxRule( const Vector3F &size_, const Vector3F &offset_ = {} );

            void build( ProceduralMesh &outMesh ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class CylinderRule
         * @brief Emits a capped cylinder centred at @p offset.
         */
        class WPProcedural_API CylinderRule final : public ModelRule
        {
        public:
            float radius = 0.5f;                  ///< Radius of the cylinder (must be > 0).
            float height = 1.0f;                  ///< Total height of the cylinder (must be > 0).
            int segments = 8;                     ///< Number of radial subdivisions (minimum 3).
            Vector3F offset{ 0.0f, 0.0f, 0.0f };  ///< Centre position in local space.

            CylinderRule();
            CylinderRule( float radius_, float height_, int segments_, const Vector3F &offset_ = {} );

            void build( ProceduralMesh &outMesh ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class ArrayRule
         * @brief Repeats a child rule @p count times, offsetting each copy by @p step.
         */
        class WPProcedural_API ArrayRule final : public ModelRule
        {
        public:
            std::shared_ptr<ModelRule> child;   ///< Rule to replicate (must not be null at build time).
            int count = 1;                      ///< Number of copies (minimum 1).
            Vector3F step{ 1.0f, 0.0f, 0.0f };  ///< Per-copy translation increment.

            ArrayRule( std::shared_ptr<ModelRule> child_, int count_, const Vector3F &step_ );

            void build( ProceduralMesh &outMesh ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class CompositeRule
         * @brief Builds each child rule in order into the same output mesh.
         */
        class WPProcedural_API CompositeRule final : public ModelRule
        {
        public:
            std::vector<std::shared_ptr<ModelRule>> children;  ///< Ordered child rules.

            /** Append @p rule to the child list.  Null rules are rejected. */
            void add( std::shared_ptr<ModelRule> rule );

            void build( ProceduralMesh &outMesh ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class ProceduralModelComponent
         * @brief ECS-style data component that owns one rule tree and its baked result.
         */
        struct WPProcedural_API ProceduralModelComponent
        {
            std::shared_ptr<ModelRule> rootRule;  ///< Root of the rule hierarchy (may be null).
            ProceduralMesh bakedMesh;             ///< Last successfully baked output.
            bool dirty = true;                    ///< True when the mesh needs to be rebuilt.

            SmartPtr<Properties> getProperties() const;
            void setProperties( SmartPtr<Properties> properties );
        };

        // ---------------------------------------------------------------

        /**
         * @class ProceduralModelSystem
         * @brief Drives baking of ProceduralModelComponent instances.
         *
         * Call bake() once per frame (or on demand) for every component
         * whose dirty flag is set.
         */
        class WPProcedural_API ProceduralModelSystem
        {
        public:
            /**
             * @brief Rebuild @p component's baked mesh if it is dirty.
             *        Catches and logs all exceptions; leaves the previous mesh
             *        intact if the rebuild fails.
             */
            void bake( ProceduralModelComponent &component );
        };

    }  // namespace procedural
}  // namespace workphone
