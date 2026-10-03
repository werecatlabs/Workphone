#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone, IDebug, ISharedObject );

    IDebug::~IDebug() = default;

    namespace
    {
        constexpr u32 boxEdges[12][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
                                          { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };

        hash_type makeDebugPrimitiveId( hash_type id, hash_type primitive, hash_type element )
        {
            // Keep compound primitive ids separate from ordinary caller ids and from each other.
            auto value =
                id ^ ( primitive + static_cast<hash_type>( 0x9e3779b9u ) + ( id << 6u ) + ( id >> 2u ) );
            return value ^ ( element + static_cast<hash_type>( 0x85ebca6bu ) + ( value << 6u ) +
                             ( value >> 2u ) );
        }

        void drawBoxEdges( IDebug &debug, hash_type id, hash_type primitive,
                           const Vector3<real_Num> *corners, u32 colour )
        {
            for( u32 edgeIdx = 0; edgeIdx < 12; ++edgeIdx )
            {
                const auto edgeId = makeDebugPrimitiveId( id, primitive, edgeIdx );
                debug.drawLine( edgeId, corners[boxEdges[edgeIdx][0]], corners[boxEdges[edgeIdx][1]],
                                colour );
            }
        }
    }  // namespace

    void IDebug::drawAABB( hash_type id, const AABB3<real_Num> &box, u32 colour )
    {
        if( !box.isFinite() || !box.isValid() )
        {
            return;
        }

        const auto &minimum = box.getMinimum();
        const auto &maximum = box.getMaximum();
        const Vector3<real_Num> corners[8] = {
            { minimum.X(), minimum.Y(), maximum.Z() }, { maximum.X(), minimum.Y(), maximum.Z() },
            { maximum.X(), maximum.Y(), maximum.Z() }, { minimum.X(), maximum.Y(), maximum.Z() },
            { minimum.X(), minimum.Y(), minimum.Z() }, { maximum.X(), minimum.Y(), minimum.Z() },
            { maximum.X(), maximum.Y(), minimum.Z() }, { minimum.X(), maximum.Y(), minimum.Z() }
        };
        drawBoxEdges( *this, id, static_cast<hash_type>( 0x41414242u ), corners, colour );
    }

    void IDebug::drawOBB( hash_type id, const OBB3<real_Num> &box, u32 colour )
    {
        if( !box.isValid() || !box.getCenter().isFinite() || !box.getOrientation().isFinite() )
        {
            return;
        }

        Vector3<real_Num> corners[8];
        box.getCorners( corners );
        drawBoxEdges( *this, id, static_cast<hash_type>( 0x4f424220u ), corners, colour );
    }

    void IDebug::drawArrow( hash_type id, const Vector3<real_Num> &start,
                            const Vector3<real_Num> &vector, u32 colour, real_Num headScale )
    {
        const auto length = vector.length();
        if( !start.isFinite() || !vector.isFinite() || !Math<real_Num>::isFinite( length ) ||
            !Math<real_Num>::isFinite( headScale ) || length <= Math<real_Num>::epsilon() )
        {
            return;
        }

        const auto end = start + vector;
        drawLine( makeDebugPrimitiveId( id, static_cast<hash_type>( 0x41525257u ), 0u ), start, end,
                  colour );

        const auto direction = vector / length;
        auto referenceAxis = Vector3<real_Num>::unitY();
        if( Math<real_Num>::Abs( direction.dotProduct( referenceAxis ) ) >
            static_cast<real_Num>( 0.95 ) )
        {
            referenceAxis = Vector3<real_Num>::unitX();
        }

        const auto side = direction.crossProduct( referenceAxis ).normaliseCopy();
        const auto clampedHeadScale = Math<real_Num>::clamp( headScale, static_cast<real_Num>( 0.01 ),
                                                             static_cast<real_Num>( 0.9 ) );
        const auto headLength = length * clampedHeadScale;
        const auto headWidth = headLength * static_cast<real_Num>( 0.5 );
        const auto headBase = end - direction * headLength;

        drawLine( makeDebugPrimitiveId( id, static_cast<hash_type>( 0x41525257u ), 1u ), end,
                  headBase + side * headWidth, colour );
        drawLine( makeDebugPrimitiveId( id, static_cast<hash_type>( 0x41525257u ), 2u ), end,
                  headBase - side * headWidth, colour );
    }

}  // namespace workphone::render
