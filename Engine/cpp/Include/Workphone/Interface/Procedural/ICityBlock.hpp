#ifndef ICityBlock_h__
#define ICityBlock_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API ICityBlock : public IProceduralObject
        {
        public:
            /** Virtual destructor. */
            ~ICityBlock() override;

            virtual void addPoint( const Vector3<real_Num> &point ) = 0;
            virtual void removePoint( const Vector3<real_Num> &point ) = 0;
            virtual Array<Vector3<real_Num>> getPoints() const = 0;

            virtual Polygon2<real_Num> getPolygon() const = 0;

            virtual bool equals( SmartPtr<ICityBlock> other ) const = 0;

            virtual Array<SmartPtr<ILot>> getLots() const = 0;
            virtual void setLots( Array<SmartPtr<ILot>> lots ) = 0;

            SmartPtr<ISharedObject> clone() override = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ICityBlock_h__
