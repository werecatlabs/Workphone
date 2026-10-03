class 'ShaderEditor' (BaseEditor)

ShaderEditorTypes =
{
	ShaderName = 9100,
	Language = 9101,
	MaterialName = 9102,
	VertexProgram = 9103,
	FragmentProgram = 9104,
	GeometryProgram = 9105,
	VertexPath = 9106,
	FragmentPath = 9107,
	GeometryPath = 9108,
	ProgramPath = 9109,
	MaterialPath = 9110,
	PassIndex = 9111,
	TechniqueIndex = 9112,
	VertexSource = 9113,
	FragmentSource = 9114,
	GeometrySource = 9115,
	TemplateButton = 9116,
	LoadButton = 9117,
	SaveSourcesButton = 9118,
	SaveProgramButton = 9119,
	SaveMaterialButton = 9120,
	ApplyMaterialButton = 9121,
	ValidateButton = 9122,
}

local ShaderEditorDefaults =
{
	shaderName = "CustomShader",
	language = "glsl",
	materialName = "CustomShaderMaterial",
	vertexProgram = "CustomShader/Vertex",
	fragmentProgram = "CustomShader/Fragment",
	geometryProgram = "",
	vertexPath = "Assets/Shaders/CustomShader.vert",
	fragmentPath = "Assets/Shaders/CustomShader.frag",
	geometryPath = "",
	programPath = "Assets/Shaders/CustomShader.program",
	materialPath = "Assets/Materials/CustomShader.material",
	techniqueIndex = "0",
	passIndex = "0",
}

local function shaderCopyDefaults()
	local result = {};
	for key, value in pairs(ShaderEditorDefaults) do
		result[key] = value;
	end
	return result;
end

local function shaderSafeCall(object, methodName, ...)
	if object == nil or methodName == nil then
		return false, nil;
	end

	local method = object[methodName];
	if method == nil then
		return false, nil;
	end

	return pcall(method, object, ...);
end

local function shaderSafeGetText(control, fallback)
	if control == nil then
		return fallback or "";
	end

	local ok, value = shaderSafeCall(control, "getText");
	if ok and value ~= nil then
		return value;
	end

	return fallback or "";
end

local function shaderSetText(control, value)
	if control ~= nil then
		shaderSafeCall(control, "setText", value or "");
	end
end

local function shaderTrim(value)
	value = tostring(value or "");
	return value:match("^%s*(.-)%s*$");
end

local function shaderIsEmpty(value)
	return shaderTrim(value) == "";
end

local function shaderChangeExtension(path, extension)
	if shaderIsEmpty(path) then
		return path;
	end

	local slash = path:match("^.*()/") or 0;
	local backslash = path:match("^.*()\\") or 0;
	local separator = math.max(slash, backslash);
	local dot = path:match("^.*()%.") or 0;

	if dot > separator then
		return path:sub(1, dot - 1) .. extension;
	end

	return path .. extension;
end

local function shaderJoinLines(lines)
	return table.concat(lines, "\n");
end

local function shaderProgramBlock(programType, programName, sourcePath, language)
	if shaderIsEmpty(programName) or shaderIsEmpty(sourcePath) then
		return "";
	end

	return shaderJoinLines({
		programType .. "_program " .. programName .. " " .. language,
		"{",
		"    source " .. sourcePath,
		"    default_params",
		"    {",
		"    }",
		"}",
		"",
	});
end

function ShaderEditor:__init(window)
	print("ShaderEditor constructor called");

	self.window = window;
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;
	self.controls = {};
	self.settings = shaderCopyDefaults();
	self.material = nil;

	self.vertexSource = "";
	self.fragmentSource = "";
	self.geometrySource = "";
end

function ShaderEditor:__finalize()
	print("ShaderEditor __finalize called");
	self.window = nil;
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;
	self.controls = nil;
	self.settings = nil;
	self.material = nil;
end

function ShaderEditor:setStatus(message)
	print("ShaderEditor: " .. tostring(message));
	if self.statusText ~= nil then
		self.statusText:setText(tostring(message));
	end
end

function ShaderEditor:addText(parent, text)
	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local textTypeInfo = IUIText.typeInfo();
	local label = ui:addElement(textTypeInfo);
	label:setText(text or "");
	label:setSameLine(false);
	parent:addChild(label);
	return label;
end

function ShaderEditor:addEntry(parent, label, elementId, value, multiline)
	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local textEntryTypeInfo = IUITextEntry.typeInfo();
	local entry = ui:addElement(textEntryTypeInfo);
	entry:setLabel(label);
	entry:setElementId(elementId);
	entry:setText(value or "");
	entry:setSameLine(false);
	if multiline == true then
		shaderSafeCall(entry, "setMultiline", true);
	end
	parent:addChild(entry);
	self.window:setHandleEvents(entry, true);
	return entry;
end

function ShaderEditor:addButton(parent, label, elementId, sameLine)
	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local buttonTypeInfo = IUIButton.typeInfo();
	local button = ui:addElement(buttonTypeInfo);
	button:setLabel(label);
	button:setElementId(elementId);
	button:setSameLine(sameLine == true);
	parent:addChild(button);
	self.window:setHandleEvents(button, true);
	return button;
end

function ShaderEditor:addDropdown(parent, label, elementId, options, selected)
	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local dropdownTypeInfo = IUIDropdown.typeInfo();
	local dropdown = ui:addElement(dropdownTypeInfo);
	dropdown:setLabel(label);
	dropdown:setElementId(elementId);
	for _, option in ipairs(options) do
		dropdown:addOption(option);
	end
	dropdown:setSelectedOption(selected or 0);
	dropdown:setSameLine(false);
	parent:addChild(dropdown);
	self.window:setHandleEvents(dropdown, true);
	return dropdown;
end

function ShaderEditor:load()
	print("ShaderEditor load start");

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	local windowTypeInfo = IUIWindow.typeInfo();
	local tabBarTypeInfo = IUITabBar.typeInfo();
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();

	local parentWindow = self.window:getParentWindow();
	parentWindow:setSize(Vector2F(780.0, 720.0));
	parentWindow:setLabel("Shader Editor");

	self.editorWindow = ui:addElement(windowTypeInfo);
	self.editorWindow:setLabel("Shader Editor");
	self.editorWindow:setSize(Vector2F(760.0, 680.0));
	parentWindow:addChild(self.editorWindow);

	self.tabBar = ui:addElement(tabBarTypeInfo);
	self.editorWindow:addChild(self.tabBar);

	local setupTab = self.tabBar:addTabItem();
	setupTab:setLabel("Setup");

	local sourceTab = self.tabBar:addTabItem();
	sourceTab:setLabel("Source");

	local outputTab = self.tabBar:addTabItem();
	outputTab:setLabel("Output");

	local shaderHeader = ui:addElement(collapsingHeaderTypeInfo);
	shaderHeader:setLabel("Shader");
	setupTab:addChild(shaderHeader);

	self.controls.shaderName = self:addEntry(shaderHeader, "Shader Name", ShaderEditorTypes.ShaderName, self.settings.shaderName, false);
	self.controls.language = self:addDropdown(shaderHeader, "Language", ShaderEditorTypes.Language, { "glsl", "hlsl", "metal" }, 0);
	self.controls.materialName = self:addEntry(shaderHeader, "Material Name", ShaderEditorTypes.MaterialName, self.settings.materialName, false);
	self.controls.vertexProgram = self:addEntry(shaderHeader, "Vertex Program", ShaderEditorTypes.VertexProgram, self.settings.vertexProgram, false);
	self.controls.fragmentProgram = self:addEntry(shaderHeader, "Fragment Program", ShaderEditorTypes.FragmentProgram, self.settings.fragmentProgram, false);
	self.controls.geometryProgram = self:addEntry(shaderHeader, "Geometry Program", ShaderEditorTypes.GeometryProgram, self.settings.geometryProgram, false);
	self.controls.techniqueIndex = self:addEntry(shaderHeader, "Technique Index", ShaderEditorTypes.TechniqueIndex, self.settings.techniqueIndex, false);
	self.controls.passIndex = self:addEntry(shaderHeader, "Pass Index", ShaderEditorTypes.PassIndex, self.settings.passIndex, false);

	local fileHeader = ui:addElement(collapsingHeaderTypeInfo);
	fileHeader:setLabel("Files");
	setupTab:addChild(fileHeader);

	self.controls.vertexPath = self:addEntry(fileHeader, "Vertex Source", ShaderEditorTypes.VertexPath, self.settings.vertexPath, false);
	self.controls.fragmentPath = self:addEntry(fileHeader, "Fragment Source", ShaderEditorTypes.FragmentPath, self.settings.fragmentPath, false);
	self.controls.geometryPath = self:addEntry(fileHeader, "Geometry Source", ShaderEditorTypes.GeometryPath, self.settings.geometryPath, false);
	self.controls.programPath = self:addEntry(fileHeader, "Program Script", ShaderEditorTypes.ProgramPath, self.settings.programPath, false);
	self.controls.materialPath = self:addEntry(fileHeader, "Material Script", ShaderEditorTypes.MaterialPath, self.settings.materialPath, false);

	self:addButton(setupTab, "Generate Template", ShaderEditorTypes.TemplateButton, false);
	self:addButton(setupTab, "Load Sources", ShaderEditorTypes.LoadButton, true);
	self:addButton(setupTab, "Validate", ShaderEditorTypes.ValidateButton, true);

	local sourceHeader = ui:addElement(collapsingHeaderTypeInfo);
	sourceHeader:setLabel("Shader Source");
	sourceTab:addChild(sourceHeader);

	self.vertexSource = self:createVertexTemplate();
	self.fragmentSource = self:createFragmentTemplate();
	self.geometrySource = "";

	self.controls.vertexSource = self:addEntry(sourceHeader, "Vertex", ShaderEditorTypes.VertexSource, self.vertexSource, true);
	self.controls.fragmentSource = self:addEntry(sourceHeader, "Fragment", ShaderEditorTypes.FragmentSource, self.fragmentSource, true);
	self.controls.geometrySource = self:addEntry(sourceHeader, "Geometry", ShaderEditorTypes.GeometrySource, self.geometrySource, true);

	local outputHeader = ui:addElement(collapsingHeaderTypeInfo);
	outputHeader:setLabel("Write");
	outputTab:addChild(outputHeader);

	self:addButton(outputHeader, "Save Sources", ShaderEditorTypes.SaveSourcesButton, false);
	self:addButton(outputHeader, "Save Program", ShaderEditorTypes.SaveProgramButton, true);
	self:addButton(outputHeader, "Save Material", ShaderEditorTypes.SaveMaterialButton, true);
	self:addButton(outputHeader, "Apply To Material", ShaderEditorTypes.ApplyMaterialButton, true);

	self.statusText = self:addText(self.editorWindow, "Ready.");
	self:syncFromControls();
	self:setStatus("Ready.");

	print("ShaderEditor load end");
end

function ShaderEditor:unload()
	print("ShaderEditor unload called");
	if self.editorWindow ~= nil then
		self.editorWindow:setVisible(false, false);
		self.editorWindow:destroyAllChildren();
	end
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;
	self.controls = {};
end

function ShaderEditor:show()
	local parentWindow = self.window:getParentWindow();
	if parentWindow ~= nil then
		parentWindow:setVisible(true, false);
	end
	if self.editorWindow ~= nil then
		self.editorWindow:setVisible(true, false);
	end
end

function ShaderEditor:hide()
	if self.editorWindow ~= nil then
		self.editorWindow:setVisible(false, false);
	end
	local parentWindow = self.window:getParentWindow();
	if parentWindow ~= nil then
		parentWindow:setVisible(false, false);
	end
end

function ShaderEditor:syncFromControls()
	if self.controls == nil then
		return;
	end

	self.settings.shaderName = shaderSafeGetText(self.controls.shaderName, self.settings.shaderName);
	self.settings.materialName = shaderSafeGetText(self.controls.materialName, self.settings.materialName);
	self.settings.vertexProgram = shaderSafeGetText(self.controls.vertexProgram, self.settings.vertexProgram);
	self.settings.fragmentProgram = shaderSafeGetText(self.controls.fragmentProgram, self.settings.fragmentProgram);
	self.settings.geometryProgram = shaderSafeGetText(self.controls.geometryProgram, self.settings.geometryProgram);
	self.settings.vertexPath = shaderSafeGetText(self.controls.vertexPath, self.settings.vertexPath);
	self.settings.fragmentPath = shaderSafeGetText(self.controls.fragmentPath, self.settings.fragmentPath);
	self.settings.geometryPath = shaderSafeGetText(self.controls.geometryPath, self.settings.geometryPath);
	self.settings.programPath = shaderSafeGetText(self.controls.programPath, self.settings.programPath);
	self.settings.materialPath = shaderSafeGetText(self.controls.materialPath, self.settings.materialPath);
	self.settings.techniqueIndex = shaderSafeGetText(self.controls.techniqueIndex, self.settings.techniqueIndex);
	self.settings.passIndex = shaderSafeGetText(self.controls.passIndex, self.settings.passIndex);

	local languages = { "glsl", "hlsl", "metal" };
	if self.controls.language ~= nil then
		local ok, index = shaderSafeCall(self.controls.language, "getSelectedOption");
		if ok and index ~= nil then
			self.settings.language = languages[(index or 0) + 1] or self.settings.language;
		end
	end

	self.vertexSource = shaderSafeGetText(self.controls.vertexSource, self.vertexSource);
	self.fragmentSource = shaderSafeGetText(self.controls.fragmentSource, self.fragmentSource);
	self.geometrySource = shaderSafeGetText(self.controls.geometrySource, self.geometrySource);
end

function ShaderEditor:syncToControls()
	shaderSetText(self.controls.shaderName, self.settings.shaderName);
	shaderSetText(self.controls.materialName, self.settings.materialName);
	shaderSetText(self.controls.vertexProgram, self.settings.vertexProgram);
	shaderSetText(self.controls.fragmentProgram, self.settings.fragmentProgram);
	shaderSetText(self.controls.geometryProgram, self.settings.geometryProgram);
	shaderSetText(self.controls.vertexPath, self.settings.vertexPath);
	shaderSetText(self.controls.fragmentPath, self.settings.fragmentPath);
	shaderSetText(self.controls.geometryPath, self.settings.geometryPath);
	shaderSetText(self.controls.programPath, self.settings.programPath);
	shaderSetText(self.controls.materialPath, self.settings.materialPath);
	shaderSetText(self.controls.techniqueIndex, self.settings.techniqueIndex);
	shaderSetText(self.controls.passIndex, self.settings.passIndex);
	shaderSetText(self.controls.vertexSource, self.vertexSource);
	shaderSetText(self.controls.fragmentSource, self.fragmentSource);
	shaderSetText(self.controls.geometrySource, self.geometrySource);
end

function ShaderEditor:updateDerivedNames()
	local shaderName = shaderTrim(self.settings.shaderName);
	if shaderName == "" then
		shaderName = "CustomShader";
		self.settings.shaderName = shaderName;
	end

	self.settings.vertexProgram = shaderName .. "/Vertex";
	self.settings.fragmentProgram = shaderName .. "/Fragment";
	if not shaderIsEmpty(self.settings.geometryPath) then
		self.settings.geometryProgram = shaderName .. "/Geometry";
	end

	self.settings.vertexPath = shaderChangeExtension(self.settings.vertexPath, ".vert");
	self.settings.fragmentPath = shaderChangeExtension(self.settings.fragmentPath, ".frag");
	if shaderIsEmpty(self.settings.programPath) then
		self.settings.programPath = "Assets/Shaders/" .. shaderName .. ".program";
	end
	if shaderIsEmpty(self.settings.materialPath) then
		self.settings.materialPath = "Assets/Materials/" .. shaderName .. ".material";
	end
end

function ShaderEditor:createVertexTemplate()
	return shaderJoinLines({
		"#version 330 core",
		"",
		"layout(location = 0) in vec3 vertex;",
		"layout(location = 1) in vec3 normal;",
		"layout(location = 2) in vec2 uv0;",
		"",
		"uniform mat4 worldViewProj;",
		"",
		"out vec2 vUv0;",
		"out vec3 vNormal;",
		"",
		"void main()",
		"{",
		"    vUv0 = uv0;",
		"    vNormal = normal;",
		"    gl_Position = worldViewProj * vec4(vertex, 1.0);",
		"}",
	});
end

function ShaderEditor:createFragmentTemplate()
	return shaderJoinLines({
		"#version 330 core",
		"",
		"in vec2 vUv0;",
		"in vec3 vNormal;",
		"",
		"uniform sampler2D diffuseMap;",
		"uniform vec4 tint;",
		"",
		"out vec4 fragColour;",
		"",
		"void main()",
		"{",
		"    vec3 n = normalize(vNormal) * 0.5 + 0.5;",
		"    vec4 baseColour = texture(diffuseMap, vUv0) * tint;",
		"    fragColour = vec4(baseColour.rgb * n, baseColour.a);",
		"}",
	});
end

function ShaderEditor:createGeometryTemplate()
	return shaderJoinLines({
		"#version 330 core",
		"",
		"layout(triangles) in;",
		"layout(triangle_strip, max_vertices = 3) out;",
		"",
		"in vec2 vUv0[];",
		"in vec3 vNormal[];",
		"out vec2 gUv0;",
		"out vec3 gNormal;",
		"",
		"void main()",
		"{",
		"    for (int i = 0; i < 3; ++i)",
		"    {",
		"        gUv0 = vUv0[i];",
		"        gNormal = vNormal[i];",
		"        gl_Position = gl_in[i].gl_Position;",
		"        EmitVertex();",
		"    }",
		"    EndPrimitive();",
		"}",
	});
end

function ShaderEditor:generateTemplates()
	self:syncFromControls();
	self:updateDerivedNames();
	self.vertexSource = self:createVertexTemplate();
	self.fragmentSource = self:createFragmentTemplate();
	if not shaderIsEmpty(self.settings.geometryProgram) then
		self.geometrySource = self:createGeometryTemplate();
	end
	self:syncToControls();
	self:setStatus("Generated shader templates.");
end

function ShaderEditor:buildProgramScript()
	self:syncFromControls();
	local language = shaderTrim(self.settings.language);
	if language == "" then
		language = "glsl";
	end

	return shaderProgramBlock("vertex", self.settings.vertexProgram, self.settings.vertexPath, language) ..
		shaderProgramBlock("fragment", self.settings.fragmentProgram, self.settings.fragmentPath, language) ..
		shaderProgramBlock("geometry", self.settings.geometryProgram, self.settings.geometryPath, language);
end

function ShaderEditor:buildMaterialScript()
	self:syncFromControls();

	local lines =
	{
		"material " .. shaderTrim(self.settings.materialName),
		"{",
		"    technique",
		"    {",
		"        pass",
		"        {",
	};

	if not shaderIsEmpty(self.settings.vertexProgram) then
		table.insert(lines, "            vertex_program_ref " .. shaderTrim(self.settings.vertexProgram));
		table.insert(lines, "            {");
		table.insert(lines, "            }");
	end

	if not shaderIsEmpty(self.settings.fragmentProgram) then
		table.insert(lines, "            fragment_program_ref " .. shaderTrim(self.settings.fragmentProgram));
		table.insert(lines, "            {");
		table.insert(lines, "            }");
	end

	if not shaderIsEmpty(self.settings.geometryProgram) then
		table.insert(lines, "            geometry_program_ref " .. shaderTrim(self.settings.geometryProgram));
		table.insert(lines, "            {");
		table.insert(lines, "            }");
	end

	table.insert(lines, "            texture_unit");
	table.insert(lines, "            {");
	table.insert(lines, "                texture white.png");
	table.insert(lines, "            }");
	table.insert(lines, "        }");
	table.insert(lines, "    }");
	table.insert(lines, "}");
	table.insert(lines, "");

	return shaderJoinLines(lines);
end

function ShaderEditor:writeTextFile(path, contents)
	if shaderIsEmpty(path) then
		return false, "Missing file path.";
	end

	local okApp, applicationManager = pcall(function() return IApplicationManager.instance(); end);
	if okApp and applicationManager then
		local okFs, fileSystem = pcall(function() return applicationManager:getFileSystem(); end);
		if okFs and fileSystem then
			local okWrite = pcall(function() fileSystem:writeAllText(path, contents); end);
			if okWrite then
				return true, nil;
			end
		end
	end

	if io and io.open then
		local file, err = io.open(path, "w");
		if file then
			file:write(contents or "");
			file:close();
			return true, nil;
		end
		return false, err;
	end

	return false, "No writable file API available.";
end

function ShaderEditor:readTextFile(path)
	if shaderIsEmpty(path) then
		return false, "Missing file path.";
	end

	local okApp, applicationManager = pcall(function() return IApplicationManager.instance(); end);
	if okApp and applicationManager then
		local okFs, fileSystem = pcall(function() return applicationManager:getFileSystem(); end);
		if okFs and fileSystem then
			local okRead, contents = pcall(function() return fileSystem:readAllText(path); end);
			if okRead then
				return true, contents;
			end
		end
	end

	if io and io.open then
		local file, err = io.open(path, "r");
		if file then
			local contents = file:read("*a");
			file:close();
			return true, contents;
		end
		return false, err;
	end

	return false, "No readable file API available.";
end

function ShaderEditor:saveSources()
	self:syncFromControls();
	local okVertex, vertexError = self:writeTextFile(self.settings.vertexPath, self.vertexSource);
	local okFragment, fragmentError = self:writeTextFile(self.settings.fragmentPath, self.fragmentSource);
	local okGeometry = true;
	local geometryError = nil;

	if not shaderIsEmpty(self.settings.geometryPath) and not shaderIsEmpty(self.geometrySource) then
		okGeometry, geometryError = self:writeTextFile(self.settings.geometryPath, self.geometrySource);
	end

	if okVertex and okFragment and okGeometry then
		self:setStatus("Saved shader source files.");
		return true;
	end

	self:setStatus("Save source failed: " .. tostring(vertexError or fragmentError or geometryError));
	return false;
end

function ShaderEditor:loadSources()
	self:syncFromControls();

	local okVertex, vertexData = self:readTextFile(self.settings.vertexPath);
	if okVertex and vertexData ~= nil then
		self.vertexSource = vertexData;
	end

	local okFragment, fragmentData = self:readTextFile(self.settings.fragmentPath);
	if okFragment and fragmentData ~= nil then
		self.fragmentSource = fragmentData;
	end

	if not shaderIsEmpty(self.settings.geometryPath) then
		local okGeometry, geometryData = self:readTextFile(self.settings.geometryPath);
		if okGeometry and geometryData ~= nil then
			self.geometrySource = geometryData;
		end
	end

	self:syncToControls();
	if okVertex or okFragment then
		self:setStatus("Loaded shader source files.");
	else
		self:setStatus("No shader source files loaded.");
	end
end

function ShaderEditor:saveProgramScript()
	local contents = self:buildProgramScript();
	local ok, err = self:writeTextFile(self.settings.programPath, contents);
	if ok then
		self:setStatus("Saved program script.");
	else
		self:setStatus("Save program failed: " .. tostring(err));
	end
	return ok;
end

function ShaderEditor:saveMaterialScript()
	local contents = self:buildMaterialScript();
	local ok, err = self:writeTextFile(self.settings.materialPath, contents);
	if ok then
		self:setStatus("Saved material script.");
	else
		self:setStatus("Save material failed: " .. tostring(err));
	end
	return ok;
end

function ShaderEditor:validate()
	self:syncFromControls();

	local errors = {};
	if shaderIsEmpty(self.settings.shaderName) then table.insert(errors, "Shader name is empty."); end
	if shaderIsEmpty(self.settings.vertexProgram) then table.insert(errors, "Vertex program is empty."); end
	if shaderIsEmpty(self.settings.fragmentProgram) then table.insert(errors, "Fragment program is empty."); end
	if shaderIsEmpty(self.settings.vertexPath) then table.insert(errors, "Vertex path is empty."); end
	if shaderIsEmpty(self.settings.fragmentPath) then table.insert(errors, "Fragment path is empty."); end
	if shaderIsEmpty(self.vertexSource) then table.insert(errors, "Vertex source is empty."); end
	if shaderIsEmpty(self.fragmentSource) then table.insert(errors, "Fragment source is empty."); end

	if #errors == 0 then
		self:setStatus("Validation passed.");
		return true;
	end

	self:setStatus(errors[1]);
	return false;
end

function ShaderEditor:getMaterialPass()
	if self.material == nil then
		return nil;
	end

	self:syncFromControls();
	local techniqueIndex = tonumber(self.settings.techniqueIndex) or 0;
	local passIndex = tonumber(self.settings.passIndex) or 0;

	local okTechnique, technique = shaderSafeCall(self.material, "getTechnique", techniqueIndex);
	if not okTechnique or technique == nil then
		return nil;
	end

	local okPass, pass = shaderSafeCall(technique, "getPass", passIndex);
	if okPass and pass ~= nil then
		return pass;
	end

	local okPasses, passes = shaderSafeCall(technique, "getPasses");
	if okPasses and passes ~= nil then
		local okAt, indexedPass = pcall(function() return passes:at(passIndex); end);
		if okAt and indexedPass ~= nil then
			return indexedPass;
		end
	end

	return nil;
end

function ShaderEditor:applyToMaterial()
	self:syncFromControls();
	local pass = self:getMaterialPass();
	if pass == nil then
		self:setStatus("No material pass selected.");
		return false;
	end

	shaderSafeCall(pass, "setVertexShaderName", self.settings.vertexProgram);
	shaderSafeCall(pass, "setFragmentShaderName", self.settings.fragmentProgram);
	shaderSafeCall(pass, "setGeometryShaderName", self.settings.geometryProgram);
	self:setStatus("Applied shader program names to material pass.");
	return true;
end

function ShaderEditor:setMaterial(material)
	self.material = material;
end

function ShaderEditor:handleEvent(parameters, results)
	if parameters == nil then
		return;
	end

	local sender = parameters:at(3);
	if sender == nil then
		return;
	end

	local elementId = sender:getElementId();

	if elementId == ShaderEditorTypes.TemplateButton then
		self:generateTemplates();
	elseif elementId == ShaderEditorTypes.LoadButton then
		self:loadSources();
	elseif elementId == ShaderEditorTypes.SaveSourcesButton then
		self:saveSources();
	elseif elementId == ShaderEditorTypes.SaveProgramButton then
		self:saveProgramScript();
	elseif elementId == ShaderEditorTypes.SaveMaterialButton then
		self:saveMaterialScript();
	elseif elementId == ShaderEditorTypes.ApplyMaterialButton then
		self:applyToMaterial();
	elseif elementId == ShaderEditorTypes.ValidateButton then
		self:validate();
	else
		self:syncFromControls();
	end
end

function ShaderEditor:getProperties(parameters)
	local properties = parameters:at(0);
	if properties == nil then
		return;
	end

	self:syncFromControls();
	for key, value in pairs(self.settings) do
		shaderSafeCall(properties, "setProperty", key, tostring(value));
	end
end

function ShaderEditor:setProperties(parameters)
	if parameters == nil then
		return;
	end

	local properties = parameters:at(0);
	if properties == nil then
		return;
	end
end
