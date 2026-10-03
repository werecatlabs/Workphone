include("BaseEditor.lua")

class 'AnimationEditor' (BaseEditor)

AnimationEditorControlKind =
{
	Value = "value",
	Toggle = "toggle",
	Dropdown = "dropdown",
	Text = "text",
}

AnimationEditorTypes =
{
	RefreshSelection = 1000,
	AddAnimator = 1001,
	Clip = 1002,
	ClipName = 1003,
	ClipLength = 1004,
	ClipSpeed = 1005,
	Looping = 1006,
	Time = 1007,
	Play = 1008,
	Pause = 1009,
	Stop = 1010,
	Rewind = 1011,
	Apply = 1012,
	AddClip = 1013,
	RemoveClip = 1014,
	DuplicateClip = 1015,
	ImportPath = 1016,
	ImportClip = 1017,
	PreviewSkeleton = 1018,
	PreviewRootMotion = 1019,
}

local AnimationEditorDefaults =
{
	selectedClip = 0,
	clipName = "Clip 01",
	clipLength = 1.0,
	clipSpeed = 1.0,
	looping = true,
	time = 0.0,
	importPath = "",
	previewSkeleton = false,
	previewRootMotion = false,
}

local function copyTable(source)
	local result = {};
	for key, value in pairs(source) do
		result[key] = value;
	end
	return result;
end

local function clamp(value, minValue, maxValue)
	value = tonumber(value) or minValue;
	if value < minValue then return minValue; end
	if value > maxValue then return maxValue; end
	return value;
end

local function safeCall(object, methodName, ...)
	if object == nil or methodName == nil then
		return false, nil;
	end

	local okMethod, method = pcall(function()
		return object[methodName];
	end);

	if not okMethod or type(method) ~= "function" then
		return false, nil;
	end

	local args = { ... };
	local okCall, result = pcall(function()
		local unpackFn = unpack or table.unpack;
		return method(object, unpackFn(args));
	end);

	if not okCall then
		print("AnimationEditor optional call failed: " .. tostring(methodName) .. " - " .. tostring(result));
		return false, nil;
	end

	return true, result;
end

local function safeGet(object, methodName, defaultValue)
	local ok, value = safeCall(object, methodName);
	if ok and value ~= nil then
		return value;
	end
	return defaultValue;
end

local function safeArg(args, index, defaultValue)
	if args == nil then
		return defaultValue;
	end

	local ok, value = pcall(function()
		return args:at(index);
	end);

	if ok and value ~= nil then
		return value;
	end

	return defaultValue;
end

local function safeParameterAt(parameters, index, defaultValue)
	if parameters == nil then
		return defaultValue;
	end

	local ok, value = pcall(function()
		return parameters:at(index);
	end);

	if ok and value ~= nil then
		return value;
	end

	return defaultValue;
end

local function addText(ui, parent, text)
	local control = ui:addElement(IUIText.typeInfo());
	control:setText(text);
	control:setSameLine(false);
	parent:addChild(control);
	return control;
end

local function addButton(ui, parent, label, elementId, sameLine)
	local control = ui:addElement(IUIButton.typeInfo());
	control:setLabel(label);
	control:setElementId(elementId);
	control:setSameLine(sameLine == true);
	parent:addChild(control);
	return control;
end

local function addToggle(ui, parent, label, elementId, value)
	local control = ui:addElement(IUILabelTogglePair.typeInfo());
	control:setLabel(label);
	control:setElementId(elementId);
	control:setValue(value == true);
	control:setSameLine(false);
	parent:addChild(control);
	return control;
end

local function addSlider(ui, parent, label, elementId, minValue, maxValue, value)
	local control = ui:addElement(IUILabelSliderPair.typeInfo());
	control:setLabel(label);
	control:setElementId(elementId);
	control:setMinValue(minValue);
	control:setMaxValue(maxValue);
	control:setValue(value or minValue);
	control:setSameLine(false);
	parent:addChild(control);
	return control;
end

local function addEntry(ui, parent, label, elementId, text)
	local control = ui:addElement(IUITextEntry.typeInfo());
	control:setLabel(label);
	control:setElementId(elementId);
	control:setText(text or "");
	control:setSameLine(false);
	parent:addChild(control);
	return control;
end

local function addDropdown(ui, parent, label, elementId, options, selected)
	local control = ui:addElement(IUIDropdown.typeInfo());
	control:setLabel(label);
	control:setElementId(elementId);
	for _, option in ipairs(options or {}) do
		control:addOption(option);
	end
	control:setSelectedOption(selected or 0);
	control:setSameLine(false);
	parent:addChild(control);
	return control;
end

function AnimationEditor:__init(window)
	print("AnimationEditor constructor called");
	BaseEditor:__init(self, window);

	self.window = window;
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;

	self.settings = copyTable(AnimationEditorDefaults);
	self.clips =
	{
		{ name = "Clip 01", length = 1.0, speed = 1.0, looping = true },
	};

	self.controls = {};
	self.bindings = {};
	self.bindingsById = {};
	self.buttonsById = {};

	self.selectedActor = nil;
	self.animator = nil;
	self.graphicsAnimationController = nil;
	self.runtimeClipCount = 0;
	self.isPlaying = false;
	self.isPaused = false;
	self.lastStatus = "Select an actor with an Animator component, or add one.";
end

function AnimationEditor:__finalize()
	print("AnimationEditor __finalize called");
	BaseEditor:__finalize();
	self:_clearRefs();
end

function AnimationEditor:_clearRefs()
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;
	self.clipDropdown = nil;
	self.clipNameEntry = nil;
	self.clipLengthSlider = nil;
	self.clipSpeedSlider = nil;
	self.loopToggle = nil;
	self.timeSlider = nil;
	self.importPathEntry = nil;
	self.previewSkeletonToggle = nil;
	self.previewRootMotionToggle = nil;
	self.controls = {};
	self.bindings = {};
	self.bindingsById = {};
	self.buttonsById = {};
end

function AnimationEditor:setStatus(text)
	self.lastStatus = text or "";
	if self.statusText ~= nil then
		self.statusText:setText(self.lastStatus);
	end
	print("AnimationEditor status: " .. tostring(self.lastStatus));
end

function AnimationEditor:registerControl(control, key, kind)
	if control == nil then
		return control;
	end

	self.controls[#self.controls + 1] = control;
	local binding =
	{
		control = control,
		key = key,
		kind = kind or AnimationEditorControlKind.Value,
	};

	self.bindings[control] = binding;

	local okId, elementId = safeCall(control, "getElementId");
	if okId and elementId ~= nil then
		self.bindingsById[elementId] = binding;
	end

	if self.window ~= nil then
		self.window:setHandleEvents(control, true);
	end

	return control;
end

function AnimationEditor:registerButton(control, action)
	if control == nil then
		return control;
	end

	local okId, elementId = safeCall(control, "getElementId");
	if okId and elementId ~= nil then
		self.buttonsById[elementId] = action;
	end

	if self.window ~= nil then
		self.window:setHandleEvents(control, true);
	end

	return control;
end

function AnimationEditor:getControlValue(control, binding, args)
	if control == nil or binding == nil then
		return nil;
	end

	if binding.kind == AnimationEditorControlKind.Dropdown then
		return safeGet(control, "getSelectedOption", safeArg(args, 0, 0));
	elseif binding.kind == AnimationEditorControlKind.Text then
		return safeGet(control, "getText", safeArg(args, 0, ""));
	elseif binding.kind == AnimationEditorControlKind.Toggle then
		local argValue = safeArg(args, 0, nil);
		if argValue ~= nil then
			return argValue == true;
		end
		return safeGet(control, "getValue", false) == true;
	end

	return safeGet(control, "getValue", safeArg(args, 0, 0.0));
end

function AnimationEditor:setControlValue(control, kind, value)
	if control == nil or value == nil then
		return;
	end

	if kind == AnimationEditorControlKind.Dropdown then
		safeCall(control, "setSelectedOption", value);
	elseif kind == AnimationEditorControlKind.Text then
		safeCall(control, "setText", value);
	elseif kind == AnimationEditorControlKind.Toggle then
		safeCall(control, "setValue", value == true);
	else
		safeCall(control, "setValue", value);
	end
end

function AnimationEditor:getSelectedClip()
	local index = (self.settings.selectedClip or 0) + 1;
	if self.clips[index] == nil then
		self.clips[index] = { name = "Clip " .. tostring(index), length = 1.0, speed = 1.0, looping = true };
	end
	return self.clips[index];
end

function AnimationEditor:syncClipToSettings()
	local clip = self:getSelectedClip();
	self.settings.clipName = clip.name or "Clip";
	self.settings.clipLength = clip.length or 1.0;
	self.settings.clipSpeed = clip.speed or self.settings.clipSpeed or 1.0;
	self.settings.looping = clip.looping == true;

	if self.animator ~= nil then
		self.settings.time = safeGet(self.animator, "getAnimationTime", self.settings.time or 0.0);
		self.settings.clipSpeed = safeGet(self.animator, "getAnimationSpeed", self.settings.clipSpeed or 1.0);
		self.settings.looping = safeGet(self.animator, "isLooping", self.settings.looping == true);
		local length = safeGet(self.animator, "getSelectedAnimationLength", 0.0);
		if length ~= nil and length > 0.0 then
			self.settings.clipLength = length;
			clip.length = length;
		end
	end
end

function AnimationEditor:syncSettingsToClip()
	local clip = self:getSelectedClip();
	clip.name = self.settings.clipName or clip.name or "Clip";
	clip.length = clamp(self.settings.clipLength or clip.length or 1.0, 0.001, 3600.0);
	clip.speed = clamp(self.settings.clipSpeed or clip.speed or 1.0, 0.0, 16.0);
	clip.looping = self.settings.looping == true;
end

function AnimationEditor:syncToControls()
	for control, binding in pairs(self.bindings) do
		if binding ~= nil then
			self:setControlValue(control, binding.kind, self.settings[binding.key]);
		end
	end
end

function AnimationEditor:syncFromControls()
	for control, binding in pairs(self.bindings) do
		if binding ~= nil then
			local value = self:getControlValue(control, binding, nil);
			if value ~= nil then
				self.settings[binding.key] = value;
			end
		end
	end
	self:syncSettingsToClip();
end

function AnimationEditor:refreshClipDropdown()
	if self.clipDropdown == nil then
		return;
	end

	local options = Parameters();
	local count = math.max(#self.clips, self.runtimeClipCount or 0);
	
	if count < 1 then
		count = 1;
	end

	for i = 1, count do
		local clip = self.clips[i];
		if clip == nil then
			clip = { name = "Clip " .. tostring(i), length = 1.0, speed = 1.0, looping = true };
			self.clips[i] = clip;
		end
		options:push_back(clip.name or ("Clip " .. tostring(i)));
	end

	local selected = clamp(self.settings.selectedClip or 0, 0, count - 1);
	self.settings.selectedClip = selected;

	self.clipDropdown:setOptions(options:getAsStringArray());

	self.clipDropdown:setSelectedOption(selected);
end

function AnimationEditor:updateSelection()
	print("AnimationEditor updateSelection called");

	self.selectedActor = nil;
	self.animator = nil;
	self.graphicsAnimationController = nil;
	self.runtimeClipCount = 0;

	local applicationManager = IApplicationManager.instance();
	local selectionManager = applicationManager:getSelectionManager();
	if selectionManager == nil then
		self:setStatus("No selection manager is available.");
		return;
	end

	local selection = selectionManager:getSelection();
	if selection == nil then
		self:setStatus("Nothing selected.");
		return;
	end

	local animatorTypeInfo = Animator.typeInfo();
	local actorTypeInfo = IActor.typeInfo();
	local graphicsMeshTypeInfo = IGraphicsMesh.typeInfo();

	local size = selection:size();
	for i = 0, size - 1 do
		local item = selection:at(i);
		if item ~= nil and item:derived(animatorTypeInfo) then
			self.animator = item;
			break;
		elseif item ~= nil and actorTypeInfo ~= nil and item:derived(actorTypeInfo) then
			self.selectedActor = item;
			local okAnimator, animator = safeCall(item, "getComponent", "Animator");
			if okAnimator and animator ~= nil then
				self.animator = animator;
				break;
			end
		elseif item ~= nil and graphicsMeshTypeInfo ~= nil and item:derived(graphicsMeshTypeInfo) then
			local okController, controller = safeCall(item, "getAnimationController");
			if okController and controller ~= nil then
				self.graphicsAnimationController = controller;
				break;
			end
		end
	end

	if self.animator ~= nil then
		self.runtimeClipCount = safeGet(self.animator, "getNumAnimationClips", self.runtimeClipCount or 0);

		local okAnimations, animations = safeCall(self.animator, "getAnimations");
		if okAnimations and animations ~= nil then
			local animationCount = safeGet(animations, "size", 0);
			self.runtimeClipCount = math.max(self.runtimeClipCount or 0, animationCount);
			for i = 0, animationCount - 1 do
				local okAnimation, animation = safeCall(animations, "at", i);
				if okAnimation and animation ~= nil then
					local clipName = safeGet(animation, "getName", "Clip " .. tostring(i + 1));
					if clipName == nil or clipName == "" then
						clipName = "Clip " .. tostring(i + 1);
					end
					self.clips[i + 1] =
					{
						name = clipName,
						length = safeGet(animation, "getLength", 1.0),
						speed = safeGet(self.animator, "getAnimationSpeed", 1.0),
						looping = safeGet(self.animator, "isLooping", true) == true,
					};
				end
			end
		end

		local okClips, clips = safeCall(self.animator, "getAnimationClips");
		if okClips and clips ~= nil then
			local clipCount = safeGet(clips, "size", 0);
			self.runtimeClipCount = math.max(self.runtimeClipCount or 0, clipCount);
			for i = 0, clipCount - 1 do
				local okClip, clip = safeCall(clips, "at", i);
				if okClip and clip ~= nil then
					self.clips[i + 1] =
					{
						name = safeGet(clip, "getName", "Clip " .. tostring(i + 1)),
						length = safeGet(clip, "getLength", 1.0),
						speed = safeGet(clip, "getSpeed", 1.0),
						looping = safeGet(clip, "isLooping", true) == true,
					};
				end
			end
		end

		self.settings.selectedClip = safeGet(self.animator, "getSelectedAnimationIndex", self.settings.selectedClip or 0);
		self:syncClipToSettings();
		self:refreshClipDropdown();
		self:syncToControls();
		self:setStatus("Animator selected with " .. tostring(self.runtimeClipCount) .. " runtime clip(s).");
	elseif self.graphicsAnimationController ~= nil then
		self:setStatus("Graphics animation controller selected.");
	else
		self:refreshClipDropdown();
		self:syncToControls();
		self:setStatus("No Animator selected.");
	end
end

function AnimationEditor:ensureAnimator()
	if self.animator ~= nil then
		return self.animator;
	end

	self:updateSelection();
	if self.animator ~= nil then
		return self.animator;
	end

	if self.selectedActor ~= nil then
		local ok, animator = safeCall(self.selectedActor, "addComponent", "Animator");
		if ok and animator ~= nil then
			self.animator = animator;
			self:setStatus("Animator component added to selected actor.");
			return animator;
		end
	end

	self:setStatus("Select an actor before adding an Animator.");
	return nil;
end

function AnimationEditor:applySettingsToAnimator()
	self:syncFromControls();
	local animator = self:ensureAnimator();
	if animator == nil then
		return false;
	end

	local clipIndex = self.settings.selectedClip or 0;
	safeCall(animator, "setSelectedAnimationIndex", clipIndex);
	safeCall(animator, "setAnimationSpeed", self.settings.clipSpeed or 1.0);
	safeCall(animator, "setLooping", self.settings.looping == true);
	safeCall(animator, "setAnimationTime", self.settings.time or 0.0);

	self:setStatus("Applied animation settings.");
	return true;
end

function AnimationEditor:performAction(action)
	if action == "refreshSelection" then
		self:updateSelection();
	elseif action == "addAnimator" then
		self:ensureAnimator();
	elseif action == "play" then
		if self:applySettingsToAnimator() then
			safeCall(self.animator, "play");
			self.isPlaying = true;
			self.isPaused = false;
			self:setStatus("Playing " .. tostring(self:getSelectedClip().name));
		end
	elseif action == "pause" then
		if self.animator ~= nil then
			safeCall(self.animator, "pause");
			self.isPaused = true;
			self:setStatus("Paused.");
		end
	elseif action == "stop" then
		if self.animator ~= nil then
			safeCall(self.animator, "stop");
		end
		self.settings.time = 0.0;
		self.isPlaying = false;
		self.isPaused = false;
		self:syncToControls();
		self:setStatus("Stopped.");
	elseif action == "rewind" then
		self.settings.time = 0.0;
		if self.animator ~= nil then
			safeCall(self.animator, "setAnimationTime", 0.0);
		end
		self:syncToControls();
		self:setStatus("Rewound.");
	elseif action == "apply" then
		self:applySettingsToAnimator();
	elseif action == "addClip" then
		local index = #self.clips + 1;
		self.clips[index] =
		{
			name = "Clip " .. tostring(index),
			length = self.settings.clipLength or 1.0,
			speed = self.settings.clipSpeed or 1.0,
			looping = self.settings.looping == true,
		};
		self.settings.selectedClip = index - 1;
		self:syncClipToSettings();
		self:refreshClipDropdown();
		self:syncToControls();
		self:setStatus("Clip metadata added.");
	elseif action == "duplicateClip" then
		local clip = self:getSelectedClip();
		local index = #self.clips + 1;
		self.clips[index] =
		{
			name = tostring(clip.name or "Clip") .. " Copy",
			length = clip.length or 1.0,
			speed = clip.speed or 1.0,
			looping = clip.looping == true,
		};
		self.settings.selectedClip = index - 1;
		self:syncClipToSettings();
		self:refreshClipDropdown();
		self:syncToControls();
		self:setStatus("Clip metadata duplicated.");
	elseif action == "removeClip" then
		if #self.clips > 1 then
			table.remove(self.clips, (self.settings.selectedClip or 0) + 1);
			self.settings.selectedClip = clamp(self.settings.selectedClip or 0, 0, #self.clips - 1);
			self:syncClipToSettings();
			self:refreshClipDropdown();
			self:syncToControls();
			self:setStatus("Clip metadata removed.");
		else
			self:setStatus("At least one clip entry is kept.");
		end
	elseif action == "importClip" then
		local path = self.settings.importPath or "";
		if path == "" then
			self:setStatus("No import path set.");
		else
			local clip = self:getSelectedClip();
			clip.name = path;
			self.settings.clipName = path;
			self:refreshClipDropdown();
			self:syncToControls();
			self:setStatus("Import path assigned to clip metadata.");
		end
	end
end

function AnimationEditor:load()
	print("AnimationEditor load called");

	self:_clearRefs();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local parentWindow = self.window:getParentWindow();
	if parentWindow == nil then
		return;
	end

	parentWindow:setSize(Vector2F(760.0, 620.0));

	self.editorWindow = ui:addElement(IUIWindow.typeInfo());
	self.editorWindow:setLabel("Animation Editor");
	self.editorWindow:setSize(Vector2F(760.0, 620.0));
	parentWindow:addChild(self.editorWindow);

	self.tabBar = ui:addElement(IUITabBar.typeInfo());
	local clipsTab = self.tabBar:addTabItem(); clipsTab:setLabel("Clips");
	local playbackTab = self.tabBar:addTabItem(); playbackTab:setLabel("Playback");
	local importTab = self.tabBar:addTabItem(); importTab:setLabel("Import");
	local debugTab = self.tabBar:addTabItem(); debugTab:setLabel("Debug");
	self.editorWindow:addChild(self.tabBar);

	local clipsHeader = ui:addElement(IUICollapsingHeader.typeInfo());
	clipsHeader:setLabel("Selection");
	clipsTab:addChild(clipsHeader);
	self:registerButton(addButton(ui, clipsHeader, "Refresh Selection", AnimationEditorTypes.RefreshSelection, false), "refreshSelection");
	self:registerButton(addButton(ui, clipsHeader, "Add Animator", AnimationEditorTypes.AddAnimator, true), "addAnimator");
	self.clipDropdown = self:registerControl(addDropdown(ui, clipsHeader, "Clip", AnimationEditorTypes.Clip, { "Clip 01" }, 0), "selectedClip", AnimationEditorControlKind.Dropdown);
	self.clipNameEntry = self:registerControl(addEntry(ui, clipsHeader, "Name", AnimationEditorTypes.ClipName, self.settings.clipName), "clipName", AnimationEditorControlKind.Text);
	self.clipLengthSlider = self:registerControl(addSlider(ui, clipsHeader, "Length", AnimationEditorTypes.ClipLength, 0.001, 3600.0, self.settings.clipLength), "clipLength", AnimationEditorControlKind.Value);
	self:registerButton(addButton(ui, clipsHeader, "Add Clip", AnimationEditorTypes.AddClip, false), "addClip");
	self:registerButton(addButton(ui, clipsHeader, "Duplicate", AnimationEditorTypes.DuplicateClip, true), "duplicateClip");
	self:registerButton(addButton(ui, clipsHeader, "Remove", AnimationEditorTypes.RemoveClip, true), "removeClip");

	local playbackHeader = ui:addElement(IUICollapsingHeader.typeInfo());
	playbackHeader:setLabel("Transport");
	playbackTab:addChild(playbackHeader);
	self.timeSlider = self:registerControl(addSlider(ui, playbackHeader, "Time", AnimationEditorTypes.Time, 0.0, 3600.0, self.settings.time), "time", AnimationEditorControlKind.Value);
	self.clipSpeedSlider = self:registerControl(addSlider(ui, playbackHeader, "Speed", AnimationEditorTypes.ClipSpeed, 0.0, 16.0, self.settings.clipSpeed), "clipSpeed", AnimationEditorControlKind.Value);
	self.loopToggle = self:registerControl(addToggle(ui, playbackHeader, "Loop", AnimationEditorTypes.Looping, self.settings.looping), "looping", AnimationEditorControlKind.Toggle);
	self:registerButton(addButton(ui, playbackHeader, "Play", AnimationEditorTypes.Play, false), "play");
	self:registerButton(addButton(ui, playbackHeader, "Pause", AnimationEditorTypes.Pause, true), "pause");
	self:registerButton(addButton(ui, playbackHeader, "Stop", AnimationEditorTypes.Stop, true), "stop");
	self:registerButton(addButton(ui, playbackHeader, "Rewind", AnimationEditorTypes.Rewind, true), "rewind");
	self:registerButton(addButton(ui, playbackHeader, "Apply", AnimationEditorTypes.Apply, false), "apply");

	local importHeader = ui:addElement(IUICollapsingHeader.typeInfo());
	importHeader:setLabel("Import");
	importTab:addChild(importHeader);
	self.importPathEntry = self:registerControl(addEntry(ui, importHeader, "Animation Asset", AnimationEditorTypes.ImportPath, self.settings.importPath), "importPath", AnimationEditorControlKind.Text);
	if self.window ~= nil then
		self.window:setDroppable(self.importPathEntry, true);
	end
	self:registerButton(addButton(ui, importHeader, "Assign Import Path", AnimationEditorTypes.ImportClip, false), "importClip");
	self.previewSkeletonToggle = self:registerControl(addToggle(ui, importHeader, "Preview Skeleton", AnimationEditorTypes.PreviewSkeleton, self.settings.previewSkeleton), "previewSkeleton", AnimationEditorControlKind.Toggle);
	self.previewRootMotionToggle = self:registerControl(addToggle(ui, importHeader, "Preview Root Motion", AnimationEditorTypes.PreviewRootMotion, self.settings.previewRootMotion), "previewRootMotion", AnimationEditorControlKind.Toggle);

	local debugHeader = ui:addElement(IUICollapsingHeader.typeInfo());
	debugHeader:setLabel("Runtime");
	debugTab:addChild(debugHeader);
	addText(ui, debugHeader, "Runtime clips are read from the selected Animator component.");
	self.statusText = addText(ui, debugHeader, self.lastStatus);

	self:refreshClipDropdown();
	self:syncToControls();
	self:updateSelection();
end

function AnimationEditor:unload()
	print("AnimationEditor unload called");

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	if self.animator ~= nil then
		safeCall(self.animator, "stop");
	end

	if self.editorWindow ~= nil then
		self.editorWindow:setVisible(false, false);
		self.editorWindow:destroyAllChildren();

		local parent = self.editorWindow:getParent();
		if parent ~= nil then
			parent:removeChild(self.editorWindow);
		end

		ui:removeElement(self.editorWindow);
	end

	self:_clearRefs();
	self.animator = nil;
	self.graphicsAnimationController = nil;
	self.selectedActor = nil;
end

function AnimationEditor:show()
	local parentWindow = self.window:getParentWindow();
	if parentWindow ~= nil then
		parentWindow:setVisible(true, false);
	end

	if self.editorWindow ~= nil then
		self.editorWindow:setVisible(true, false);
	end
end

function AnimationEditor:hide()
	if self.editorWindow ~= nil then
		self.editorWindow:setVisible(false, false);
	end

	local parentWindow = self.window:getParentWindow();
	if parentWindow ~= nil then
		parentWindow:setVisible(false, false);
	end
end

function AnimationEditor:onSettingChanged(sender, args, elementId)
	local binding = self.bindings[sender];
	if binding == nil and elementId ~= nil then
		binding = self.bindingsById[elementId];
	end

	if binding == nil then
		return;
	end

	local value = self:getControlValue(sender, binding, args);
	if value == nil then
		return;
	end

	self.settings[binding.key] = value;

	if binding.key == "selectedClip" then
		if self.animator ~= nil then
			safeCall(self.animator, "setSelectedAnimationIndex", value);
		end
		self:syncClipToSettings();
		self:syncToControls();
	elseif binding.key == "time" then
		if self.animator ~= nil then
			safeCall(self.animator, "setAnimationTime", value);
		end
	elseif binding.key == "clipSpeed" then
		if self.animator ~= nil then
			safeCall(self.animator, "setAnimationSpeed", value);
		end
		self:syncSettingsToClip();
	elseif binding.key == "looping" then
		if self.animator ~= nil then
			safeCall(self.animator, "setLooping", value == true);
		end
		self:syncSettingsToClip();
	else
		self:syncSettingsToClip();
	end

	self:setStatus("Changed: " .. tostring(binding.key));
end

function AnimationEditor:handleDrop(args)
	local dataStr = safeArg(args, 0, nil);
	
	if dataStr == nil then
		return;
	end

	local path = dataStr;
	local okJson, cjson = pcall(require, "cjson");
	if okJson and cjson ~= nil then
		local okDecode, decoded = pcall(function()
			return cjson.decode(dataStr);
		end);
		if okDecode and decoded ~= nil then
			path = decoded.filePath or decoded.path or path;
		end
	end

	self.settings.importPath = path;
	
	if self.importPathEntry ~= nil then
		self.importPathEntry:setText(path);
	end
	
	self:setStatus("Import path set: " .. tostring(path));
end

function AnimationEditor:handleEvent(parameters, results)
	if self.window == nil or not self.window:isWindowVisible() then
		return;
	end

	local eventHash = safeParameterAt(parameters, 1, nil);
	local args = safeParameterAt(parameters, 2, nil);
	local sender = safeParameterAt(parameters, 3, nil);
	
	if sender == nil then
		return;
	end

	local okId, elementId = safeCall(sender, "getElementId");
	
	if not okId then
		return;
	end

	if eventHash == IEvent.handleSelection then
		local action = self.buttonsById[elementId];
		if action ~= nil then
			self:performAction(action);
		else
			self:onSettingChanged(sender, args, elementId);
		end
	elseif eventHash == IEvent.handleValueChanged or eventHash == IEvent.handlePropertyChanged then
		self:onSettingChanged(sender, args, elementId);
	elseif eventHash == IEvent.handleDrop then
		self:handleDrop(args);
	end

	if results ~= nil then
		results["settings"] = self.settings;
		results["status"] = self.lastStatus;
	end
end

function AnimationEditor:update()
	if self.animator ~= nil then
		self.isPlaying = safeGet(self.animator, "isPlaying", self.isPlaying == true) == true;
		self.isPaused = safeGet(self.animator, "isPaused", self.isPaused == true) == true;
		self.settings.time = safeGet(self.animator, "getAnimationTime", self.settings.time or 0.0);

		local length = safeGet(self.animator, "getSelectedAnimationLength", self.settings.clipLength or 1.0);
		if length ~= nil and length > 0.0 then
			self.settings.clipLength = length;
		end

		if self.timeSlider ~= nil then
			self.timeSlider:setMaxValue(math.max(0.001, self.settings.clipLength or 1.0));
			self.timeSlider:setValue(self.settings.time or 0.0);
		end
	end
end

function AnimationEditor:getProperties(parameters)
	local properties = safeParameterAt(parameters, 0, nil);
	if properties == nil then
		return;
	end

	self:syncFromControls();
	for key, value in pairs(self.settings) do
		if type(value) == "boolean" then
			properties:setPropertyAsBool(key, value);
		elseif type(value) == "number" then
			properties:setPropertyAsFloat(key, value);
		elseif type(value) == "string" then
			properties:setPropertyAsString(key, value);
		end
	end

	properties:setPropertyAsFloat("clipCount", #self.clips);
	for index, clip in ipairs(self.clips) do
		local prefix = "clip" .. tostring(index) .. ".";
		properties:setPropertyAsString(prefix .. "name", clip.name or "");
		properties:setPropertyAsFloat(prefix .. "length", clip.length or 1.0);
		properties:setPropertyAsFloat(prefix .. "speed", clip.speed or 1.0);
		properties:setPropertyAsBool(prefix .. "looping", clip.looping == true);
	end
end

function AnimationEditor:setProperties(parameters)
	local properties = safeParameterAt(parameters, 0, nil);
	if properties == nil then
		return;
	end

	for key, defaultValue in pairs(AnimationEditorDefaults) do
		if properties:hasProperty(key) then
			if type(defaultValue) == "boolean" then
				self.settings[key] = properties:getPropertyAsBool(key);
			elseif type(defaultValue) == "number" then
				self.settings[key] = properties:getPropertyAsFloat(key);
			else
				self.settings[key] = properties:getPropertyAsString(key);
			end
		end
	end

	local clipCount = 0;
	if properties:hasProperty("clipCount") then
		clipCount = math.floor(properties:getPropertyAsFloat("clipCount"));
	end

	if clipCount > 0 then
		self.clips = {};
		for index = 1, clipCount do
			local prefix = "clip" .. tostring(index) .. ".";
			local clip =
			{
				name = "Clip " .. tostring(index),
				length = 1.0,
				speed = 1.0,
				looping = true,
			};
			if properties:hasProperty(prefix .. "name") then
				clip.name = properties:getPropertyAsString(prefix .. "name");
			end
			if properties:hasProperty(prefix .. "length") then
				clip.length = properties:getPropertyAsFloat(prefix .. "length");
			end
			if properties:hasProperty(prefix .. "speed") then
				clip.speed = properties:getPropertyAsFloat(prefix .. "speed");
			end
			if properties:hasProperty(prefix .. "looping") then
				clip.looping = properties:getPropertyAsBool(prefix .. "looping");
			end
			self.clips[#self.clips + 1] = clip;
		end
	end

	self:refreshClipDropdown();
	self:syncClipToSettings();
	self:syncToControls();
	self:applySettingsToAnimator();
end


