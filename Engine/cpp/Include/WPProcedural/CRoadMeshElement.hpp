#ifndef RoadSegment_h__
#define RoadSegment_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoadMeshElement.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Line3.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API CRoadMeshElement : public CProceduralObject<IRoadMeshElement>
        {
        public:
            CRoadMeshElement();
            ~CRoadMeshElement() override;

            // Lifetime
            void unload( SmartPtr<ISharedObject> data ) override;

            // Parent section access
            SmartPtr<IRoadSection> getParentSection() const override;
            void setParentSection( SmartPtr<IRoadSection> section ) override;

            // Vertex management
            Array<Vector3<real_Num>> getVertices() const override;
            void setVertices( const Array<Vector3<real_Num>> &vertices ) override;
            void addVertex( const Vector3<real_Num> &vertex );
            bool removeVertex( u32 index );
            bool getVertex( u32 index, Vector3<real_Num> &outVertex ) const;
            void clearVertices();
            u32 getVertexCount() const;

            // Index management
            Array<u32> getIndices() const override;
            void setIndices( const Array<u32> &indices ) override;
            void addIndex( u32 index );
            bool removeIndex( u32 index );
            void clearIndices();
            u32 getIndexCount() const;

            // Polygon management
            Array<Polygon3<real_Num>> getPolygons() const override;
            void setPolygons( const Array<Polygon3<real_Num>> &polygons ) override;
            void clearPolygons();
            u32 getPolygonCount() const;

            // Build methods
            void build() override;
            void updateMesh() override;

            // Width accessors
            void setWidth( f32 width );
            void setWidthA( f32 widthA );
            void setWidthB( f32 widthB );
            void setPavementWidth( f32 pavementWidth );

            // Points management
            void setPoints( const Array<Vector3F> &points );
            void addPoint( const Vector3F &point );
            bool removePoint( u32 index );
            void clearPoints();
            u32 getPointCount() const;

            // Utility
            Vector3F getCenter() const;
            f32 getLength() const;
            f32 getAverageWidth() const;
            bool isBuilt() const;
            bool isValid() const;

            // Public geometry data
            Vector3F TangentA;
            Vector3F TangentB;
            Line3F CenterLine;
            Array<Vector3F> Points;
            AABB3F Box;
            f32 Width;
            f32 WidthA;
            f32 WidthB;
            f32 PavementWidth;

            WP_CLASS_REGISTER_DECL;

        private:
            // Internal state
            SmartPtr<IRoadSection> m_parentSection;
            Array<Vector3<real_Num>> m_vertices;
            Array<u32> m_indices;
            Array<Polygon3<real_Num>> m_polygons;
            bool m_isBuilt;

            // Width validation
            bool isWidthValid( f32 width ) const;
            f32 clampWidth( f32 width ) const;

            // Mesh generation helpers
            void invalidateMesh();
            void updateCenterLine();
            void updateTangents();
            void generateVertices();
            void regenerateVertices();
            f32 lerpWidth( f32 t ) const;
            Vector3F calculatePerpendicularOffset( u32 pointIndex, f32 width, bool isLeft ) const;
            void generateIndices();
            void updateBounds();
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // RoadSegment_h__
