#ifndef ModifyPropertyCmd_h__
#define ModifyPropertyCmd_h__

#include <commands/Command.hpp>
#include <Workphone/Database/AssetReferenceChange.hpp>

namespace workphone
{
    namespace editor
    {
        class ModifyPropertyCmd : public Command
        {
        public:
            ModifyPropertyCmd();
            ~ModifyPropertyCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;
            bool configure( SmartPtr<AssetDatabaseManager> catalog,
                            const AssetDatabaseManager::EntrySnapshot &entry,
                            const Array<SmartPtr<ISharedObject>> &targets, const String &property,
                            hash_type expectedType );
            const String &getError() const;
            bool wasApplied() const;

            WP_CLASS_REGISTER_DECL;
        private:
            SmartPtr<AssetDatabaseManager> activeCatalog() const;
            void markDirty();
            Array<AssetReferenceChange> m_changes;
            String m_error;
            bool m_applied = false;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // ModifyPropertyCmd_h__
