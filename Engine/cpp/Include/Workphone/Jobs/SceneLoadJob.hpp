#ifndef SceneLoadJob_h__
#define SceneLoadJob_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{

    // Inputs are configured before submission and the destination scene is pinned.
    // Async preparation performs only I/O and parsing. This job commits on the
    // primary queue; Finish therefore means the entire graph commit has returned.
    class WPCore_API SceneLoadJob : public Job
    {
    public:
        SceneLoadJob();

        ~SceneLoadJob() override;

        void execute() override;
        void prepare();
        void queuePrepare();

        SmartPtr<scene::IGameScene> getScene() const;

        void setScene( SmartPtr<scene::IGameScene> scene );

        String getFilePath() const;

        void setFilePath( const String &filePath );

        SmartPtr<scene::LightingDirector> getLightingDirector() const;

        void setLightingDirector( SmartPtr<scene::LightingDirector> lightingDirector );

        bool getCreateActorJobs() const;

        void setCreateActorJobs( bool createActorJobs );

        String getDataStr() const;

        void setDataStr( const String &dataStr );

        WP_CLASS_REGISTER_DECL;

    protected:
        AtomicSmartPtr<scene::IGameScene> m_scene;

        AtomicSmartPtr<scene::LightingDirector> m_lightingDirector;

        AtomicObject<String> m_filePath;

        AtomicObject<String> m_dataStr;

        atomic_bool m_createActorJobs = false;
        u64 m_loadGeneration = 0;
        SmartPtr<Properties> m_preparedData;
        String m_preparedPath;
        String m_preparedLabel;
        String m_prepareError;
        bool m_prepared = false;
    };
}  // namespace workphone

#endif  // SceneLoadJob_h__
