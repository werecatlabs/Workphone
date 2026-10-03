#ifndef LodPage_h__
#define LodPage_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        class LodPage : public ISharedObject
        {
        public:
            LodPage();
            ~LodPage();

            void addObject( SmartPtr<LodObject> lodObject );
            void removeObject( SmartPtr<LodObject> lodObject );

        protected:
            typedef std::map<std::string, Ogre::InstanceManager *> InstanceManagers;
            InstanceManagers m_instanceManagers;

            typedef std::map<u32, SmartPtr<LodObject>> LodObjects;
            LodObjects m_lodObjects;
        };

    }  // namespace render
}  // namespace workphone

#endif  // LodPage_h__
