#ifndef _OgreUtil_H
#define _OgreUtil_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <OgreVector.h>
#include <OgreString.h>
#include <OgreColourValue.h>
#include <OgreAxisAlignedBox.h>

namespace workphone
{
    namespace render
    {
        /** Provides functions to help deal with ogre. */
        class OgreUtil
        {
        public:
            static Ogre::String toString( const String &str );
            static Ogre::String toString( const StringW &str );

            static bool equals( const Ogre::Vector3 &a, const Ogre::Vector3 &b );
            static bool equals( const Ogre::Quaternion &a, const Ogre::Quaternion &b );

            static bool equals( const Ogre::ColourValue &a, const Ogre::ColourValue &b );

            static Ogre::ColourValue convertToOgre( const ColourF &c );
            static ColourF convert( const Ogre::ColourValue &c );

            /** Converts this engine's vector to an ogre vector. */
            template <class T>
            static Ogre::Vector2 convertToOgre( const Vector2<T> &vector );

            /** Converts this engine's vector to an ogre vector. */
            template <class T>
            static Ogre::Vector3 convertToOgre( const Vector3<T> &vector );

            static Vector3F convert( const Ogre::Vector3 &vector );

            /** Converts this engine's Quaternion to an ogre Quaternion. */
            static Ogre::Quaternion convertToOgre( const QuaternionF &orienation );

            static QuaternionF convert( const Ogre::Quaternion &orienation );

            static AABB3F convert( const Ogre::AxisAlignedBox &box );

            static Ogre::Vector3 getDerivedPosition( Ogre::SceneNode *sceneNode );
            static Ogre::AxisAlignedBox getDerivedBoundingBox( Ogre::SceneNode *node );

            static void getVertexMinMax( Ogre::Entity *entity, Ogre::Vector3 &minPoint,
                                         Ogre::Vector3 &maxPoint );

            static s32 calculateNearest2Pow( s32 input );
        };

        template <class T>
        Ogre::Vector2 OgreUtil::convertToOgre( const Vector2<T> &vector )
        {
            return Ogre::Vector2( vector.x, vector.y );
        }

        template <class T>
        Ogre::Vector3 OgreUtil::convertToOgre( const Vector3<T> &vector )
        {
            return Ogre::Vector3( vector.x, vector.y, vector.z );
        }

        inline Ogre::Quaternion convertToOgreVector( const QuaternionF &orienation )
        {
            return Ogre::Quaternion( orienation.w, orienation.x, orienation.y, orienation.z );
        }

    }  // namespace render
}  // namespace workphone

#endif
