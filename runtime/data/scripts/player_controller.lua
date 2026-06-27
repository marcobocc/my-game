-- player_controller.lua
-- Reads keyboard input and drives a CharacterDynamics script.
-- Attach to the same entity as CharacterDynamics.

Properties = {
    { name = "cameraEntity", type = "entity", default = 0    },
    { name = "meshEntity",   type = "entity", default = 0    },
    { name = "moveSpeed",    type = "float",  default = 5.0  },
    { name = "sprintSpeed",  type = "float",  default = 10.0 },
}

local Logger = require("logger")

local M = {}

function M:onStart()
    print("player_controller onStart fired on entity " .. tostring(self.entity))
    self.camScript    = nil
    self.dynScript    = nil
    self.spaceWasDown = false

    if self.cameraEntity and self.cameraEntity ~= 0 then
        self.camScript = World:getScript(self.cameraEntity, "third_person_controller")
    end

    Logger.info("player_controller started on entity " .. tostring(self.entity))
end

function M:getDynScript()
    if not self.dynScript then
        self.dynScript = World:getScript(self.entity, "character_dynamics")
        if not self.dynScript then
            Logger.warn("player_controller: character_dynamics not found on entity " .. tostring(self.entity))
        end
    end
    return self.dynScript
end

function M:onUpdate(dt)
    local cameraYaw = self.camScript and self.camScript:getCameraYaw() or 0.0
    local yawRad    = math.rad(cameraYaw)
    local camFwd    = Vec3( math.sin(yawRad), 0,  math.cos(yawRad))
    local camRight  = Vec3(-math.cos(yawRad), 0,  math.sin(yawRad))

    local moveDir = Vec3(0, 0, 0)
    local moving  = false
    Logger.info("player_controller: onUpdate running, W=" .. tostring(Input:isKeyDown("W")))
    if Input:isKeyDown("W") then moveDir = moveDir + camFwd;   moving = true end
    if Input:isKeyDown("S") then moveDir = moveDir - camFwd;   moving = true end
    if Input:isKeyDown("A") then moveDir = moveDir - camRight; moving = true end
    if Input:isKeyDown("D") then moveDir = moveDir + camRight; moving = true end

    local vx, vz = 0.0, 0.0
    if moving then
        local len = math.sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z)
        if len > 0.001 then
            moveDir = Vec3(moveDir.x / len, 0, moveDir.z / len)
        end
        local speed = (Input:isKeyDown("LSHIFT") or Input:isKeyDown("RSHIFT")) and self.sprintSpeed or self.moveSpeed
        vx = moveDir.x * speed
        vz = moveDir.z * speed
    end

    if moving then
        Logger.info("player_controller: moving vx=" .. vx .. " vz=" .. vz)
    end

    local dyn = self:getDynScript()
    if dyn then
        dyn:setMoveVelocity(vx, vz, moving)
    end

    local spaceDown = Input:isKeyDown("SPACE")
    if spaceDown and not self.spaceWasDown and dyn then
        dyn:jump()
    end
    self.spaceWasDown = spaceDown

    if self.camScript then
        self.camScript:recomputePosition()
    end

end

function M:onCollision(other)
    Logger.info("player_controller collision with entity " .. tostring(other))
end

return M
