#ifndef CCityBlock_h__
#define CCityBlock_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/ICityBlock.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <WPProcedural/CProceduralObject.hpp>

namespace workphone
{
    namespace procedural
    {
        /// Ordered boundary in world coordinates. Child node positions are the geometry source.
        class WPProcedural_API CCityBlock : public CProceduralObject<ICityBlock>
        {
        public:
            CCityBlock();
            ~CCityBlock() override;

            void addPoint( const Vector3<real_Num> &point ) override;
            void removePoint( const Vector3<real_Num> &point ) override;
            Array<Vector3<real_Num>> getPoints() const override;

            Polygon2<real_Num> getPolygon() const override;

            /// Exact 3D boundary equality, independent of start vertex and winding.
            /// Ignores one repeated closing vertex; lots and metadata are not compared.
            bool equals( SmartPtr<ICityBlock> other ) const override;

            Array<SmartPtr<ILot>> getLots() const override;
            void setLots( Array<SmartPtr<ILot>> lots ) override;

            void addNode( SmartPtr<IProceduralNode> node ) override;
            void removeNode( SmartPtr<IProceduralNode> node ) override;
            Polygon3<real_Num> getPolygon3() const;
            AABB3<real_Num> getBounds() const;
            void updateBounds() override;
            /// Copies boundary nodes and properties; lots remain shared and scene ownership is detached.
            SmartPtr<ISharedObject> clone() override;
            void unload( SmartPtr<ISharedObject> data ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<ILot>> m_lots;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // CCityBlock_h__
