#ifndef LODManager_h__
#define LODManager_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Grid2.hpp>

namespace workphone
{
    namespace render
    {

        class LodManager : public ISharedObject
        {
        public:
            LodManager();
            ~LodManager();

            void addObject( SmartPtr<LodObject> lodObject );
            void removeObject( SmartPtr<LodObject> lodObject );

        protected:
            SmartPtr<LodPage> getPage( const Vector2I &index );

            typedef std::map<Vector2I, SmartPtr<LodPage>> LodPages;
            LodPages m_lodPages;

            Grid2 m_grid;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // LODManager_h__
