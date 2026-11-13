#pragma once
#include <Workphone/Core/Properties.hpp>
#include <memory>

namespace workphone::scene
{
    class IGameActor;
    class IGameScene;
    class PrefabInstanceService;
    class WPCore_API PrefabUpdateTransaction
    {
    public:
        ~PrefabUpdateTransaction();
        void commit();
        void rollback();
        void finish();
        size_t changedCount() const;
        struct Impl;
    private:
        explicit PrefabUpdateTransaction( std::unique_ptr<Impl> implementation );
        std::unique_ptr<Impl> m_impl;
        friend class PrefabInstanceService;
    };
    class WPCore_API PrefabInstanceService
    {
    public:
        static SmartPtr<Properties> createLinkedInstanceData( SmartPtr<Properties> source,
            const String &sourcePath, const String &sourceAssetUuid = "" );
        static SmartPtr<Properties> makeTemplateData( SmartPtr<IGameActor> editedRoot );
        static std::unique_ptr<PrefabUpdateTransaction> prepareUpdate(
            SmartPtr<IGameActor> root, SmartPtr<Properties> source );
        static std::unique_ptr<PrefabUpdateTransaction> prepareUpdates(
            SmartPtr<IGameScene> scene, const String &sourcePath, SmartPtr<Properties> source,
            bool migrateWerecat = true );
        // Explicit adoption only; source structure must match the existing hierarchy.
        static void adopt( SmartPtr<IGameActor> root, SmartPtr<Properties> source,
            const String &sourcePath, const String &sourceAssetUuid = "" );
    };
}
