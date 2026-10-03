#ifndef Area_h__
#define Area_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
//#include <WPProcedural/ProceduralNode.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <map>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API Area  //: public CProceduralObject
        {
        public:
            Area();
            ~Area();

            void setCityMapValue( const Vector2F &index, bool value, bool addMapPoint = false );
            bool getCityMapValue( const Vector2F &index );
            bool hasCityMapEntry( const Vector2F &index ) const;
            AABB3F getMapAABB() const;
            std::map<Vector2F, bool> getCityMap() const;

        protected:
            using CityMap = std::map<Vector2F, bool>;  // to know the area thats painted
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // Area_h__
