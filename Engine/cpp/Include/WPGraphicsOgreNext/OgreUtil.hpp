#ifndef _OgreUtil_H
#define _OgreUtil_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgreString.h>
#include <OgreAxisAlignedBox.h>
#include <Terra/Hlms/OgreHlmsTerraPrerequisites.h>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <OgreImage2.h>

/**
 * @namespace workphone::render
 * @brief Contains rendering utilities and helpers for Ogre integration.
 */
namespace workphone
{
    namespace render
    {

        /**
         * @class OgreUtil
         * @brief Utility class providing static helper functions for working with Ogre types and conversions.
         *
         * This class offers a collection of static methods to facilitate conversions between engine-specific types
         * (such as ColourF, Vector2, Vector3, Quaternion, etc.) and their Ogre equivalents. It also provides
         * utility functions for mesh information extraction, bounding box calculations, and other common Ogre-related tasks.
         */
        class OgreUtil
        {
        public:
            /**
             * @brief Converts an engine String to an Ogre::String.
             * @param str The engine String to convert.
             * @return The equivalent Ogre::String.
             */
            static Ogre::String toString( const String &str );

            /**
             * @brief Converts an engine StringW (wide string) to an Ogre::String.
             * @param str The engine StringW to convert.
             * @return The equivalent Ogre::String.
             */
            static Ogre::String toString( const StringW &str );

            /**
             * @brief Compares two Ogre::Vector3 objects for equality.
             * @param a First vector.
             * @param b Second vector.
             * @return True if the vectors are equal, false otherwise.
             */
            static bool equals( const Ogre::Vector3 &a, const Ogre::Vector3 &b );

            /**
             * @brief Compares two Ogre::Quaternion objects for equality.
             * @param a First quaternion.
             * @param b Second quaternion.
             * @return True if the quaternions are equal, false otherwise.
             */
            static bool equals( const Ogre::Quaternion &a, const Ogre::Quaternion &b );

            /**
             * @brief Compares two Ogre::ColourValue objects for equality.
             * @param a First colour value.
             * @param b Second colour value.
             * @return True if the colour values are equal, false otherwise.
             */
            static bool equals( const Ogre::ColourValue &a, const Ogre::ColourValue &b );

            /**
             * @brief Converts an engine ColourF to an Ogre::ColourValue.
             * @param c The ColourF to convert.
             * @return The equivalent Ogre::ColourValue.
             */
            static Ogre::ColourValue convertToOgre( const ColourF &c );

            /**
             * @brief Converts an Ogre::ColourValue to an engine ColourF.
             * @param c The Ogre::ColourValue to convert.
             * @return The equivalent ColourF.
             */
            static ColourF convert( const Ogre::ColourValue &c );

            /**
             * @brief Converts an Ogre::Matrix4 to an engine Matrix4F.
             * @param matrix The Ogre::Matrix4 to convert.
             * @return The equivalent Matrix4F.
             */
            static Matrix4F convert( const Ogre::Matrix4 &matrix );

            /**
             * @brief Converts an engine Vector2<T> to an Ogre::Vector2.
             * @tparam T The underlying type of the vector components.
             * @param vector The Vector2<T> to convert.
             * @return The equivalent Ogre::Vector2.
             */
            template <class T>
            static Ogre::Vector2 convertToOgre( const Vector2<T> &vector );

            /**
             * @brief Converts an engine Vector3<T> to an Ogre::Vector3.
             * @tparam T The underlying type of the vector components.
             * @param vector The Vector3<T> to convert.
             * @return The equivalent Ogre::Vector3.
             */
            template <class T>
            static Ogre::Vector3 convertToOgre( const Vector3<T> &vector );

            /**
             * @brief Converts an Ogre::Vector3 to an engine Vector3F.
             * @param vector The Ogre::Vector3 to convert.
             * @return The equivalent Vector3F.
             */
            static Vector3F convert( const Ogre::Vector3 &vector );

            /**
             * @brief Converts an engine QuaternionF to an Ogre::Quaternion.
             * @param orienation The QuaternionF to convert.
             * @return The equivalent Ogre::Quaternion.
             */
            static Ogre::Quaternion convertToOgre( const QuaternionF &orienation );

            /**
             * @brief Converts an Ogre::Quaternion to an engine QuaternionF.
             * @param orienation The Ogre::Quaternion to convert.
             * @return The equivalent QuaternionF.
             */
            static QuaternionF convert( const Ogre::Quaternion &orienation );

            /**
             * @brief Converts an Ogre::AxisAlignedBox to an engine AABB3F.
             * @param box The Ogre::AxisAlignedBox to convert.
             * @return The equivalent AABB3F.
             */
            static AABB3F convert( const Ogre::AxisAlignedBox &box );

            /**
             * @brief Gets the derived (world) position of an Ogre::SceneNode.
             * @param sceneNode The scene node to query.
             * @return The derived position as an Ogre::Vector3.
             */
            static Ogre::Vector3 getDerivedPosition( Ogre::SceneNode *sceneNode );

            /**
             * @brief Gets the derived (world) bounding box of an Ogre::SceneNode.
             * @param node The scene node to query.
             * @return The derived bounding box as an Ogre::AxisAlignedBox.
             */
            static Ogre::AxisAlignedBox getDerivedBoundingBox( Ogre::SceneNode *node );

            /**
             * @brief Computes the minimum and maximum vertex positions of an Ogre::Entity's mesh.
             * @param entity The entity whose mesh to analyze.
             * @param minPoint Output parameter for the minimum vertex position.
             * @param maxPoint Output parameter for the maximum vertex position.
             */
            static void getVertexMinMax( Ogre::Entity *entity, Ogre::Vector3 &minPoint,
                                         Ogre::Vector3 &maxPoint );

            /**
             * @brief Calculates the nearest power of two greater than or equal to the input value.
             * @param input The input integer.
             * @return The nearest power of two.
             */
            static s32 calculateNearest2Pow( s32 input );

            /**
             * @brief Extracts mesh information (vertices and indices) from an Ogre::MeshPtr.
             * @param mesh The mesh pointer.
             * @param vertex_count Output parameter for the number of vertices.
             * @param vertices Output parameter for the vertex array (allocated by the function).
             * @param index_count Output parameter for the number of indices.
             * @param indices Output parameter for the index array (allocated by the function).
             * @param position The position offset to apply.
             * @param orient The orientation to apply.
             * @param scale The scale to apply.
             */
            static void getMeshInformation( const Ogre::MeshPtr mesh, size_t &vertex_count,
                                            Ogre::Vector3 *&vertices, size_t &index_count,
                                            unsigned long *&indices, const Ogre::Vector3 &position,
                                            const Ogre::Quaternion &orient, const Ogre::Vector3 &scale );

            /**
             * @brief Extracts mesh information (vertices and indices) from an Ogre v1 Entity.
             * @param entity The v1 entity pointer.
             * @param vertex_count Output parameter for the number of vertices.
             * @param vertices Output parameter for the vertex array (allocated by the function).
             * @param index_count Output parameter for the number of indices.
             * @param indices Output parameter for the index array (allocated by the function).
             * @param position The position offset to apply.
             * @param orient The orientation to apply.
             * @param scale The scale to apply.
             */
            static void getMeshInformation( const Ogre::v1::Entity *entity, size_t &vertex_count,
                                            Ogre::Vector3 *&vertices, size_t &index_count,
                                            unsigned long *&indices, const Ogre::Vector3 &position,
                                            const Ogre::Quaternion &orient, const Ogre::Vector3 &scale );

            /**
             * @brief Converts an engine TerrainTextureTypes enum to an Ogre::TerraTextureTypes enum.
             * @param terrainTextureType The engine terrain texture type.
             * @return The corresponding Ogre::TerraTextureTypes value.
             */
            static Ogre::TerraTextureTypes getTerrainTextureType(
                TerrainTextureTypes terrainTextureType );

            /**
             * @brief Loads an image file into an Ogre::Image2 object.
             * @param filePath The path to the image file.
             * @param outImage The Ogre::Image2 object to load into.
             * @return True if the image was loaded successfully, false otherwise.
             */
            static bool loadIntoImage( const String &filePath, Ogre::Image2 &outImage );
        };

        /**
         * @brief Converts an engine Vector2<T> to an Ogre::Vector2.
         * @tparam T The underlying type of the vector components.
         * @param vector The Vector2<T> to convert.
         * @return The equivalent Ogre::Vector2.
         */
        template <class T>
        inline Ogre::Vector2 OgreUtil::convertToOgre( const Vector2<T> &vector )
        {
            return Ogre::Vector2( vector.X(), vector.Y() );
        }

        /**
         * @brief Converts an engine Vector3<T> to an Ogre::Vector3.
         * @tparam T The underlying type of the vector components.
         * @param vector The Vector3<T> to convert.
         * @return The equivalent Ogre::Vector3.
         */
        template <class T>
        inline Ogre::Vector3 OgreUtil::convertToOgre( const Vector3<T> &vector )
        {
            return Ogre::Vector3( vector.X(), vector.Y(), vector.Z() );
        }

        /**
         * @brief Converts an engine QuaternionF to an Ogre::Quaternion.
         * @param orienation The QuaternionF to convert.
         * @return The equivalent Ogre::Quaternion.
         */
        inline Ogre::Quaternion convertToOgreVector( const QuaternionF &orienation )
        {
            return Ogre::Quaternion( orienation.W(), orienation.X(), orienation.Y(), orienation.Z() );
        }

        /**
         * @brief Converts an Ogre::Matrix4 to an engine Matrix4F.
         * @param matrix The Ogre::Matrix4 to convert.
         * @return The equivalent Matrix4F.
         */
        inline Matrix4F OgreUtil::convert( const Ogre::Matrix4 &matrix )
        {
            return Matrix4F( matrix[0] );
        }

    }  // end namespace render
}  // namespace workphone

#endif
