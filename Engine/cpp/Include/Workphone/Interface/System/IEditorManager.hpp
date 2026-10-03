#ifndef IEditorManager_h__
#define IEditorManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for the editor manager.
     *
     * The IEditorManager interface provides methods for managing editor-specific functionality,
     * such as asset import, project and cache path management, debug visualization toggles,
     * and manipulation tools for translation, rotation, and scaling within the editor environment.
     *
     * Implementations of this interface are responsible for handling editor state and tools
     * that affect the scene and UI during editing sessions.
     */
    class WPCore_API IEditorManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~IEditorManager() override;

        /**
         * @brief Imports assets into the editor environment.
         *
         * This function should handle the process of loading or registering assets
         * required by the editor or project.
         */
        virtual void importAssets() = 0;

        /**
         * @brief Gets the current project path.
         * @return The file system path to the current project as a String.
         */
        virtual String getProjectPath() const = 0;

        /**
         * @brief Sets the project path.
         * @param path The file system path to set as the current project.
         */
        virtual void setProjectPath( const String &path ) = 0;

        /**
         * @brief Gets the cache path used by the editor.
         * @return The file system path to the cache directory as a String.
         */
        virtual String getCachePath() const = 0;

        /**
         * @brief Sets the cache path used by the editor.
         * @param path The file system path to set as the cache directory.
         */
        virtual void setCachePath( const String &path ) = 0;

        /**
         * @brief Checks if debug information is shown in the editor.
         * @return True if debug information is displayed, false otherwise.
         */
        virtual bool getShowDebug() const = 0;

        /**
         * @brief Sets whether debug information should be shown in the editor.
         * @param showDebug True to display debug information, false to hide it.
         */
        virtual void setShowDebug( bool showDebug ) = 0;

        /**
         * @brief Gets the current translate manipulator tool.
         * @return Smart pointer to the TranslateManipulator instance.
         */
        virtual SmartPtr<TranslateManipulator> getTranslateManipulator() const = 0;

        /**
         * @brief Sets the translate manipulator tool.
         * @param translateManipulator Smart pointer to the TranslateManipulator to set.
         */
        virtual void setTranslateManipulator( SmartPtr<TranslateManipulator> translateManipulator ) = 0;

        /**
         * @brief Gets the current rotate manipulator tool.
         * @return Smart pointer to the RotateManipulator instance.
         */
        virtual SmartPtr<RotateManipulator> getRotateManipulator() const = 0;

        /**
         * @brief Sets the rotate manipulator tool.
         * @param rotateManipulator Smart pointer to the RotateManipulator to set.
         */
        virtual void setRotateManipulator( SmartPtr<RotateManipulator> rotateManipulator ) = 0;

        /**
         * @brief Gets the current scale manipulator tool.
         * @return Smart pointer to the ScaleManipulator instance.
         */
        virtual SmartPtr<ScaleManipulator> getScaleManipulator() const = 0;

        /**
         * @brief Sets the scale manipulator tool.
         * @param scaleManipulator Smart pointer to the ScaleManipulator to set.
         */
        virtual void setScaleManipulator( SmartPtr<ScaleManipulator> scaleManipulator ) = 0;

        /**
         * @brief Checks if transformations are applied in local space.
         * @return True if transformations are local, false if global.
         */
        virtual bool isTransformLocal() const = 0;

        /**
         * @brief Sets whether transformations are applied in local or global space.
         * @param transformLocal True for local space, false for global space.
         */
        virtual void setTransformLocal( bool transformLocal ) = 0;

        /** Refreshes transform controls after an editor manipulation. */
        virtual void refreshTransformUI()
        {
        }

        /**
         * @brief Checks if scene debug drawing is enabled.
         * @return True if scene debug drawing is enabled, false otherwise.
         */
        virtual bool getDrawSceneDebug() const = 0;

        /**
         * @brief Sets whether scene debug drawing is enabled.
         * @param drawSceneDebug True to enable scene debug drawing, false to disable.
         */
        virtual void setDrawSceneDebug( bool drawSceneDebug ) = 0;

        /**
         * @brief Checks if UI debug drawing is enabled.
         * @return True if UI debug drawing is enabled, false otherwise.
         */
        virtual bool getDrawUiDebug() const = 0;

        /**
         * @brief Sets whether UI debug drawing is enabled.
         * @param drawUiDebug True to enable UI debug drawing, false to disable.
         */
        virtual void setDrawUiDebug( bool drawUiDebug ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IEditorManager_h__
