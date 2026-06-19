-- player_controller.lua
-- Handles player movement and jumping.
-- Attach to the player actor.
-- Set cameraEntity to the entity that has third_person_controller attached.

Properties = {
    { name = "cameraEntity",   type = "entity", default = 0   },
    { name = "moveSpeed",      type = "float",  default = 5.0 },
    { name = "sprintSpeed",    type = "float",  default = 10.0 },
    { name = "rotationSpeed",  type = "float",  default = 8.0 },
    { name = "jumpForce",      type = "float",  default = 6.0 },
    { name = "gravity",        type = "float",  default = 18.0 },
}

local Logger = require("logger")

local M = {}

function M:onStart()
    self.meshYaw   = 0.0
    self.velocityY = 0.0
    self.grounded  = false
    self.groundY   = 0.0
    self.camScript = nil

    local t = World:getTransform(self.entity)
    if t then
        self.groundY = t.position.y
        local fwd = t:getForward()
        self.meshYaw = math.deg(math.atan2(fwd.x, fwd.z))
    end

    if self.cameraEntity and self.cameraEntity ~= 0 then
        self.camScript = World:getScript(self.cameraEntity, "third_person_controller")
    end

    Logger.info("player_controller started on entity " .. tostring(self.entity))
end

function M:onUpdate(dt)
    local t = World:getTransform(self.entity)
    if not t then return end

    local cameraYaw = self.camScript and self.camScript:getCameraYaw() or 0.0

    local yawRad   = math.rad(cameraYaw)
    local camFwd   = Vec3( math.sin(yawRad), 0,  math.cos(yawRad))
    local camRight = Vec3(-math.cos(yawRad), 0,  math.sin(yawRad))

    -- Horizontal movement
    local moveDir = Vec3(0, 0, 0)
    local moving  = false
    if Input:isKeyDown("W") then moveDir = moveDir + camFwd;   moving = true end
    if Input:isKeyDown("S") then moveDir = moveDir - camFwd;   moving = true end
    if Input:isKeyDown("A") then moveDir = moveDir - camRight; moving = true end
    if Input:isKeyDown("D") then moveDir = moveDir + camRight; moving = true end

    if moving then
        local len = math.sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z)
        if len > 0.001 then
            moveDir = Vec3(moveDir.x / len, 0, moveDir.z / len)
        end

        local speed = (Input:isKeyDown("LSHIFT") or Input:isKeyDown("RSHIFT")) and self.sprintSpeed or self.moveSpeed
        t.position = t.position + moveDir * (speed * dt)

        local targetYaw = math.deg(math.atan2(moveDir.x, moveDir.z))
        local diff = ((targetYaw - self.meshYaw) + 540) % 360 - 180
        self.meshYaw = self.meshYaw + diff * math.min(1.0, self.rotationSpeed * dt)
        t.rotation = quatAxisAngle(Vec3(0, 1, 0), self.meshYaw)
    end

    -- Vertical movement (gravity + jump)
    self.grounded = t.position.y <= self.groundY + 0.001

    if self.grounded then
        self.velocityY = 0.0
        t.position = Vec3(t.position.x, self.groundY, t.position.z)

        if Input:isKeyDown("SPACE") then
            self.velocityY = self.jumpForce
            self.grounded  = false
        end
    else
        self.velocityY = self.velocityY - self.gravity * dt
    end

    t.position = Vec3(t.position.x, t.position.y + self.velocityY * dt, t.position.z)

    World:setTransform(self.entity, t)

    -- Reposition camera immediately after player moves to eliminate one-frame lag.
    if self.camScript then
        self.camScript:recomputePosition()
    end
end

return M
