#ifndef ISceneBuilder_h__
#define ISceneBuilder_h__

#include <Workphone/Interface/IObjectBuilder.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Interface for building and managing scene components.
         *
         * The ISceneBuilder interface provides functionality for creating and managing
         * scene elements including the scene itself, directors, and actors. It serves
         * as a factory and manager for scene-related components.
         */
        class WPCore_API IGameSceneBuilder : public IObjectBuilder
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IGameSceneBuilder() override;

            /**
             * @brief Get the current scene instance.
             * @return SmartPtr<IGameScene> A smart pointer to the current scene.
             */
            virtual SmartPtr<IGameScene> getScene() const = 0;

            /**
             * @brief Set the scene instance.
             * @param scene SmartPtr<IGameScene> A smart pointer to the scene to set.
             */
            virtual void setScene( SmartPtr<IGameScene> scene ) = 0;

            /**
             * @brief Get the current director instance.
             * @return SmartPtr<IBuildDirector> A smart pointer to the current director.
             */
            virtual SmartPtr<IBuildDirector> getDirector() const = 0;

            /**
             * @brief Set the director instance.
             * @param director SmartPtr<IBuildDirector> A smart pointer to the director to set.
             */
            virtual void setDirector( SmartPtr<IBuildDirector> director ) = 0;

            /**
             * @brief Get the current actor instance.
             * @return SmartPtr<IGameActor> A smart pointer to the current actor.
             */
            virtual SmartPtr<IGameActor> getActor() const = 0;

            /**
             * @brief Set the actor instance.
             * @param actor SmartPtr<IGameActor> A smart pointer to the actor to set.
             */
            virtual void setActor( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Get all actors in the scene.
             * @return Array<SmartPtr<IGameActor>> An array of smart pointers to all actors.
             */
            virtual Array<SmartPtr<IGameActor>> getActors() const = 0;

            /**
             * @brief Set the collection of actors in the scene.
             * @param actors Array<SmartPtr<IGameActor>> An array of smart pointers to actors to set.
             */
            virtual void setActors( const Array<SmartPtr<IGameActor>> &actors ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // ISceneBuilder_h__
