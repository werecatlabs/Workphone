#ifndef CProceduralWorld_h__
#define CProceduralWorld_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralWorld.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API CProceduralWorld : public IProceduralWorld
        {
        public:
            CProceduralWorld();
            ~CProceduralWorld() override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void addScene( SmartPtr<IProceduralScene> scene ) override;
            void removeScene( SmartPtr<IProceduralScene> scene ) override;
            Array<SmartPtr<IProceduralScene>> getScenes() const override;
            void setScenes( const Array<SmartPtr<IProceduralScene>> &scenes ) override;

        protected:
            Array<SmartPtr<IProceduralScene>> m_scenes{
                Allocator<SmartPtr<IProceduralScene>>( sourceBudget ) };
        };
    }  // namespace procedural
}  // namespace workphone

#endif
