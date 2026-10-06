if not RacingSupport then include("RacingSupport.lua") end
-- Versioned scalar dictionaries; never execute player-owned data as Lua.
class 'DataLoader' (RacingComponent)
local function encode(value) return (value:gsub("([^%w _.-])", function(c) return string.format("%%%02X",c:byte()) end)) end
local function decode(value)
    assert(not value:gsub("%%%x%x", ""):find("%%"), "Invalid escaped value")
    return (value:gsub("%%(%x%x)", function(hex) return string.char(tonumber(hex,16)) end))
end
function DataLoader:__init(component) BaseComponent.__init(self,component); self.maxBytes, self.maxEntries = 1048576, 4096 end
function DataLoader:serialize(data)
    local lines, keys = {"WORKPHONE_RACING_DATA=1"}, {}
    for key in pairs(data) do assert(type(key)=="string" and key:match("^[%w_.:-]+$") and #key<=128,"Invalid data key"); keys[#keys+1]=key end
    assert(#keys<=self.maxEntries,"Too many data entries"); table.sort(keys)
    for _,key in ipairs(keys) do
        local value, kind = data[key], type(data[key])
        if kind=="number" then assert(RacingSupport.finite(value),"Invalid data number"); value="n:"..string.format("%.17g",value)
        elseif kind=="boolean" then value=value and "b:1" or "b:0"
        else assert(kind=="string" and #value<=65536,"Invalid data value"); value="s:"..encode(value) end
        lines[#lines+1]=key.."="..value
    end
    local result=table.concat(lines,"\n").."\n"; assert(#result<=self.maxBytes,"Data is too large"); return result
end
function DataLoader:deserialize(text)
    assert(type(text)=="string" and #text<=self.maxBytes,"Data is too large")
    local data, count, header = {}, 0, false
    for rawLine in text:gmatch("[^\n]+") do
        local line=rawLine:gsub("\r$","")
        if not header then assert(line=="WORKPHONE_RACING_DATA=1","Unsupported data version"); header=true
        else
            local key,kind,value=line:match("^([%w_.:-]+)=([nbs]):(.*)$")
            assert(key and #key<=128 and data[key]==nil,"Malformed or duplicate data key")
            if kind=="n" then value=tonumber(value); assert(RacingSupport.finite(value),"Invalid data number")
            elseif kind=="b" then assert(value=="0" or value=="1","Invalid boolean"); value=value=="1"
            else value=decode(value); assert(#value<=65536,"Value is too large") end
            data[key]=value; count=count+1; assert(count<=self.maxEntries,"Too many entries")
        end
    end
    assert(header,"Empty data"); return data
end
function DataLoader:load(path)
    local file, failure=io.open(path,"rb"); if not file then return nil,failure end
    local text=file:read(self.maxBytes+1); file:close()
    local ok, result=pcall(self.deserialize,self,text or "")
    return ok and result or nil, not ok and tostring(result) or nil
end
function DataLoader:save(path,data)
    local ok,text=pcall(self.serialize,self,data); if not ok then return false,tostring(text) end
    return DataLoader.writeAtomic(path,text)
end
function DataLoader.writeAtomic(path,text)
    local temporary, backup = path..".tmp", path..".bak"
    local stale=io.open(backup,"rb"); if stale then stale:close(); return false,"Recover existing backup before saving: "..backup end
    local file, failure=io.open(temporary,"wb"); if not file then return false,failure end
    local written,writeError=file:write(text); local closed,closeError=file:close()
    if not written or not closed then os.remove(temporary); return false,writeError or closeError end
    local existing=io.open(path,"rb"); local moved=false
    if existing then
        existing:close(); local renamed,renameError=os.rename(path,backup)
        if not renamed then os.remove(temporary); return false,renameError end
        moved=true
    end
    local renamed,renameError=os.rename(temporary,path)
    if not renamed then if moved then os.rename(backup,path) end; os.remove(temporary); return false,renameError end
    if moved then os.remove(backup) end
    return true
end
