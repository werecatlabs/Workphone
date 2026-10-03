//
// Created by Zane Desir on 11/11/2021.
//

#ifndef WP_ADDRESOURCECMD_H
#define WP_ADDRESOURCECMD_H

#include <EditorPrerequisites.hpp>
#include <commands/Command.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{
    namespace editor
    {

        /** @brief The AddResourceCmd class
         */
        class AddResourceCmd : public Command
        {
        public:
            enum class ResourceType
            {
                None,
                Script,
                LightingPreset,
                Material,
                Scene,
                Director,
            };

            AddResourceCmd();
            ~AddResourceCmd() override;

            void redo() override;
            void execute() override;
            void undo() override;

            String getFilePath() const;
            void setFilePath( const String &filePath );

            ResourceType getResourceType() const;
            void setResourceType( ResourceType resourceType );

            SmartPtr<ISharedObject> getResource() const;

            void setResource( SmartPtr<ISharedObject> resource );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ISharedObject> m_resource;
            AtomicObject<String> m_filePath;
            AtomicObject<String> m_createdFilePath;
            AtomicValue<ResourceType> m_resourceType = ResourceType::None;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // WP_ADDRESOURCECMD_H
