class 'ProceduralRenderable' (ProceduralObject)

function ProceduralRenderable:__init()
    ProceduralObject.__init(self)
end

function ProceduralRenderable:Build()
    -- Implement build logic here if needed
end

function ProceduralRenderable:Cook()
    -- Implement cook logic here if needed
end

function ProceduralRenderable:SetupRenderers()
    -- Example: get all child renderers and set shadow casting mode off
    -- This is a stub. Replace with engine-specific logic if available.
    if self.GetComponentsInChildren then
        local renderers = self:GetComponentsInChildren("Renderer")
        for _, renderer in ipairs(renderers) do
            if renderer.shadowCastingMode then
                renderer.shadowCastingMode = "Off"
            end
        end
    end
end