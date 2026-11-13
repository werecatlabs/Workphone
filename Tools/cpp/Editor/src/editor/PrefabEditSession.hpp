#pragma once
#include <EditorPrerequisites.hpp>
#include <memory>

namespace workphone::editor
{
    // A separate editing document. The user's scene and its undo objects stay alive.
    class PrefabEditSession
    {
    public:
        PrefabEditSession();
        ~PrefabEditSession();
        bool open( const String &path, String &error );
        bool save( String &error, size_t *updatedInstances = nullptr );
        bool close( String &error );
        bool active() const;
        String path() const;
        SmartPtr<scene::IGameActor> root() const;
        SmartPtr<scene::IGameScene> authoringScene() const;
        SmartPtr<ISharedObject> authoringData() const;
    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
