-- Shared 1920x1080 canvas setup for generated scene-component UI.
UICanvas = UICanvas or {}

function UICanvas.addComponent(actor, name)
    local component = assert(actor:addComponent(name), "UI component is unavailable: " .. name)
    -- Runtime-created components need an explicit state after their native load.
    component:setState(IApplicationManager.instance():isPlaying() and State.Play or State.Edit)
    return component
end

function UICanvas.bindButton(button, component, functionName)
    button:setClickHandler(component, functionName)
end

function UICanvas.center(transform)
    local center = Vector2F(0.5, 0.5)
    transform:setAnchor(center)
    transform:setAnchorMin(center)
    transform:setAnchorMax(center)
end

function UICanvas.attach(actor, interactive)
    local canvas = UICanvas.addComponent(actor, "Layout")
    local properties = canvas:getProperties()
    for _, name in ipairs({"flagBorder", "flagMovable", "flagScalable", "flagClosable",
        "flagMinimizable", "flagTitle", "flagBackground"}) do
        properties:setPropertyAsBool(name, false)
    end
    properties:setPropertyAsBool("flagNoScrollbar", true)
    properties:setPropertyAsBool("flagNoInput", not interactive)
    canvas:setProperties(properties)
    return canvas
end
