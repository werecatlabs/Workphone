#ifndef IWorldGenerator_h__
#define IWorldGenerator_h__

#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>
#include <Workphone/Interface/Procedural/IProceduralWorld.hpp>
#include <Workphone/Interface/Procedural/IProceduralScene.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace procedural
    {

        /**
         * @brief Interface for generating procedural worlds.
         *
         * This interface defines the contract for generating worlds, managing scenes, and controlling
         * the world generation process. It is derived from IProceduralGenerator and adds additional
         * methods for configuring and controlling the world generation process.
         */
        class WPCore_API IWorldGenerator : public IProceduralGenerator
        {
        public:
            /** Virtual destructor. */
            ~IWorldGenerator() override;

            /**
             * @brief Returns the procedural world module.
             *
             * @return The procedural world module, or nullptr if no world has been set.
             */
            virtual SmartPtr<IProceduralWorld> getProceduralWorld() const = 0;

            /**
             * @brief Sets the procedural world module.
             *
             * @param proceduralWorld The procedural world module to set.
             */
            virtual void setProceduralWorld( SmartPtr<IProceduralWorld> proceduralWorld ) = 0;

            /**
             * @brief Returns the scenes managed by this world generator.
             *
             * @return An array of smart pointers to the procedural scenes.
             */
            virtual const Array<SmartPtr<IProceduralScene>> getScenes() const = 0;

            /**
             * @brief Adds a scene to the world.
             *
             * @param scene The scene to add.
             */
            virtual void addScene( SmartPtr<IProceduralScene> scene ) = 0;

            /**
             * @brief Removes a scene from the world.
             *
             * @param scene The scene to remove.
             */
            virtual void removeScene( SmartPtr<IProceduralScene> scene ) = 0;

            /**
             * @brief Sets the scenes managed by this world generator.
             *
             * @param scenes The array of scenes to set.
             */
            virtual void setScenes( Array<SmartPtr<IProceduralScene>> scenes ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IWorldGenerator_h__
