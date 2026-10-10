#ifndef _REMOVE_ENTITY_BODY_CMD_H
#define _REMOVE_ENTITY_BODY_CMD_H

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Core/Properties.hpp>
#include <commands/Command.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * Command to remove a resource from the resource manager.
         */
        class RemoveResourceCmd : public Command
        {
        public:
            /**
             * Constructor.
             */
            RemoveResourceCmd();

            /**
             * Destructor.
             */
            ~RemoveResourceCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;

            String getFilePath() const;

            void setFilePath( const String &filePath );

            /**
             * Gets the resource to be removed.
             * @return The resource to be removed.
             */
            SmartPtr<IResource> getResource() const;

            /**
             * Sets the resource to be removed.
             * @param val The resource to be removed.
             */
            void setResource( SmartPtr<IResource> resource );

            WP_CLASS_REGISTER_DECL;

        private:
            String m_filePath;
            SmartPtr<IResource> m_resource;
            SmartPtr<AssetDatabaseManager> m_operationCatalog;
            String m_operationId;
            String m_operationRoot;
        };
    }  // namespace editor
}  // namespace workphone

#endif
