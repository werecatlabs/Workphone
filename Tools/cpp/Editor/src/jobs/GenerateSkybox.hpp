#ifndef GenerateSkyboxMaterials_h__
#define GenerateSkyboxMaterials_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>

namespace workphone
{
    namespace editor
    {
        /** Generate skybox materials */
        class GenerateSkybox : public Job
        {
        public:
            /** Constructor */
            GenerateSkybox();

            /** Destructor */
            ~GenerateSkybox() override;

            /** @copydoc Job::execute */
            void execute() override;

            /** Get the folder path
             * @return The folder path.
             */
            String getFolderPath() const;

            /** Set the folder path
             * @param folderPath The folder path.
             */
            void setFolderPath( const String &folderPath );

        protected:
            void setupSkyboxFromFiles( SmartPtr<scene::Skybox> skybox, const Array<String> &files );

            /** Create a material from a folder */
            void createMaterialFromFolder( SmartPtr<IFolderExplorer> folder );

            /** Setup the skybox from a folder */
            void setupSkyboxFromFolder( SmartPtr<IFolderExplorer> folder );

            /** Folder path */
            AtomicObject<String> m_folderPath;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // GenerateSkyboxMaterials_h__
