/*
zlib License

Copyright (c) 2026 ${Your Name}

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented.
2. Altered source versions must be plainly marked as such.
3. This notice may not be removed or altered from any source distribution.
*/
#ifndef __IApplication_h__
#define __IApplication_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone
{
    namespace core
    {

        /**
         * @brief Interface for an application class that serves as the main entry point for game or
         * application development.
         *
         * This interface provides core functionality for managing application lifecycle, scene creation,
         * resource management, and event handling. It serves as a foundation for implementing game
         * engines or complex applications with features like scene management, UI creation, and physics
         * simulation.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class WPCore_API IApplication : public ISharedObject
        {
        public:
            static const String mediaPathStr;
            static const String mediaPathStrBundle;

            static const u8 frameStatisticsFlag;
            static const u8 debugModeFlag;
            static const u8 developerModeFlag;

            IApplication();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of resources when the application is destroyed.
             */
            ~IApplication() override;

            /**
             * @brief Starts the main application loop.
             *
             * This method initializes the application and begins the main game/application loop.
             * The loop continues running until the application is explicitly closed or exited.
             * This is where the primary application logic and rendering occurs.
             */
            virtual void run() = 0;

            /**
             * @brief Performs a single iteration of the application loop.
             *
             * This method is called once per frame and handles all per-frame updates,
             * including physics, rendering, and game logic updates.
             */
            virtual void iterate() = 0;

            /**
             * @brief Processes and handles application events.
             *
             * @param eventType The type of event that occurred
             * @param eventValue A hash value identifying the specific event
             * @param arguments Additional parameters associated with the event
             * @param sender The object that triggered the event
             * @param object The target object of the event
             * @param event The event object containing detailed information
             * @return Parameter containing the result of event handling
             */
            virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                                           const Array<Parameter> &arguments,
                                           SmartPtr<ISharedObject> sender,
                                           SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) = 0;

            /**
             * @brief Retrieves the application's finite state machine.
             *
             * The FSM manages the application's state transitions and controls the flow
             * of the application's execution.
             *
             * @return IFSM* A pointer to the application's FSM
             */
            virtual IFSM *getFSMPtr() const = 0;

            /**
             * @brief Retrieves the application's finite state machine.
             *
             * The FSM manages the application's state transitions and controls the flow
             * of the application's execution.
             *
             * @return SmartPtr<IFSM> A shared pointer to the application's FSM
             */
            virtual SmartPtr<IFSM> getFSM() const = 0;

            /**
             * @brief Sets the application's finite state machine.
             *
             * @param fsm The new FSM to be used by the application
             */
            virtual void setFSM( SmartPtr<IFSM> fsm ) = 0;

            /**
             * @brief Gets the number of currently active threads in the application.
             *
             * @return size_t The number of active threads
             */
            virtual u32 getActiveThreads() const = 0;

            /**
             * @brief Sets the number of active threads for the application.
             *
             * @param activeThreads The desired number of active threads
             */
            virtual void setActiveThreads( u32 activeThreads ) = 0;

            /**
             * @brief Creates a new scene object with specified type and director.
             *
             * @param type The type identifier for the scene object
             * @param director The director containing additional scene information
             * @return SmartPtr<scene::IGameActor> The newly created scene object
             */
            virtual SmartPtr<scene::IGameActor> createSceneObject(
                u32 type, SmartPtr<IBuildDirector> director ) = 0;

            /**
             * @brief Creates a default UI panel.
             *
             * @param director The director for the panel
             * @param hint Optional hint text for the panel
             * @param addToScene Whether to automatically add the panel to the scene
             * @return SmartPtr<scene::IGameActor> The created panel actor
             */
            virtual SmartPtr<scene::IGameActor> createPanel( SmartPtr<IBuildDirector> director,
                                                             const String &hint = "",
                                                             bool addToScene = true ) = 0;

            /**
             * @brief Creates a default UI button.
             *
             * @param label The text label for the button
             * @param director The director for the button
             * @param hint Optional hint text for the button
             * @param addToScene Whether to automatically add the button to the scene
             * @return SmartPtr<scene::IGameActor> The created button actor
             */
            virtual SmartPtr<scene::IGameActor> createButton( const String &label,
                                                              SmartPtr<IBuildDirector> director,
                                                              const String &hint = "",
                                                              bool addToScene = true ) = 0;

            /**
             * @brief Creates a default text element.
             *
             * @param label The text content
             * @param director The director for the text element
             * @param hint Optional hint text
             * @param addToScene Whether to automatically add the text to the scene
             * @return SmartPtr<scene::IGameActor> The created text actor
             */
            virtual SmartPtr<scene::IGameActor> createText( const String &label,
                                                            SmartPtr<IBuildDirector> director,
                                                            const String &hint = "",
                                                            bool addToScene = true ) = 0;

            /**
             * @brief Creates a default toggle/checkbox element.
             *
             * @param label The label for the toggle
             * @param director The director for the toggle
             * @param hint Optional hint text
             * @param addToScene Whether to automatically add the toggle to the scene
             * @return SmartPtr<scene::IGameActor> The created toggle actor
             */
            virtual SmartPtr<scene::IGameActor> createToggle( const String &label,
                                                              SmartPtr<IBuildDirector> director,
                                                              const String &hint = "",
                                                              bool addToScene = true ) = 0;

            /**
             * @brief Creates a default slider control.
             *
             * @param label The label for the slider
             * @param director The director for the slider
             * @param hint Optional hint text
             * @param addToScene Whether to automatically add the slider to the scene
             * @return SmartPtr<scene::IGameActor> The created slider actor
             */
            virtual SmartPtr<scene::IGameActor> createSlider( const String &label,
                                                              SmartPtr<IBuildDirector> director,
                                                              const String &hint = "",
                                                              bool addToScene = true ) = 0;

            /**
             * @brief Creates a default scrollbar control.
             *
             * @param label The label for the scrollbar
             * @param director The director for the scrollbar
             * @param hint Optional hint text
             * @param addToScene Whether to automatically add the scrollbar to the scene
             * @return SmartPtr<scene::IGameActor> The created scrollbar actor
             */
            virtual SmartPtr<scene::IGameActor> createScrollbar( const String &label,
                                                                 SmartPtr<IBuildDirector> director,
                                                                 const String &hint = "",
                                                                 bool addToScene = true ) = 0;

            /**
             * @brief Creates a default cubemap for the application.
             *
             * @param addToScene Whether to automatically add the cubemap to the scene
             * @return SmartPtr<scene::IGameActor> The created cubemap actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultCubemap( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default sky for the scene.
             *
             * @param addToScene Whether to automatically add the sky to the scene
             * @return SmartPtr<scene::IGameActor> The created sky actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultSky( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default camera for the scene.
             *
             * @param addToScene Whether to automatically add the camera to the scene
             * @return SmartPtr<scene::IGameActor> The created camera actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultCamera( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default cube mesh.
             *
             * @param addToScene Whether to automatically add the cube to the scene
             * @return SmartPtr<scene::IGameActor> The created cube actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultCube( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default cube mesh with physics properties.
             *
             * @param addToScene Whether to automatically add the cube mesh to the scene
             * @return SmartPtr<scene::IGameActor> The created cube mesh actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultCubeMesh( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default ground plane.
             *
             * @param addToScene Whether to automatically add the ground to the scene
             * @return SmartPtr<scene::IGameActor> The created ground actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultGround( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default terrain.
             *
             * @param addToScene Whether to automatically add the terrain to the scene
             * @return SmartPtr<scene::IGameActor> The created terrain actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultTerrain( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default physics constraint.
             *
             * @return SmartPtr<scene::IGameActor> The created constraint actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultConstraint() = 0;

            /**
             * @brief Creates a directional light source.
             *
             * @param addToScene Whether to automatically add the light to the scene
             * @return SmartPtr<scene::IGameActor> The created directional light actor
             */
            virtual SmartPtr<scene::IGameActor> createDirectionalLight( bool addToScene = true ) = 0;

            /**
             * @brief Creates a point light source.
             *
             * @param addToScene Whether to automatically add the light to the scene
             * @return SmartPtr<scene::IGameActor> The created point light actor
             */
            virtual SmartPtr<scene::IGameActor> createPointLight( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default plane mesh.
             *
             * @param addToScene Whether to automatically add the plane to the scene
             * @return SmartPtr<scene::IGameActor> The created plane actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultPlane( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default vehicle with physics properties.
             *
             * @param addToScene Whether to automatically add the vehicle to the scene
             * @return SmartPtr<scene::IGameActor> The created vehicle actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultVehicle( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default car with physics properties.
             *
             * @param addToScene Whether to automatically add the car to the scene
             * @return SmartPtr<scene::IGameActor> The created car actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultCar( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default truck with physics properties.
             *
             * @param addToScene Whether to automatically add the truck to the scene
             * @return SmartPtr<scene::IGameActor> The created truck actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultTruck( bool addToScene = true ) = 0;

            /**
             * @brief Creates a default particle system.
             *
             * @param addToScene Whether to automatically add the particle system to the scene
             * @return SmartPtr<scene::IGameActor> The created particle system actor
             */
            virtual SmartPtr<scene::IGameActor> createDefaultParticleSystem(
                bool addToScene = true ) = 0;

            /**
             * @brief Creates a default UI material.
             *
             * @return SmartPtr<render::IMaterial> The created UI material
             */
            virtual SmartPtr<render::IMaterial> createDefaultMaterialUI() = 0;

            /**
             * @brief Creates a default material for 3D objects.
             *
             * @return SmartPtr<render::IMaterial> The created material
             */
            virtual SmartPtr<render::IMaterial> createDefaultMaterial() = 0;

            /**
             * @brief Creates all default materials used by the application.
             */
            virtual void createDefaultMaterials() = 0;

            /**
             * @brief Creates a rigid static mesh for physics simulation.
             *
             * @param actor The actor to create the rigid static mesh for
             * @param recursive Whether to process child actors recursively
             */
            virtual void createRigidStaticMesh( SmartPtr<scene::IGameActor> actor, bool recursive ) = 0;

            /**
             * @brief Creates a rigid dynamic mesh for physics simulation.
             *
             * @param actor The actor to create the rigid dynamic mesh for
             * @param recursive Whether to process child actors recursively
             */
            virtual void createRigidDynamicMesh( SmartPtr<scene::IGameActor> actor, bool recursive ) = 0;

            /**
             * @brief Imports a scene from a file.
             *
             * @param filePath The path to the scene file
             * @return SmartPtr<Properties> The properties of the imported scene
             */
            virtual SmartPtr<Properties> importScene( const String &filePath ) = 0;

            /**
             * @brief Creates rigid static meshes for all applicable actors in the scene.
             */
            virtual void createRigidStaticMesh() = 0;

            /**
             * @brief Creates rigid dynamic meshes for all applicable actors in the scene.
             */
            virtual void createRigidDynamicMesh() = 0;

            /**
             * @brief Gets whether frame statistics are being created.
             *
             * @return bool True if frame statistics are being created, false otherwise
             */
            virtual bool getCreateFrameStatistics() const = 0;

            /**
             * @brief Sets whether frame statistics should be created.
             *
             * @param bCreateFrameStatistics Whether to create frame statistics
             */
            virtual void setCreateFrameStatistics( bool bCreateFrameStatistics ) = 0;

            /**
             * @brief Gets the path to the plugins configuration file.
             *
             * @return String The path to the plugins configuration file
             */
            virtual String getPluginsConfigFilePath() const = 0;

            /**
             * @brief Sets the path to the plugins configuration file.
             *
             * @param pluginsConfigFilePath The new path to the plugins configuration file
             */
            virtual void setPluginsConfigFilePath( const String &pluginsConfigFilePath ) = 0;

            /**
             * @brief Gets whether the application is running in debug mode.
             *
             * @return bool True if in debug mode, false otherwise
             */
            virtual bool isDebugMode() const = 0;

            /**
             * @brief Sets whether the application should run in debug mode.
             *
             * @param debugMode Whether to enable debug mode
             */
            virtual void setDebugMode( bool debugMode ) = 0;

            /**
             * @brief Gets the media path for the application.
             *
             * The media path is platform-dependent and contains resources like textures,
             * models, and other media files.
             *
             * @return String The media path
             */
            virtual String getMediaPath() const = 0;

            virtual u8 getApplicationFlags() const = 0;

            virtual void setApplicationFlags( u8 applicationFlags ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace core
}  // namespace workphone

#endif  // __IApplication_h__
