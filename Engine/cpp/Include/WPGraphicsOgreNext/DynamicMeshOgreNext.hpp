#ifndef __DynamicMesh_H_
#define __DynamicMesh_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/DynamicRenderable.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    namespace render
    {

        class DynamicMeshOgreNext : public DynamicRenderable
        {
        public:
            DynamicMeshOgreNext( Ogre::IdType id, Ogre::ObjectMemoryManager *objectMemoryManager,
                                 Ogre::SceneManager *manager, Ogre::SceneNode *owner = nullptr );
            DynamicMeshOgreNext( Ogre::SceneNode *owner );
            DynamicMeshOgreNext( const String &name, Ogre::SceneNode *owner );
            ~DynamicMeshOgreNext() override;

            void setDirty();
            void update();

            void setOwner( Ogre::SceneNode *owner );
            Ogre::SceneNode *getOwner() const;

            void setMesh( SmartPtr<ISubMesh> subMesh );
            SmartPtr<ISubMesh> getSubMesh() const;

        private:
            void createVertexDeclaration() override;
            void fillHardwareBuffers() override;

            SmartPtr<IStateListener> m_stateListener;

            SmartPtr<ISubMesh> m_engineMesh;
            Ogre::SceneNode *m_owner = nullptr;
            bool m_dirty = false;
        };
    }  // end namespace render
}  // namespace workphone

#endif
