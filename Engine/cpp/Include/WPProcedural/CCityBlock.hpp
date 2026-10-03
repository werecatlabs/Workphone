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
        class WPProcedural_API CCityBlock : public CProceduralObject<ICityBlock>
        {
        public:
            CCityBlock();
            ~CCityBlock() override;

            void addPoint( const Vector3<real_Num> &point ) override;
            void removePoint( const Vector3<real_Num> &point ) override;
            Array<Vector3<real_Num>> getPoints() const override;

            Polygon2<real_Num> getPolygon() const override;

            bool equals( SmartPtr<ICityBlock> other ) const override;

            Array<SmartPtr<ILot>> getLots() const override;
            void setLots( Array<SmartPtr<ILot>> lots ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<Vector3<real_Num>> m_points;
            Array<SmartPtr<ILot>> m_lots;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // CCityBlock_h__
