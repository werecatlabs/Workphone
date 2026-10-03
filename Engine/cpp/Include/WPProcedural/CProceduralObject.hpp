#ifndef CProceduralObject_h__
#define CProceduralObject_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IProceduralNode.hpp>
#include <Workphone/Interface/Procedural/IProceduralScene.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    namespace procedural
    {
        template <class T>
        class CProceduralObject : public T
        {
        public:
            CProceduralObject()
            {
                m_properties = workphone::make_ptr<Properties>();
            }

            ~CProceduralObject() override
            {
                unload( nullptr );
            }

            void unload( SmartPtr<ISharedObject> data ) override
            {
                for( auto node : m_nodes )
                {
                    node->unload( nullptr );
                }

                m_nodes.clear();

                // m_worldTransform = nullptr;
                m_properties = nullptr;
                m_scene = nullptr;
            }

            virtual void build()
            {
                updateBounds();
            }

            virtual void updateBounds()
            {
                m_Polygon = getPolygon();
                m_Bounds = getBounds();

                auto boundsCenter = m_Bounds.getCenter();
                auto boundsLength = m_Bounds.getExtent().length();

                m_BoundingSphere = Sphere3<real_Num>( boundsCenter, boundsLength );
            }

            Polygon2<real_Num> getPolygon() const
            {
                return Polygon2<real_Num>();
            }

            Polygon3<real_Num> getPolygon3() const
            {
                return Polygon3<real_Num>();
            }

            AABB3<real_Num> getBounds() const
            {
                auto bounds = AABB3<real_Num>();
                auto polygon = getPolygon3();

                auto center = polygon.getCenter();
                // bounds.setCenter(center);

                auto points = polygon.getPoints();
                for( auto p : points )
                {
                    bounds.merge( p );
                }

                return bounds;
            }

            virtual String getName() const
            {
                return m_name;
            }

            virtual void setName( const String &name )
            {
                m_name = name;
            }

            virtual Transform3<real_Num> getWorldTransform() const
            {
                return m_worldTransform;
            }

            virtual void setWorldTransform( Transform3<real_Num> transform )
            {
                m_worldTransform = transform;
            }

            virtual void setPosition( const Vector3<real_Num> &position )
            {
                m_worldTransform.setPosition( position );
            }

            virtual Vector3<real_Num> getPosition() const
            {
                return m_worldTransform.getPosition();
            }

            virtual Vector3<real_Num> getScale() const
            {
                return m_worldTransform.getScale();
            }

            virtual void setScale( const Vector3<real_Num> &value )
            {
                m_worldTransform.setScale( value );
            }

            virtual Quaternion<real_Num> getOrientation() const
            {
                return m_worldTransform.getOrientation();
            }

            virtual void setOrientation( const Quaternion<real_Num> &value )
            {
                m_worldTransform.setOrientation( value );
            }

            virtual SmartPtr<Properties> getProperties() const
            {
                return m_properties;
            }

            virtual void setProperties( SmartPtr<Properties> properties )
            {
                m_properties = properties;
            }

            virtual SmartPtr<ISharedObject> clone()
            {
                return nullptr;
            }

            CProceduralObject &operator=( CProceduralObject &other )
            {
                m_name = other.m_name;
                m_worldTransform = other.m_worldTransform;
                return *this;
            }

            SmartPtr<IProceduralScene> getScene() const
            {
                return m_scene;
            }

            void setScene( SmartPtr<IProceduralScene> value )
            {
                m_scene = value;
            }

            void addNode( SmartPtr<IProceduralNode> node )
            {
                m_nodes.push_back( node );
            }

            void removeNode( SmartPtr<IProceduralNode> node )
            {
                auto it = std::find( m_nodes.begin(), m_nodes.end(), node );
                if( it != m_nodes.end() )
                {
                    m_nodes.erase( it );
                }
            }

            virtual Array<SmartPtr<IProceduralNode>> getNodes() const
            {
                return m_nodes;
            }

        protected:
            /// The object id.
            String m_Id;

            /// The object's name.
            String m_name;

            /// Store the transform data.
            Transform3<real_Num> m_worldTransform;

            /// The object properties.
            SmartPtr<Properties> m_properties;

            /// The scene this object belongs to.
            SmartPtr<IProceduralScene> m_scene;

            /// nodes
            Array<SmartPtr<IProceduralNode>> m_nodes;

            /// polygon
            Polygon2<real_Num> m_Polygon;

            /// bounds
            AABB3<real_Num> m_Bounds;

            /// sphere
            Sphere3<real_Num> m_BoundingSphere;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CProceduralObject_h__
