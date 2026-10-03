#ifndef LodObject_h__
#define LodObject_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace render
    {

        class LodObject : public ISharedObject
        {
        public:
            LodObject();
            ~LodObject();

            u32 getId() const
            {
                return m_id;
            }
            void setId( u32 id )
            {
                m_id = id;
            }

            LodPage *getLodPage() const
            {
                return m_lodPage;
            }
            void setLodPage( LodPage *lodPage )
            {
                m_lodPage = lodPage;
            }

            std::string getMeshName() const;
            void setMeshName( const std::string &meshName );

            Vector3F getPosition() const
            {
                return m_position;
            }
            void setPosition( const Vector3F &position )
            {
                m_position = position;
            }

            Vector3F getScale() const
            {
                return m_scale;
            }
            void setScale( const Vector3F &scale )
            {
                m_scale = scale;
            }

            QuaternionF getOrientation() const
            {
                return m_orientation;
            }
            void setOrientation( const QuaternionF &orientation )
            {
                m_orientation = orientation;
            }

        protected:
            LodPage *m_lodPage;

            Ogre::Entity *m_entity;
            Ogre::SceneNode *m_node;

            u32 m_id;

            Vector3F m_position;
            Vector3F m_scale;
            QuaternionF m_orientation;

            std::string m_meshName;

            static u32 m_idExt;
        };

    }  // namespace render
}  // namespace workphone

#endif  // LodObject_h__
