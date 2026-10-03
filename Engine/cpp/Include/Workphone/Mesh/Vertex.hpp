#ifndef __FBVertex__H
#define __FBVertex__H

#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    /** A vertex. */
    struct Vertex
    {
        Vertex();

        bool isFinite() const;

        Vector3<real_Num> position;
        Vector3<real_Num> normal;
        Vector2<real_Num> texCoord;
        Vector2<real_Num> texCoord1;
    };

    class FoliageVertex
    {
    public:
        Vector3F position;
        Vector3F normal;
        Vector3F tangent;
        Vector3F texCoord;
        f32 vertexData[4];  // store properties
    };

}  // namespace workphone

#endif
