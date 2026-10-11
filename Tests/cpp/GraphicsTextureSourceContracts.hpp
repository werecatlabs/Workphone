#pragma once
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace graphics_texture_source_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require( bool value, const String &message )
    { if(!value) throw std::runtime_error(message.c_str()); }

    // Uses the cooked foliage fixture's 4x4 green cutout descriptor before its
    // source directory is hidden. No compiler cache or catalog entry is needed.
    inline void run( const String &root, const resource::ResourceID &id )
    {
        auto scheduler=WorldStreamingScheduler::shared(); const auto baseline=scheduler->stats(); String error;
        {
            StreamBudget retained;
            CookedTextureData data;
            require(decodeGraphicsTextureSource(root,id,data,error,84,&retained),error);
            require(data.ddsBytes.empty()&&data.levels.size()==3&&data.levels[0].width==4&&
                data.levels[0].height==4&&data.levels[0].bgra[1]==173&&
                data.mipSettings.filter==TextureMipFilter::Cutout&&data.target=="windows-dx11"&&
                !data.packagedBuild,"Authored descriptor reuses exact portable cutout mip semantics");
            const auto *pixels=data.levels[0].bgra.data();const auto retainedIdentity=retained.get();
            require(retained&&retained->bytes()>=84,"Decoded result owns retained source accounting");
            require(!decodeGraphicsTextureSource(root,id,data,error,83,&retained)&&
                pixels==data.levels[0].bgra.data()&&retained.get()==retainedIdentity,
                "Header output rejection retains previous pixels and budget lease");
            require(scheduler->stats().used[size_t(StreamMemoryClass::WorkingCpu)]==
                baseline.used[size_t(StreamMemoryClass::WorkingCpu)],"All decode scratch retires before return");

            const auto pressure=scheduler->stats();
            struct Restore
            {
                std::shared_ptr<WorldStreamingScheduler> scheduler;WorldStreamStats original;
                ~Restore(){String error;for(auto type:{StreamMemoryClass::WorkingCpu,StreamMemoryClass::SourceCpu})
                    scheduler->setBudget(type,original.limits[size_t(type)],error);}
            }restore{scheduler,pressure};
            require(scheduler->setBudget(StreamMemoryClass::WorkingCpu,pressure.used[size_t(StreamMemoryClass::WorkingCpu)],error),error);
            require(!decodeGraphicsTextureSource(root,id,data,error,84,&retained)&&
                pixels==data.levels[0].bgra.data()&&retained.get()==retainedIdentity,
                "Working budget exhaustion preserves previous decoded texture");
            require(scheduler->setBudget(StreamMemoryClass::WorkingCpu,pressure.limits[size_t(StreamMemoryClass::WorkingCpu)],error),error);
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,pressure.used[size_t(StreamMemoryClass::SourceCpu)],error),error);
            require(!decodeGraphicsTextureSource(root,id,data,error,84,&retained)&&
                pixels==data.levels[0].bgra.data()&&retained.get()==retainedIdentity,
                "Retained source admission precedes decoder output allocation");
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,pressure.limits[size_t(StreamMemoryClass::SourceCpu)],error),error);

            const auto invalid=std::filesystem::u8path(root.c_str())/"invalid-source.texres";
            {std::ofstream file(invalid,std::ios::binary);file<<"{\"version\":1,\"source\":\"data://../outside.tga\",\"mipFilter\":\"cutout\",\"atlasColumns\":1,\"alphaCutoff\":0.5}";
             require(bool(file),"Write invalid texture descriptor fixture");}
            require(!decodeGraphicsTextureSource(root,resource::ResourceID("data://invalid-source.texres"),data,error,84,&retained)&&
                pixels==data.levels[0].bgra.data(),"Authored texture descriptor rejects source traversal");
            require(decodeGraphicsTextureSource(root,id,data,error,84,&retained),"Decode retries after restored budgets");
        }
        require(scheduler->stats().used==baseline.used,"Authored texture decode leases balance after result destruction");
    }
}
