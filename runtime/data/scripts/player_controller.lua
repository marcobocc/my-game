-- player_controller.lua
-- Reads keyboard/mouse input, orbits the camera around a target, and drives CharacterDynamics.
-- Attach to the camera entity. Set targetCharacter to the player entity.

Properties = {
    { name = "targetCharacter",   type = "entity", default = 0   },
    { name = "mouseSens",      type = "float",  default = 0.1 },
    { name = "cameraDistance", type = "float",  default = 5.0 },
    { name = "minDistance",    type = "float",  default = 1.0 },
    { name = "maxDistance",    type = "float",  default = 15.0 },
    { name = "pivotHeight",    type = "float",  default = 1.0 },
}

local Logger = require("logger")

local M = {}

function M:onStart()
    self.dynScript       = nil
    self.spaceWasDown    = false
    self.lmbWasDown      = false
    self.statusScript    = nil

    self.yaw             = 0.0
    self.pitch           = 15.0
    self.currentDistance = self.cameraDistance
    self.targetDistance  = self.cameraDistance

    local t = self.targetCharacter:isValid() and self.targetCharacter:getTransform()
    if t then
        local fwd = t:getForward()
        self.yaw = math.deg(math.atan2(fwd.x, fwd.z))
    end

    Input:lockMouse()
    Logger.info("player_controller started on entity " .. tostring(self.entity))
end

function M:getDynScript()
    if not self.dynScript then
        self.dynScript = self.targetCharacter:getScript("character_dynamics")
        if not self.dynScript then
            Logger.warn("player_controller: character_dynamics not found on entity " .. tostring(self.targetCharacter))
        end
    end
    return self.dynScript
end

function M:getStatusScript()
    if not self.statusScript then
        self.statusScript = self.targetCharacter:getScript("character_status")
        if not self.statusScript then
            Logger.warn("player_controller: character_status not found on entity " .. tostring(self.targetCharacter))
        end
    end
    return self.statusScript
end

function M:recomputeCameraPosition()
    if not self.targetCharacter or not self.targetCharacter:isValid() then return end
    local targetT = self.targetCharacter:getTransform()
    if not targetT then return end

    local yawRad   = math.rad(self.yaw)
    local pitchRad = math.rad(self.pitch)

    local dist  = self.currentDistance
    local hDist = math.cos(pitchRad) * dist
    local armX  = -math.sin(yawRad) * hDist
    local armY  =  math.sin(pitchRad) * dist
    local armZ  = -math.cos(yawRad) * hDist

    local pivot = Vec3(
        targetT.position.x,
        targetT.position.y + self.pivotHeight,
        targetT.position.z
    )

    local camT = self.entity:getTransform()
    if not camT then return end

    camT.position = Vec3(pivot.x + armX, pivot.y + armY, pivot.z + armZ)

    local camYawQ   = quatAxisAngle(Vec3(0, 1, 0), math.deg(yawRad))
    local rightAxis = Vec3(-math.cos(yawRad), 0, math.sin(yawRad))
    local camPitchQ = quatAxisAngle(rightAxis, -self.pitch)
    camT.rotation   = camPitchQ * camYawQ

    self.entity:setTransform(camT)
end

function M:onUpdate(dt)
    -- Camera orbit
    local dx, dy = Input:getMouseDelta()
    self.yaw   = self.yaw - dx * self.mouseSens
    self.pitch = math.max(-20, math.min(60, self.pitch + dy * self.mouseSens))

    local scroll = Input:getScrollDelta()
    self.targetDistance  = math.max(self.minDistance, math.min(self.maxDistance, self.targetDistance - scroll))
    local alpha = 1.0 - math.exp(-10.0 * dt)
    self.currentDistance = self.currentDistance + (self.targetDistance - self.currentDistance) * alpha

    -- Movement input relative to camera yaw
    local yawRad   = math.rad(self.yaw)
    local camFwd   = Vec3( math.sin(yawRad), 0,  math.cos(yawRad))
    local camRight = Vec3(-math.cos(yawRad), 0,  math.sin(yawRad))

    local moveDir = Vec3(0, 0, 0)
    local moving  = false
    if Input:isKeyDown("W") then moveDir = moveDir + camFwd;   moving = true end
    if Input:isKeyDown("S") then moveDir = moveDir - camFwd;   moving = true end
    if Input:isKeyDown("A") then moveDir = moveDir - camRight; moving = true end
    if Input:isKeyDown("D") then moveDir = moveDir + camRight; moving = true end

    local dyn = self:getDynScript()

    local vx, vz = 0.0, 0.0
    if moving then
        local len = math.sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z)
        if len > 0.001 then
            moveDir = Vec3(moveDir.x / len, 0, moveDir.z / len)
        end
        local speed = (Input:isKeyDown("LSHIFT") or Input:isKeyDown("RSHIFT"))
            and (dyn and dyn.sprintSpeed or 10.0)
             or (dyn and dyn.moveSpeed   or  5.0)
        vx = moveDir.x * speed
        vz = moveDir.z * speed
    end

    if dyn then
        dyn:setMoveVelocity(vx, vz, moving)
    end

    -- Jump
    local spaceDown = Input:isKeyDown("SPACE")
    if spaceDown and not self.spaceWasDown and dyn then
        dyn:jump()
    end
    self.spaceWasDown = spaceDown

    -- Attack
    local lmbDown = Input:isMouseButtonDown("LEFT")
    if lmbDown and not self.lmbWasDown then
        local status = self:getStatusScript()
        if status then status:attack() end
    end
    self.lmbWasDown = lmbDown

    self:recomputeCameraPosition()
end

function M:onCollision(other)
    Logger.info("player_controller collision with entity " .. tostring(other))
end

return M
