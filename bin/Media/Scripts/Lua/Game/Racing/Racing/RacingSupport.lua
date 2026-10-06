-- Shared validation/math for racing components. No scene or singleton ownership.
RacingSupport = RacingSupport or {}
local R = RacingSupport
function R.finite(value) return type(value) == "number" and value == value and math.abs(value) < math.huge end
function R.number(value, low, high, name)
    assert(R.finite(value) and value >= low and value <= high, (name or "value") .. " is out of range")
    return value
end
function R.clamp(value, low, high) return math.max(low, math.min(high, value)) end
function R.integer(value, low, high, name)
    R.number(value, low, high, name); assert(value == math.floor(value), (name or "value") .. " must be an integer")
    return value
end
function R.vector(v)
    assert(v and R.finite(v.x) and R.finite(v.y) and R.finite(v.z), "Invalid position")
    return Vector3F(v.x, v.y, v.z)
end
function R.length(v) return math.sqrt(v.x*v.x + v.y*v.y + v.z*v.z) end
function R.unit(v)
    local n = R.length(v)
    return n > 0.00001 and Vector3F(v.x/n, v.y/n, v.z/n) or Vector3F(0, 0, -1)
end
function R.now()
    local app = IApplicationManager.instance()
    return app and app:getTimer():now() or 0
end
function R.delta(owner, dt)
    local now = R.now()
    dt = dt or (owner.lastUpdate and math.max(0, now - owner.lastUpdate) or 0)
    owner.lastUpdate = now
    return R.number(dt, 0, 3600, "delta time")
end
function R.quaternion(q)
    local w, x, y, z = q:W(), q:X(), q:Y(), q:Z()
    assert(R.finite(w) and R.finite(x) and R.finite(y) and R.finite(z), "Invalid rotation")
    local n = math.sqrt(w*w+x*x+y*y+z*z)
    assert(n > 0.00001, "Zero rotation")
    return Quaternion(w/n, x/n, y/n, z/n)
end
function R.rotation(a, b, alpha)
    local aw, ax, ay, az = a:W(), a:X(), a:Y(), a:Z()
    local bw, bx, by, bz = b:W(), b:X(), b:Y(), b:Z()
    if aw*bw+ax*bx+ay*by+az*bz < 0 then bw,bx,by,bz = -bw,-bx,-by,-bz end
    return R.quaternion(Quaternion(aw+(bw-aw)*alpha, ax+(bx-ax)*alpha, ay+(by-ay)*alpha, az+(bz-az)*alpha))
end
function R.label(label, text)
    if label and label:getText() ~= text then label:setText(text) end
end
return R
