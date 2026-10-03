#ifndef _WATER_MESH_H_
#define _WATER_MESH_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include "OgrePlatform.h"
#include "Ogre.h"

namespace workphone
{
    namespace render
    {
        class WaterMesh
        {
        private:
            Ogre::MeshPtr mesh;
            Ogre::SubMesh *subMesh;
            float *vertexBuffers[3];  // we need 3 vertex buffers
            int currentBuffNumber;
            int complexity;
            String meshName;
            int numFaces;
            int numVertices;
            Ogre::Vector3 *vNormals;

            // HardwareVertexBufferSharedPtr posVertexBuffer ;
            // HardwareVertexBufferSharedPtr normVertexBuffer ;
            // HardwareVertexBufferSharedPtr texcoordsVertexBuffer ;
            // HardwareIndexBufferSharedPtr indexBuffer ;

            f32 lastTimeStamp;
            f32 lastAnimationTimeStamp;
            f32 lastFrameTime;

            void calculateFakeNormals();
            void calculateNormals();

        public:
            WaterMesh( const String &meshName, f32 planeSize, int complexity );

            virtual ~WaterMesh();

            /** "pushes" a mesh at position [x,y]. Note, that x,y are float, hence
             *	4 vertices are actually pushed
             *	@note
             *		This should be replaced by push with 'radius' parameter to simulate
             *  		big objects falling into water
             */
            void push( f32 x, f32 y, f32 depth, bool absolute = false );

            /** gets height at given x and y, takes average value of the closes nodes */
            Ogre::Real getHeight( f32 x, f32 y );

            /** updates mesh */
            void updateMesh( f32 timeSinceLastFrame );

            Ogre::Real PARAM_C;  // ripple speed
            Ogre::Real PARAM_D;  // distance
            Ogre::Real PARAM_U;  // viscosity
            Ogre::Real PARAM_T;  // time
            bool useFakeNormals;
        };
    }  // end namespace render
}  // namespace workphone

#endif
