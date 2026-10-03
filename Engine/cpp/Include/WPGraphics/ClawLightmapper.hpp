#ifndef __Lightmapper_H__
#define __Lightmapper_H__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/ILightmapper.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    class WPGraphics_API Lightmapper : public render::ILightmapper
    {
    public:
        Lightmapper();

        ~Lightmapper() override;

        void update() override;

        void generateLightMap( const Properties &properties );

        WP_CLASS_REGISTER_DECL;

    private:
        void generate( int threadNumber );

        void lightMapEntity( SmartPtr<scene::IGameActor> entity );

        void buildingEntityList( SmartPtr<scene::IGameActor> entity, const AABB3F &box,
                                 Array<SmartPtr<scene::IGameActor>> &entities );

        String createMaterial( const String &curMaterialName, const String &newMaterialName );

        Properties m_properties;

        atomic_u32 LastGenerateRequest;
        atomic_u32 LastGenerate;

        ConcurrentArray<SmartPtr<scene::IGameActor>> m_lightmapEntities;
        bool m_showDebugLightmaps;
        bool m_lightmapAll;
        int m_textureSize;
        bool m_autoTexSize;
        String m_prefix;
        String m_imageExtension;

        u32 m_startTime;
        u32 m_endTime;

        atomic_u32 m_matCounter;
    };
}  // namespace workphone

#endif
