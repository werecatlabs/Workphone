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

            void addScene( SmartPtr<IProceduralScene> scene ) override;
            void removeScene( SmartPtr<IProceduralScene> scene ) override;
            Array<SmartPtr<IProceduralScene>> getScenes() const override;

        protected:
            Array<SmartPtr<IProceduralScene>> m_scenes;
        };
    }  // namespace procedural
}  // namespace workphone

#endif
