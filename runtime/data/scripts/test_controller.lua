-- test_controller.lua
-- Tests all four scripting requirements:
--   1. Script calls engine API  (World:getTransform / setTransform, Input:isKeyDown)
--   2. Engine calls script hooks (onStart, onUpdate)
--   3. Scripts call each other  (require "logger")
--   4. Custom structs passed    (Transform usertype with Vec3 members)

-- Properties declared here are parsed by the editor and shown in the inspector.
-- Values set in the inspector are injected into self before onStart runs.
Properties = {
    { name = "moveSpeed",    type = "float",  default = 7.0  },
    { name = "mouseSens",    type = "float",  default = 0.1  },
    { name = "jumpForce",    type = "float",  default = 11.0 },
    { name = "gravity",      type = "float",  default = 9.81 },
    { name = "groundHeight", type = "float",  default = 1.7  },
}

local Logger = require("logger")  -- requirement 3

local M = {}

function M:onStart()
    -- self.moveSpeed, self.mouseSens etc. are pre-populated from the inspector
    self.yaw      = 0.0
    self.pitch    = 0.0
    self.jumpVel  = 0.0
    self.grounded = true

    -- requirement 1: read Transform from world
    local t = World:getTransform(self.entity)
    if t then
        -- derive initial yaw from forward vector
        local fwd = t:getForward()
        self.yaw   = math.deg(math.atan2(fwd.x, fwd.z))
        self.pitch = math.deg(math.asin(math.max(-1, math.min(1, fwd.y))))
    end

    Input:lockMouse()
    Logger.info("test_controller started on entity " .. tostring(self.entity))
end

function M:onUpdate(dt)
    -- requirement 1 + 4: get Transform (custom struct) from world
    local t = World:getTransform(self.entity)
    if not t then return end

    -- Mouse look
    local dx, dy = Input:getMouseDelta()
    self.yaw   = self.yaw   - dx * self.mouseSens
    self.pitch = math.max(-89, math.min(89, self.pitch - dy * self.mouseSens))

    local yawRad   = math.rad(self.yaw)
    local pitchRad = math.rad(self.pitch)
    local yawQ     = quatAxisAngle(Vec3(0, 1, 0), self.yaw)
    local pitchQ   = quatAxisAngle(Vec3(1, 0, 0), self.pitch)
    t.rotation     = yawQ * pitchQ

    -- WASD movement — requirement 1: Input:isKeyDown with string names
    local fwd   = t:getForward()
    local right = t:getRight()
    -- flatten to horizontal plane
    fwd.y = 0;   fwd   = Vec3(fwd.x,   0, fwd.z)
    right.y = 0; right = Vec3(right.x, 0, right.z)

    if Input:isKeyDown("W") then
        t.position = t.position + fwd * (self.moveSpeed * dt)
    end
    if Input:isKeyDown("S") then
        t.position = t.position - fwd * (self.moveSpeed * dt)
    end
    if Input:isKeyDown("A") then
        t.position = t.position + right * (self.moveSpeed * dt)
    end
    if Input:isKeyDown("D") then
        t.position = t.position - right * (self.moveSpeed * dt)
    end

    -- Jump
    if self.grounded and Input:isKeyPressed("SPACE") then
        self.jumpVel  = self.jumpForce
        self.grounded = false
        Logger.info("jumped")
    end
    self.jumpVel  = self.jumpVel - self.gravity * dt
    t.position.y  = t.position.y + self.jumpVel * dt
    if t.position.y <= self.groundHeight then
        t.position.y  = self.groundHeight
        self.jumpVel  = 0.0
        self.grounded = true
    end

    -- Debug key
    if Input:isKeyPressed("F") then
        Logger.info("pos=(" ..
            string.format("%.2f", t.position.x) .. ", " ..
            string.format("%.2f", t.position.y) .. ", " ..
            string.format("%.2f", t.position.z) .. ")")
    end

    -- requirement 1: write Transform back
    World:setTransform(self.entity, t)
end

return M
