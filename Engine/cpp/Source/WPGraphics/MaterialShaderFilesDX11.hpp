#pragma once
#include <WPGraphics/MaterialProgramDX11.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <array>
#include <stdexcept>

namespace workphone::render::detail
{
    /** Bounded owner-thread polling. No thread, callback or compiler work enters
     * native Bind. Failed/missing includes remain watched so recovery is automatic. */
    class ShaderFilesDX11
    {
        struct Stamp
        {
            bool exists = false;
            uintmax_t size = 0;
            std::filesystem::file_time_type time{};
            bool operator==(const Stamp &b) const { return exists==b.exists && size==b.size && time==b.time; }
        };
        static Stamp stamp(const std::filesystem::path &path)
        {
            std::error_code error;
            Stamp result;
            result.exists = std::filesystem::is_regular_file(path,error);
            if(result.exists)
            {
                result.size = std::filesystem::file_size(path,error);
                if(error) { result.exists=false; return result; }
                result.time = std::filesystem::last_write_time(path,error);
                if(error) result.exists=false;
            }
            return result;
        }
        struct File
        {
            StreamBudget budget;
            std::filesystem::path path;
            Stamp observed;
            File(const std::filesystem::path &value) : budget(reserveSourceBudgetBytes(
                sizeof(File)+(value.native().capacity()+1)*sizeof(std::filesystem::path::value_type))),
                path(value),observed(stamp(value)) {}
        };
        using FilePtr = std::shared_ptr<File>;
        using Files = std::vector<FilePtr,SourceBudgetAllocator<FilePtr>>;
        Files m_files;
        FilePtr m_roots[2];
        std::chrono::steady_clock::time_point m_next{};
        u64 m_revision = 1;
        FilePtr track(const std::filesystem::path &path)
        {
            auto normalized = std::filesystem::absolute(path).lexically_normal();
            for(auto &file : m_files) if(file->path == normalized) return file;
            if(m_files.size() >= 128) throw std::runtime_error("Shader include dependency limit exceeded");
            auto file = std::make_shared<File>(normalized);
            m_files.push_back(file); return file;
        }
        struct Include : ID3DInclude
        {
            struct Block
            {
                FilePtr file;
                std::vector<char,SourceBudgetAllocator<char>> bytes;
            };
            ShaderFilesDX11 &owner;
            FilePtr root;
            std::vector<std::unique_ptr<Block>,SourceBudgetAllocator<std::unique_ptr<Block>>> blocks;
            size_t total = 0;
            String error;
            Include(ShaderFilesDX11 &o,FilePtr r) : owner(o),root(std::move(r)) {}
            bool read(Block &block)
            {
                auto before=stamp(block.file->path);
                if(!before.exists) { error=String("Shader file missing: ")+String(block.file->path.u8string().c_str()); return false; }
                if(before.size > (4u<<20) || total+before.size > (16u<<20))
                { error="Shader source/include byte limit exceeded"; return false; }
                block.bytes.resize(size_t(before.size)+1);
                std::ifstream input(block.file->path,std::ios::binary);
                if(!input || !input.read(block.bytes.data(),std::streamsize(before.size)) || !(stamp(block.file->path)==before))
                { error=String("Shader file changed or could not be read: ")+String(block.file->path.u8string().c_str()); return false; }
                block.bytes[size_t(before.size)]=0;
                total+=size_t(before.size); return true;
            }
            HRESULT __stdcall Open(D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID parent, LPCVOID *data, UINT *bytes) override
            {
                try
                {
                    auto directory=root->path.parent_path();
                    for(auto &block : blocks) if(block->bytes.data()==parent) { directory=block->file->path.parent_path(); break; }
                    auto block=std::make_unique<Block>(); block->file=owner.track(directory/std::filesystem::u8path(name));
                    if(!read(*block)) return E_FAIL;
                    *data=block->bytes.data(); *bytes=UINT(block->bytes.size()-1);
                    blocks.push_back(std::move(block)); return S_OK;
                }
                catch(const std::bad_alloc &) { error="Shader include CPU admission denied"; return E_OUTOFMEMORY; }
                catch(const std::exception &e) { error=e.what(); return E_FAIL; }
            }
            HRESULT __stdcall Close(LPCVOID) override { return S_OK; } // Keep addresses stable for nested-parent lookup.
        };
    public:
        ShaderFilesDX11(const String &vertex,const String &pixel)
        {
            if(pixel.empty()) throw std::invalid_argument("Pixel shader file is required");
            if(!vertex.empty()) m_roots[0]=track(std::filesystem::u8path(vertex.c_str()));
            m_roots[1]=track(std::filesystem::u8path(pixel.c_str()));
        }
        bool Poll(bool force=false)
        {
            const auto now=std::chrono::steady_clock::now();
            if(!force && now<m_next) return false;
            m_next=now+std::chrono::milliseconds(250);
            bool changed=force;
            for(auto &file : m_files)
            {
                const auto current=stamp(file->path);
                if(!(current==file->observed)) { file->observed=current; changed=true; }
            }
            if(changed) ++m_revision;
            return changed;
        }
        u64 Revision() const { return m_revision; }
        String Path(size_t stage) const { return m_roots[stage] ? String(m_roots[stage]->path.u8string().c_str()) : String(); }
        bool HasStage(size_t stage) const { return bool(m_roots[stage]); }
        bool Compile(wp_shader *shader,size_t stage,Microsoft::WRL::ComPtr<ID3DBlob> &code,String &error)
        {
            if(!m_roots[stage]) return stage==0;
            Include include(*this,m_roots[stage]); Include::Block root; root.file=m_roots[stage];
            if(!include.read(root)) { error=include.error; return false; }
            std::array<D3D_SHADER_MACRO,WP_SHADER_MAX_DEFINES+1> macros{};
            for(wp_s32 i=0;i<wp_shader_get_define_count(shader);++i)
            {
                const auto define=wp_shader_get_define_at(shader,i);
                macros[i]={reinterpret_cast<const char *>(define->name),reinterpret_cast<const char *>(define->value)};
            }
            const auto entry=reinterpret_cast<const char *>(wp_shader_get_entry_point(shader));
            const auto profile=reinterpret_cast<const char *>(wp_shader_get_profile(shader));
            Microsoft::WRL::ComPtr<ID3DBlob> diagnostic;
            auto result=D3DCompile(root.bytes.data(),root.bytes.size()-1,root.file->path.u8string().c_str(),macros.data(),&include,
                *entry?entry:"main",*profile?profile:(stage?"ps_5_0":"vs_5_0"),
                D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&diagnostic);
            if(result==E_OUTOFMEMORY || include.error=="Shader include CPU admission denied") throw std::bad_alloc();
            if(FAILED(result))
            {
                error=include.error.empty() && diagnostic ? String(static_cast<const char *>(diagnostic->GetBufferPointer())) : include.error;
                if(error.empty()) error="Shader file compilation failed";
                return false;
            }
            return true;
        }
    };
}
