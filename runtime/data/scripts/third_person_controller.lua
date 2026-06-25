-- third_person_controller.lua
-- Orbits a camera around a target entity based on mouse input.
-- Attach to the camera entity. Set targetEntity to the player actor.

Properties = {
    { name = "targetEntity",   type = "entity", default = 0   },
    { name = "mouseSens",      type = "float",  default = 0.1 },
    { name = "cameraDistance", type = "float",  default = 5.0 },
    { name = "minDistance",    type = "float",  default = 1.0 },
    { name = "maxDistance",    type = "float",  default = 15.0 },
    { name = "pivotHeight",    type = "float",  default = 1.0 },
}

local Logger = require("logger")

local M = {}

function M:onStart()
    self.yaw            = 0.0
    self.pitch          = 15.0
    self.currentDistance = self.cameraDistance
    self.targetDistance  = self.cameraDistance

    local t = self.targetEntity ~= 0 and World:getTransform(self.targetEntity)
    if t then
        local fwd = t:getForward()
        self.yaw = math.deg(math.atan2(fwd.x, fwd.z))
    end

    Input:lockMouse()
    Logger.info("third_person_controller started")
end

function M:recomputePosition()
    if not self.targetEntity or self.targetEntity == 0 then return end
    local targetT = World:getTransform(self.targetEntity)
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

    local camT = World:getTransform(self.entity)
    if not camT then return end

    camT.position = Vec3(pivot.x + armX, pivot.y + armY, pivot.z + armZ)

    local camYawQ   = quatAxisAngle(Vec3(0, 1, 0), math.deg(yawRad))
    local rightAxis = Vec3(-math.cos(yawRad), 0, math.sin(yawRad))
    local camPitchQ = quatAxisAngle(rightAxis, -self.pitch)
    camT.rotation   = camPitchQ * camYawQ

    World:setTransform(self.entity, camT)
end

function M:onUpdate(dt)
    local dx, dy = Input:getMouseDelta()
    self.yaw   = self.yaw   - dx * self.mouseSens
    self.pitch = math.max(-20, math.min(60, self.pitch + dy * self.mouseSens))

    local scroll = Input:getScrollDelta()
    local minDist = self.minDistance or 1.0
    local maxDist = self.maxDistance or 15.0
    self.targetDistance  = math.max(minDist, math.min(maxDist, self.targetDistance - scroll))
    local alpha = 1.0 - math.exp(-10.0 * dt)
    self.currentDistance = self.currentDistance + (self.targetDistance - self.currentDistance) * alpha

    self:recomputePosition()
end

function M:getCameraYaw()
    return self.yaw
end

return M
