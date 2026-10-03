#ifndef SceneBuilder_h__
#define SceneBuilder_h__

#include <Workphone/Interface/Scene/IGameSceneBuilder.hpp>

namespace workphone
{
    namespace scene
    {
        /** Scene implementation. */
        class WPCore_API GameSceneBuilder : public IGameSceneBuilder
        {
        public:
            /** Constructor. */
            GameSceneBuilder();

            /** Destructor. */
            ~GameSceneBuilder() override;

            /** @copydoc ISceneBuilder::getScene */
            SmartPtr<IGameScene> getScene() const override;

            /** @copydoc ISceneBuilder::setScene */
            void setScene( SmartPtr<IGameScene> scene ) override;

            /** @copydoc ISceneBuilder::getDirector */
            SmartPtr<IBuildDirector> getDirector() const override;

            /** @copydoc ISceneBuilder::setDirector */
            void setDirector( SmartPtr<IBuildDirector> director ) override;

            /** @copydoc ISceneBuilder::create */
            SmartPtr<ISharedObject> create() override;

            /** @copydoc ISceneBuilder::getActor */
            SmartPtr<IGameActor> getActor() const override;

            /** @copydoc ISceneBuilder::setActor */
            void setActor( SmartPtr<IGameActor> actor ) override;

            /** @copydoc ISceneBuilder::getActors */
            Array<SmartPtr<IGameActor>> getActors() const override;

            /** @copydoc ISceneBuilder::setActors */
            void setActors( const Array<SmartPtr<IGameActor>> &actors ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** The scene. */
            SmartPtr<IGameScene> m_scene;

            /** The director. */
            SmartPtr<IBuildDirector> m_director;

            /** The actor created. */
            SmartPtr<IGameActor> m_actor;

            /** The actors created. */
            Array<SmartPtr<IGameActor>> m_actors;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // SceneBuilder_h__
