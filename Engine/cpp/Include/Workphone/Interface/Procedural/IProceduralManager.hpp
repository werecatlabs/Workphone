#ifndef IProceduralManager_h__
#define IProceduralManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IProceduralManager : public ISharedObject
        {
        public:
            ~IProceduralManager() override;

            virtual void generate() = 0;

            virtual SmartPtr<ICityGenerator> getCityGenerator() const = 0;
            virtual void setCityGenerator( SmartPtr<ICityGenerator> cityGenerator ) = 0;

            virtual SmartPtr<ITerrainGenerator> getTerrainGenerator() const = 0;
            virtual void setTerrainGenerator( SmartPtr<ITerrainGenerator> terrainGenerator ) = 0;

            virtual SmartPtr<IProceduralCollision> getCollisionManager() const = 0;
            virtual void setCollisionManager( SmartPtr<IProceduralCollision> collisionManager ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralManager_h__
