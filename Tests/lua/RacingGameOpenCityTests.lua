-- Run from the repository root with luavm.exe.
local fixture = dofile("Tests/lua/RacingGameFullTests.lua")
local app, race, owner, keys = fixture.app, fixture.race, fixture.owner, fixture.keys
include("RacingGameOpenCity.lua")
assert(RacingGameFull ~= RacingGameOpenCity, "City game must have its own class")
local properties = {values={["Open City"]=false}, buttons={}}
function properties:hasProperty(name) return self.values[name] ~= nil end
function properties:setPropertyAsBool(name, value) self.values[name] = value end
function properties:setPropertyAsInt(name, value) self.values[name] = value end
function race:getProperties() return properties end
function race:setProperties(p) self.cityOptions=p.values end
local store = os.tmpname()
os.remove(store)
local originalOpen, originalRename, originalRemove = io.open, os.rename, os.remove
local files = {}
local function redirect(path)
    local suffix = path:match("RacingGameOpenCity(.*)$")
    if suffix then local target=store..suffix; files[target]=true; return target end
    return path
end
io.open = function(path, mode) return originalOpen(redirect(path),mode) end
os.rename = function(a,b) return originalRename(redirect(a),redirect(b)) end
os.remove = function(path) return originalRemove(redirect(path)) end
local game = RacingGameOpenCity({getActor=function() return owner end})
app.playing=false
local inspector = {values={Seed="23", ["Appearance Quality"]=0, Laps=1, ["City Blocks"]=8, ["City Route"]=1}}
function inspector:hasProperty(name) return self.values[name] ~= nil end
function inspector:getPropertyAsString(name) return self.values[name] end
function inspector:getPropertyAsInt(name, default) return self.values[name] or default end
function inspector:isButtonPressed(name) return name=="Generate" end
game:setProperties(fixture.array({inspector}))
assert(game.editPreview and game.generatedRoot.name == "RacingGameOpenCity.Generated")
assert(race.cityOptions["Open City"] and race.cityOptions["City Blocks"]==8 and race.cityOptions["City Route"]==1)
local root = game.generatedRoot
app.playing=true; app.now=20; game:update()
assert(game.gameState=="main" and game.menu.visible)
game.mode="freeDrive"; game:startRace()
assert(game.generatedRoot==root, "Free drive reuses the city preview")
app.now=21; game:update(); app.now=24; game:update()
assert(game.gameState=="racing" and race.playerControls)
for _, index in ipairs({10,20,25,35,45,50,60,70,75,85,95,99,0}) do
    race.index=index; game.vehicleActor.position=Vector3F(0,.42,-index)
    app.now=app.now+1; game:update()
end
assert(game.manager.session.completedLaps==0 and game.records:getBest(23)==0, "Exploration must not award race records")
assert(game.hud.dashboard.setTextValue:find("OPEN CITY"))
keys[KeyCode.Escape]=true; app.now=app.now+1; game:update()
assert(game.gameState=="pause" and app.paused)
keys[KeyCode.Escape]=false; game:update(); game:resumeRace()
assert(game.gameState=="racing" and not app.paused)
game.mode="race"; game:startRace()
assert(game.generatedRoot==root)
app.now=app.now+1; game:update(); app.now=app.now+3; game:update()
for _, index in ipairs({10,20,25,35,45,50,60,70,75,85,95,99,0}) do
    race.index=index; game.vehicleActor.position=Vector3F(0,.42,-index)
    app.now=app.now+1; game:update()
end
assert(game.gameState=="results" and game.records:getBest(23)>0)
local record=game.records:getBest(23)
game.cityRoute=2; game:startRace()
assert(game.generatedRoot~=root and game.generatedRoute==2 and game.records:getBest(23)==0)
game.cityRoute=1; game:startRace()
assert(game.records:getBest(23)==record, "Routes must keep independent persistent records")
root=game.generatedRoot; game.cityBlocks=10; game:startRace()
assert(game.generatedRoot~=root and race.cityOptions["City Blocks"]==10 and game.records:getBest(23)==0)
game.mode="timeTrial"; game:startRace()
assert(game.manager.session.mode=="timeTrial")
game:pauseRace(); game.menu.view.callbacks.Workshop()
-- Native callbacks are deferred; use the normal update to finish the trial.
game:update()
assert(game.gameState=="results")
properties.values["City Natural Roads"]=false
root=game.generatedRoot
game.cityNaturalRoads=true;game:startRace()
assert(game.generatedRoot~=root and race.cityOptions["City Natural Roads"] and game.records:getBest(23)==0,
    "Switching to natural roads must rebuild and use separate records")
game.cityNaturalRoads=false;game:startRace()
assert(not race.cityOptions["City Natural Roads"],"Switching back to grid must clear natural layout state")

-- Persist the canonical source through both a retained native component and a
-- recreated Lua/component instance. Source payloads live independently of it.
properties.values["City Terrain Roads"], properties.values["City Road Authoring UUID"] = false, ""
function properties:getPropertyAsBool(name, default) local value=self.values[name];if value==nil then return default end;return value end
function properties:getPropertyAsString(name) return self.values[name] end
function properties:setPropertyAsString(name, value) self.values[name]=value end
local originalRegenerate = race.regenerate
local recordsLoad,recordsSave=RacingRecords.load,RacingRecords.save
local terrainLapLoads,terrainLapSaves=0,0
function RacingRecords:load()
    if self.path==nil then terrainLapLoads=terrainLapLoads+1;error("Session-only terrain lap records must not load a file") end
    return recordsLoad(self)
end
function RacingRecords:save()
    if self.path==nil then terrainLapSaves=terrainLapSaves+1;error("Session-only terrain lap records must not save a file") end
    return recordsSave(self)
end
local initialId = "46bfebfc-f4a8-44bd-8811-ecc343533001"
local editedId = "46bfebfc-f4a8-44bd-8811-ecc343533002"
local selectedId = "46bfebfc-f4a8-44bd-8811-ecc343533003"
local sources = {
    [initialId]={width=12,knot="81f2148b-c7ab-48a2-a045-f01c56ed1001"},
    [editedId]={width=11.75,knot="81f2148b-c7ab-48a2-a045-f01c56ed1002"},
    [selectedId]={width=10.5,knot="81f2148b-c7ab-48a2-a045-f01c56ed1003"}
}
local generations = 0
function race:regenerate()
    assert(self.cityOptions["City Terrain Roads"], "Terrain mode must reach the native component before generation")
    local id=self.cityOptions["City Road Authoring UUID"]
    assert(type(id)=="string", "Source binding must be forwarded before generation")
    if id=="" then id=initialId;properties.values["City Road Authoring UUID"]=id end
    self.liveSource=assert(sources[id], "Regeneration must resolve the retained canonical source")
    generations=generations+1
    return true
end
local function inspectorValues(values)
    local p={values=values or {},generate=false}
    function p:hasProperty(name) return self.values[name]~=nil end
    function p:getPropertyAsString(name) return self.values[name] end
    function p:getPropertyAsInt(name,default) local v=self.values[name];return v==nil and default or v end
    function p:getPropertyAsBool(name,default) local v=self.values[name];if v==nil then return default end;return v end
    function p:setPropertyAsString(name,value) self.values[name]=value end
    function p:setPropertyAsInt(name,value) self.values[name]=value end
    function p:setPropertyAsBool(name,value) self.values[name]=value end
    function p:setButtonPressed(name,value) if name=="Generate" then self.generate=value end end
    function p:isButtonPressed(name) return name=="Generate" and self.generate end
    return p
end
local terrainInspector=inspectorValues({["City Terrain Roads"]=true,["City Road Authoring UUID"]=""})
terrainInspector.generate=true;game:setProperties(fixture.array({terrainInspector}))
assert(game.cityTerrainRoads and game.generatedTerrainRoads and game.cityRoadAuthoringUUID==initialId)
assert(race.cityOptions["City Terrain Roads"] and race.liveSource==sources[initialId])
assert(game.records.path==nil and game.routeRecordsPath==nil and game:setupSummary():find("play session only",1,true))
assert(game.records:record(game.generatedSeed,91));game:savePreferences()
root=game.generatedRoot;local nativeScene,vehicle,camera=game.raceScene,game.vehicleActor,game.cameraActor
-- A native Inspector selection is captured unless the script has a newer selection.
properties.values["City Road Authoring UUID"]=editedId
game.quality=(game.quality+1)%4;game.seed=game.seed+1;game:startRace()
assert(game.generatedRoot==root and game.raceScene==nativeScene and game.vehicleActor==vehicle and game.cameraActor==camera,
    "Terrain city quality/seed changes must reuse the active native scene and camera")
assert(game.cityRoadAuthoringUUID==editedId and game.generatedRoadAuthoringUUID==editedId and race.liveSource.width==11.75)
assert(race.liveSource.knot==sources[editedId].knot and game.manager.totalRacers==1)
assert(game.records.path==nil and game.records:getBest(game.generatedSeed)==0,"Changing a canonical source must discard incompatible session lap records")
local before=generations
terrainInspector.values["City Road Authoring UUID"]=selectedId;terrainInspector.generate=false
game:setProperties(fixture.array({terrainInspector}));game:startRace()
assert(generations==before+1 and game.generatedRoot==root and game.cityRoadAuthoringUUID==selectedId and race.liveSource.width==10.5,
    "A newer script Inspector UUID must survive regeneration and invalidate the generation cache")
game.mode="race";game.totalLaps=1;game:startRace()
app.now=app.now+1;game:update();app.now=app.now+3;game:update()
for _,index in ipairs({10,20,25,35,45,50,60,70,75,85,95,99,0}) do
    race.index=index;game.vehicleActor.position=Vector3F(0,.42,-index)
    app.now=app.now+1;game:update()
end
local sessionBest=game.records:getBest(game.raceSeed)
assert(game.gameState=="results" and sessionBest>0 and game.menu.view.statusText.setTextValue:find("play session only",1,true),
    "Terrain races must finish and expose their in-memory best time and record persistence policy")
game:startRace()
assert(game.records:getBest(game.raceSeed)==sessionBest,"Retrying the same route may retain the current play-session best")
local routeLength=race.getCircuitLength
function race:getCircuitLength() return routeLength(self)+1 end
app.now=app.now+1;game:update()
assert(game.gameState=="main" and game.records:getBest(game.raceSeed)==0 and game.records.path==nil,
    "Native route count/length invalidation must clear terrain session records")
race.getCircuitLength=routeLength
local saved=inspectorValues();game:getProperties(fixture.array({saved}))
assert(saved.values["City Terrain Roads"] and saved.values["City Road Authoring UUID"]==selectedId,
    "Serialized script properties must retain terrain mode and canonical source UUID")
game:shutdown()
properties.values["City Terrain Roads"],properties.values["City Road Authoring UUID"]=false,""
local restored=RacingGameOpenCity({getActor=function() return owner end});saved.generate=true
restored:setProperties(fixture.array({saved}))
assert(restored.started and restored.raceScene==race and restored.cityTerrainRoads and restored.generatedTerrainRoads)
assert(restored.cityRoadAuthoringUUID==selectedId and race.liveSource==sources[selectedId] and race.liveSource.knot==sources[selectedId].knot,
    "A recreated script/native component must bind the edited source instead of creating a new source")
local restoredRoot=restored.generatedRoot
local modeOff=inspectorValues({["City Terrain Roads"]=false});modeOff.generate=true
local disabled,disableError=pcall(restored.setProperties,restored,fixture.array({modeOff}))
assert(not disabled and tostring(disableError):find("separate scene",1,true) and restored.generatedRoot==restoredRoot and restored.cityTerrainRoads,
    "Switching an active terrain city to flat must preserve its source and report native lifecycle unavailability")
restored:shutdown();race.regenerate=originalRegenerate

local failedId="46bfebfc-f4a8-44bd-8811-ecc343533004"
local partialSource={width=11.25,knot="81f2148b-c7ab-48a2-a045-f01c56ed1004"}
local assignedSources,failedOnce=0,false
properties.values["City Terrain Roads"],properties.values["City Road Authoring UUID"]=false,""
function race:regenerate()
    assert(self.cityOptions["City Terrain Roads"], "Retry must retain terrain mode")
    local id=self.cityOptions["City Road Authoring UUID"]
    if id=="" then
        assignedSources=assignedSources+1;id=failedId;properties.values["City Road Authoring UUID"]=id
    end
    assert(id==failedId, "Retry must resolve the first failed generation's source")
    self.liveSource=partialSource
    if not failedOnce then failedOnce=true;return false end
    return true
end
local retry=RacingGameOpenCity({getActor=function() return owner end})
retry.cityTerrainRoads=true
local firstSucceeded=pcall(retry.generate,retry)
assert(not firstSucceeded and retry.startFailed and not retry.started and retry.cityRoadAuthoringUUID==failedId,
    "A failed first generation must capture its newly assigned canonical binding before shutdown")
partialSource.width=10.75
properties.values["City Terrain Roads"],properties.values["City Road Authoring UUID"]=false,""
retry:generate()
assert(retry.started and retry.cityRoadAuthoringUUID==failedId and assignedSources==1 and race.liveSource==partialSource and race.liveSource.width==10.75,
    "Retry must preserve edits on the retained failed-generation source instead of creating a duplicate anchor")
retry:shutdown()
-- An explicit script UUID selection has priority over stale native properties,
-- including when that generation fails before the success-only binding cache.
local selectedRetry=RacingGameOpenCity({getActor=function() return owner end})
selectedRetry.cityTerrainRoads=true;selectedRetry.cityRoadAuthoringUUID=selectedId
function race:regenerate()
    assert(self.cityOptions["City Road Authoring UUID"]==selectedId)
    properties.values["City Road Authoring UUID"]=failedId
    return false
end
local selectedSucceeded=pcall(selectedRetry.generate,selectedRetry)
assert(not selectedSucceeded and selectedRetry.cityRoadAuthoringUUID==selectedId,
    "Failure capture must not overwrite an explicit Inspector source selection")
selectedRetry:shutdown();race.regenerate=originalRegenerate
assert(terrainLapLoads==0 and terrainLapSaves==0,"Editable terrain lap records must remain entirely in memory")
RacingRecords.load,RacingRecords.save=recordsLoad,recordsSave
game:shutdown()
assert(not app.paused and not game.started and not game.gameInitialized)
-- Unsupported native builds must fail cleanly without leaving a generated subtree.
local children = #owner.children
properties.values["Open City"] = nil
app.playing=false
local unsupported = RacingGameOpenCity({getActor=function() return owner end})
local ok, failure = pcall(unsupported.generate, unsupported)
assert(not ok and tostring(failure):find("Open City support"))
assert(not unsupported.started and #owner.children==children)
properties.values["Open City"] = true
io.open, os.rename, os.remove=originalOpen,originalRename,originalRemove
for path in pairs(files) do originalRemove(path) end
print("Open city generation options, free drive, pause, races, route records and cleanup: PASS")
