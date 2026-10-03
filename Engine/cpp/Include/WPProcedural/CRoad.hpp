#ifndef ProceduralRoad_h__
#define ProceduralRoad_h__

#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoad.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Math/Line2.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @class CRoad
         * @brief Concrete implementation of the IRoad interface for procedural road generation.
         *
         * Manages road nodes, segments, sections and provides intersection, subdivision,
         * and property serialisation support for the game editor.
         */
        class WPProcedural_API CRoad : public CProceduralObject<IRoad>
        {
        public:
            CRoad();
            ~CRoad() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            void build() override;

            void updateBounds() override;

            bool intersects( const Sphere3<real_Num> &sphere );
            bool intersects( const Sphere3<real_Num> &sphere, SmartPtr<IRoadNode> &collidingNode ) const;
            bool intersects( const AABB3<real_Num> &box );
            bool intersects( const Polygon2<real_Num> &polygon );
            bool intersects( const Line2<real_Num> &line, Vector2<real_Num> &intersectionPoint,
                             bool checkStart = false );
            bool intersects( SmartPtr<IRoad> road, Vector3<real_Num> &intersectionPoint,
                             bool checkStart = false ) const;
            bool intersects( SmartPtr<IRoad> road, Vector3<real_Num> &intersectionPoint,
                             Array<SmartPtr<IRoadNode>> &nodes, bool checkStart = false ) const;
            bool intersects( SmartPtr<IRoadNode> node, Array<SmartPtr<IRoadNode>> &nodes,
                             bool checkStart = false ) const;

            Array<SmartPtr<IRoadHitPoint>> intersects( SmartPtr<IRoad> road ) override;

            void addNode( SmartPtr<IRoadNode> node );
            void removeNode( SmartPtr<IRoadNode> node );
            void removeNodes();

            Array<SmartPtr<IRoadNode>> getRoadNodes() const override;

            SmartPtr<IRoadNode> getNode( size_t index ) const override;
            SmartPtr<IRoadNode> getFirstNode() const override;
            SmartPtr<IRoadNode> getLastNode() const override;

            void insertNode( const Array<SmartPtr<IRoadNode>> &nodes, SmartPtr<IRoadNode> newNode );

            const Array<SmartPtr<IRoadNode>> &getRoadNodeObjects() const;

            bool isPartOfRoad( const SmartPtr<IRoadNode> &node ) const;

            f32 getRoadLength() const;

            Vector3<real_Num> getPointOnRoad( real_Num distance );
            Vector3<real_Num> getPointOnRoad( real_Num distance,
                                              Array<SmartPtr<IRoadNode>> &pointNodes );

            SmartPtr<IRoad> subDivide( real_Num maxNodeDistance );

            Array<SmartPtr<IRoadElement>> &getRoadSegments();
            const Array<SmartPtr<IRoadElement>> &getRoadSegments() const;
            void setRoadSegments( Array<SmartPtr<IRoadElement>> value );

            String getRoadType() const override;
            void setRoadType( const String &value ) override;

            s32 getMarkerFromTransform( const Transform3<real_Num> &transform ) override;

            void addRoadSection( SmartPtr<IRoadSection> section ) override;
            void removeRoadSection( SmartPtr<IRoadSection> section ) override;

            Array<SmartPtr<IRoadSection>> getRoadSections() const override;
            void setRoadSections( Array<SmartPtr<IRoadSection>> value ) override;

            Array<SmartPtr<IProceduralNode>> getNodes() const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_roadType;                       ///< Type tag for this road (e.g. "residential").
            Array<SmartPtr<IRoadNode>> m_roadNodes;  ///< Ordered list of nodes defining the road spine.
            Array<SmartPtr<IRoadElement>> m_roadSegments;  ///< Road segment elements.
            Array<SmartPtr<IRoadSection>> m_roadSections;  ///< Road sections built from the nodes.
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ProceduralRoad_h__
