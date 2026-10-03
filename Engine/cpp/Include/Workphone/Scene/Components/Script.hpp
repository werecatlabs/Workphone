#ifndef UserComponent_h__
#define UserComponent_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class UserComponent
         * @brief Represents a user-defined component that can be attached to actors in the scene.
         *
         * This component allows for custom script functionality and properties, enabling users to extend
         * the behavior of actors. It supports script invocation, property management, and edit mode
         * updates.
         */
        class WPCore_API Script : public Component
        {
        public:
            /**
             * @class ScriptReceiver
             * @brief Receives and handles script calls for the UserComponent.
             *
             * This nested class implements the IScriptReceiver interface, allowing the UserComponent
             * to receive and process script property changes and function calls.
             */
            class ScriptReceiver : public IScriptReceiver
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                ScriptReceiver();

                /**
                 * @brief Constructor with owner.
                 * @param owner Pointer to the owning UserComponent.
                 */
                explicit ScriptReceiver( Script *owner );

                /**
                 * @brief Virtual destructor.
                 */
                ~ScriptReceiver() override;

                /**
                 * @brief Sets a property by hash and string value.
                 * @param hash Property hash.
                 * @param value Property value as string.
                 * @return Status code.
                 */
                s32 setProperty( hash_type hash, const String &value ) override;

                /**
                 * @brief Gets a property by hash and string value.
                 * @param hash Property hash.
                 * @param value Output property value as string.
                 * @return Status code.
                 */
                s32 getProperty( hash_type hash, String &value ) const override;

                /**
                 * @brief Sets a property by hash and parameter.
                 * @param hash Property hash.
                 * @param param Property parameter.
                 * @return Status code.
                 */
                s32 setProperty( hash_type hash, const Parameter &param ) override;

                /**
                 * @brief Sets a property by hash and parameters.
                 * @param hash Property hash.
                 * @param params Property parameters.
                 * @return Status code.
                 */
                s32 setProperty( hash_type hash, const Parameters &params ) override;

                /**
                 * @brief Sets a property by hash and raw pointer.
                 * @param hash Property hash.
                 * @param param Raw pointer to property data.
                 * @return Status code.
                 */
                s32 setProperty( hash_type hash, void *param ) override;

                /**
                 * @brief Gets a property by hash and parameter.
                 * @param hash Property hash.
                 * @param param Output property parameter.
                 * @return Status code.
                 */
                s32 getProperty( hash_type hash, Parameter &param ) const override;

                /**
                 * @brief Gets a property by hash and parameters.
                 * @param hash Property hash.
                 * @param params Output property parameters.
                 * @return Status code.
                 */
                s32 getProperty( hash_type hash, Parameters &params ) const override;

                /**
                 * @brief Gets a property by hash and raw pointer.
                 * @param hash Property hash.
                 * @param param Output raw pointer to property data.
                 * @return Status code.
                 */
                s32 getProperty( hash_type hash, void *param ) const override;

                /**
                 * @brief Calls a script function by hash and parameters.
                 * @param hash Function hash.
                 * @param params Input parameters.
                 * @param results Output results.
                 * @return Status code.
                 */
                s32 callFunction( hash_type hash, const Parameters &params,
                                  Parameters &results ) override;

                /**
                 * @brief Calls a script function by hash and shared object.
                 * @param hash Function hash.
                 * @param object Shared object parameter.
                 * @param results Output results.
                 * @return Status code.
                 */
                s32 callFunction( hash_type hash, SmartPtr<ISharedObject> object,
                                  Parameters &results ) override;

                /**
                 * @brief Gets the owner UserComponent.
                 * @return Pointer to the owner UserComponent.
                 */
                Script *getOwner() const;

                /**
                 * @brief Sets the owner UserComponent.
                 * @param owner Pointer to the owner UserComponent.
                 */
                void setOwner( Script *owner );

            private:
                Script *m_owner = nullptr;  ///< Pointer to the owner UserComponent.
            };

            /** @name Static Hashes and Strings */
            ///@{
            static const hash_type UPDATE_HASH;            ///< Hash for the update function.
            static const hash_type GET_COMPONENT_HASH;     ///< Hash for getting a component.
            static const hash_type ADD_COMPONENT_HASH;     ///< Hash for adding a component.
            static const hash_type REMOVE_COMPONENT_HASH;  ///< Hash for removing a component.
            static const hash_type POSITION_HASH;          ///< Hash for position property.
            static const hash_type ROTATION_HASH;          ///< Hash for rotation property.
            static const hash_type SCALE_HASH;             ///< Hash for scale property.

            static const String classNameStr;  ///< String identifier for the class name.
            static const String
                updateInEditModeStr;  ///< String identifier for update-in-edit-mode property.
            static const String
                updateInPlayModeStr;  ///< String identifier for update-in-play-mode property.
            static const String getPropertiesStr;  ///< String identifier for getting properties.
            static const String setPropertiesStr;  ///< String identifier for setting properties.
            ///@}

            /**
             * @brief Default constructor.
             */
            Script();

            /**
             * @brief Virtual destructor.
             */
            ~Script() override;

            /**
             * @brief Loads the component with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the component, releasing resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reloads the component with the given data.
             * @param data Shared object containing reload data.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the component. Called every frame or tick.
             */
            void update() override;

            /**
             * @brief Gets the script invoker for this component.
             * @return Smart pointer to the script invoker.
             */
            SmartPtr<IScriptInvoker> getInvoker() const;

            /**
             * @brief Sets the script invoker for this component.
             * @param invoker Smart pointer to the script invoker.
             */
            void setInvoker( SmartPtr<IScriptInvoker> invoker );

            /**
             * @brief Gets the script receiver for this component.
             * @return Smart pointer to the script receiver.
             */
            SmartPtr<IScriptReceiver> getReceiver() const;

            /**
             * @brief Sets the script receiver for this component.
             * @param receiver Smart pointer to the script receiver.
             */
            void setReceiver( SmartPtr<IScriptReceiver> receiver );

            /**
             * @brief Gets the properties of the component.
             * @return Smart pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the component.
             * @param properties Smart pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Checks if the component updates in edit mode.
             * @return True if updates in edit mode, false otherwise.
             */
            bool getUpdateInEditMode() const;

            /**
             * @brief Sets whether the component should update in edit mode.
             * @param updateInEditMode True to update in edit mode, false otherwise.
             */
            void setUpdateInEditMode( bool updateInEditMode );

            /**
             * @brief Checks if the component updates in play mode.
             * @return True if updates in play mode, false otherwise.
             */
            bool getUpdateInPlayMode() const;

            /**
             * @brief Sets whether the component should update in play mode.
             * @param updateInPlayMode True to update in play mode, false otherwise.
             */
            void setUpdateInPlayMode( bool updateInPlayMode );

            /**
             * @brief Gets the script class associated with this component.
             * @return Smart pointer to the script class.
             */
            SmartPtr<IScriptClass> getScriptClass() const;

            /**
             * @brief Sets the script class for this component.
             * @param scriptClass Smart pointer to the script class.
             */
            void setScriptClass( SmartPtr<IScriptClass> scriptClass );

            /**
             * @brief Gets the class name of the component.
             * @return The class name as a string.
             */
            String getClassName() const;

            /**
             * @brief Sets the class name of the component.
             * @param className The class name as a string.
             */
            void setClassName( const String &className );

            /**
             * @brief Generates the component.
             */
            void generate();

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handles component events using a finite state machine.
             * @param state Current state.
             * @param eventType Event type.
             * @return FSM return type.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Creates script-related data for the component.
             */
            void createScriptData();

            /**
             * @brief Destroys script-related data for the component.
             */
            void destroyScriptData();

            void updateEditModeState();
            void updatePlayModeState();

            bool m_updateInPlayMode = false;       ///< Whether the component updates in play mode.
            bool m_updateInEditMode = false;       ///< Whether the component updates in edit mode.
            String m_className;                    ///< Name of the script class.
            SmartPtr<IScriptClass> m_scriptClass;  ///< Script class associated with this component.
            SmartPtr<IScriptInvoker> m_invoker;    ///< Used to call script functions.
            SmartPtr<IScriptReceiver> m_receiver;  ///< Used to receive script calls.
            SmartPtr<IScriptData> m_scriptData;    ///< The data used by the script system.
        };
    }  // namespace scene
}  // namespace workphone

#endif  // UserComponent_h__
