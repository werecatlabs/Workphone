#ifndef _OGRE_NEXT_DYNAMIC_LINES_H_
#define _OGRE_NEXT_DYNAMIC_LINES_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/DynamicRenderable.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        class DynamicLinesOgreNext : public DynamicRenderable
        {
        public:
            enum
            {
                POSITION_BINDING,
                TEXCOORD_BINDING
            };

            /// Constructor
            DynamicLinesOgreNext( Ogre::IdType id, Ogre::ObjectMemoryManager *objectMemoryManager,
                                  Ogre::SceneManager *manager,
                                  Ogre::OperationType opType = Ogre::OT_LINE_STRIP );

            ~DynamicLinesOgreNext() override;

            /// Add a point to the point list
            void addPoint( const Ogre::Vector3 &p );

            /// Add a point to the point list
            void addPoint( Ogre::Real x, Ogre::Real y, Ogre::Real z );

            /// Change the location of an existing point in the point list
            void setPoint( unsigned short index, const Ogre::Vector3 &value );

            /// Return the location of an existing point in the point list
            const Ogre::Vector3 &getPoint( unsigned short index ) const;

            /// Return the total number of points in the point list
            unsigned short getNumPoints( void ) const;

            /// Remove all points from the point list
            void clear();

            /// Call this to update the hardware buffer after making changes.
            void update();

            /** Set the type of operation to draw with.
             * @param opType Can be one of
             *    - RenderOperation::OT_LINE_STRIP
             *    - RenderOperation::OT_LINE_LIST
             *    - RenderOperation::OT_POINT_LIST
             *    - RenderOperation::OT_TRIANGLE_LIST
             *    - RenderOperation::OT_TRIANGLE_STRIP
             *    - RenderOperation::OT_TRIANGLE_FAN
             *    The default is OT_LINE_STRIP.
             */
            void setOperationType( Ogre::OperationType opType );
            Ogre::OperationType getOperationType() const;

        protected:
            /// Implementation DynamicRenderable, creates a simple vertex-only decl
            void createVertexDeclaration() override;

            /// Implementation DynamicRenderable, pushes point list out to hardware memory
            void fillHardwareBuffers() override;

            Array<Ogre::Vector3> mPoints;
            bool mDirty = false;
        };
    }  // end namespace render
}  // namespace workphone

#endif
