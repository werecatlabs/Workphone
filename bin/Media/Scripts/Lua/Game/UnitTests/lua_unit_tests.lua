include ("Road.lua")

class 'UnitTests'

function UnitTests:__init()
	self:createActor();
	self:createComponents();
	self:createProceduralTest();
end

function UnitTests:createComponents()
	print("UnitTests:testComponents called");
	
	local applicationManager = IApplicationManager.instance();
	local sceneManager = applicationManager:getSceneManager();
	local scene = sceneManager:getCurrentScene();
	
	local actor = sceneManager:createActor();
	
	local typeInfo = UserComponent.typeInfo();
	local roadTypeId = Road.typeInfo();
	
	print("UnitTests:testComponents typeInfo "..typeInfo);
	print("UnitTests:testComponents roadTypeId "..roadTypeId);
	
	local road = actor:addComponent("Road");
	road:test();
	
	sceneManager:destroyActor(actor);
end

function UnitTests:createActor()
	print("UnitTests:createActor called");
	local applicationManager = IApplicationManager.instance();
	local sceneManager = applicationManager:getSceneManager();
	local scene = sceneManager:getCurrentScene();
	
	local actor = sceneManager:createActor();
	sceneManager:destroyActor(actor);
end

function UnitTests:createProceduralTest()
	print("UnitTests:createProceduralTest called");
	
	local applicationManager = IApplicationManager.instance();
	local sceneManager = applicationManager:getSceneManager();
	local scene = sceneManager:getCurrentScene();
	
	local actor = sceneManager:createActor();
	
	local typeInfo = UserComponent.typeInfo();
	local roadTypeId = Road.typeInfo();
	
	print("UnitTests:testComponents typeInfo "..typeInfo);
	print("UnitTests:testComponents roadTypeId "..roadTypeId);
	
	local proceduralScene = actor:addComponent("ProceduralScene");
	proceduralScene:loadData("bullsmoor_small.osm");
	
	sceneManager:destroyActor(actor);
end
