#include <EditorPCH.hpp>
#include <editor/MaterialShaderReloadActions.hpp>
#include <Workphone/Workphone.hpp>
#if WP_GRAPHICS_SYSTEM_CLAW && WP_BUILD_RENDERER_DX11
#include <WPGraphics/ClawMaterial.hpp>
#endif
#include <stdexcept>

namespace workphone::editor
{
    void processMaterialShaderReloadAction(SmartPtr<render::IMaterial> material,
                                           SmartPtr<Properties> request)
    {
        if(!request) return;
        request->setProperty("success",false);
        request->setProperty("queued",false);
        request->setProperty("available",false);
        request->setProperty("pending",false);
        request->setProperty("runtimeSuccess",false);
        try
        {
            const auto action = request->getProperty("action");
            if(action != "read" && action != "reload" && action != "setWatching" && action != "configureFiles")
                throw std::invalid_argument("Unknown material shader reload action: " + action);
            if(!material) throw std::runtime_error("Select a material before configuring shader reloads.");
#if WP_GRAPHICS_SYSTEM_CLAW && WP_BUILD_RENDERER_DX11
            auto claw = dynamic_cast<render::ClawMaterial *>(material.get());
            if(!claw) throw std::runtime_error("Runtime shader reload is unavailable for this material backend.");
            bool queued = false;
            if(action == "configureFiles")
            {
                const auto pixel = request->getProperty("pixelFile");
                const auto vertex = request->getProperty("vertexFile");
                if(pixel.empty()) throw std::invalid_argument("Choose a pixel shader file; the vertex shader is optional.");
                queued = claw->requestRuntimeShaderFiles(vertex,pixel);
            }
            else if(action == "reload") queued = claw->requestRuntimeShaderReload();
            else if(action == "setWatching")
            {
                if(!request->hasProperty("watching")) throw std::invalid_argument("The watch request requires an enabled value.");
                queued = claw->setRuntimeShaderWatching(request->getPropertyAsBool("watching"));
            }
            const auto status = claw->getRuntimeShaderReloadStatus();
            if(!status) throw std::runtime_error("The runtime shader reload status is unavailable.");
            // Request success means an action was handled/queued. Keep the
            // compiler's last result separate so queued never means compiled.
            for(const auto &property : status->getPropertiesAsArray())
                if(property.getName() != "success") request->addProperty(property);
            request->setProperty("runtimeSuccess",status->getPropertyAsBool("success"));
            request->setProperty("queued",queued);
            if(action != "read" && !queued)
                throw std::runtime_error("Shader reload request was not queued. Configure shader files or select a reload-enabled material.");
            if(action == "read" && !status->getPropertyAsBool("available"))
            {
                auto message = status->getProperty("status");
                if(message.empty()) message = "Runtime shader reload is unavailable. Configure shader files for this material.";
                request->setProperty("status",message);
                return;
            }
            request->setProperty("success",true);
#else
            throw std::runtime_error("Runtime shader reload requires the Claw DirectX 11 backend.");
#endif
        }
        catch(const std::exception &error)
        {
            request->setProperty("success",false);
            request->setProperty("status",String(error.what()));
        }
    }
}
