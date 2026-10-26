#pragma once
#include "MaterialCpuAttributionProbe.hpp"
#include <cstdlib>
#include <stdexcept>

namespace material_cpu_attribution
{
    class Client
    {
    public:
        Client()
        {
            enabled=std::getenv("WP_MATERIAL_CPU_ATTRIBUTION")!=nullptr;
            if(!enabled) return;
            const auto module=GetModuleHandleW(L"WPGraphics.dll");
            const auto get=module?reinterpret_cast<GetApi>(GetProcAddress(module,"wp_material_cpu_attribution_v1")):nullptr;
            api=get?get():nullptr;
            if(!api || api->version!=Version || api->bytes!=sizeof(Api))
                throw std::runtime_error("Material CPU attribution requires the matching instrumented renderer");
            if(const auto setting=std::getenv("WP_MATERIAL_CPU_SAMPLE_STRIDE"))
            {
                char *end=nullptr; const auto value=std::strtoul(setting,&end,10);
                if(!end || *end || !value || value>4096) throw std::runtime_error("Invalid material CPU sampling stride");
                stride=static_cast<uint32_t>(value);
            }
            api->calibrate();
        }
        void reset()
        { if(api) api->configure(stride,0x9e3779b9u^(++sequence*0x85ebca6bu),true); }
        void begin(Stage stage) { if(api) api->begin(stage); }
        void end(Stage stage) { if(api) api->end(stage); }
        Metrics read() const { Metrics result{}; if(api) api->read(&result); return result; }
        bool isEnabled() const { return enabled; }
    private:
        const Api *api=nullptr;
        bool enabled=false;
        uint32_t stride=32,sequence=0;
    };
}
