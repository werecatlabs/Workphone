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
game:shutdown()
assert(not app.paused and not game.started and not game.gameInitialized)
io.open, os.rename, os.remove=originalOpen,originalRename,originalRemove
for path in pairs(files) do originalRemove(path) end
print("Open city generation options, free drive, pause, races, route records and cleanup: PASS")
