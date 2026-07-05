-- character_dynamics.lua
-- Handles physics, mesh rotation, and animation for a character.
-- Attach to the same entity as the collider. Set meshEntity to the root transform/animator entity.

Properties = {
    { name = "meshEntity",    type = "entity", default = 0    },
    { name = "jumpForce",     type = "float",  default = 6.0  },
    { name = "gravity",       type = "float",  default = 18.0 },
    { name = "rotationSpeed", type = "float",  default = 8.0  },
}

local M = {}

function M:onStart()
    self.velocityY  = 0.0
    self.grounded   = false
    self.velocityXZ = Vec3(0, 0, 0)
    self.moving     = false
    self.meshYaw    = 0.0
    self.animState  = ""

    local rootEntity = (self.meshEntity and self.meshEntity ~= 0) and self.meshEntity or self.entity
    local t = World:getTransform(rootEntity)
    if t then
        local fwd = t:getForward()
        self.meshYaw = math.deg(math.atan2(fwd.x, fwd.z))
    end
end

-- Called by PlayerController each frame with the desired horizontal velocity and whether the character is moving.
function M:setMoveVelocity(vx, vz, moving)
    self.velocityXZ = Vec3(vx, 0, vz)
    self.moving     = moving
end

-- Returns true if a jump was successfully initiated.
function M:jump()
    if not self.grounded then return false end
    self.velocityY = self.jumpForce
    self.grounded  = false
    return true
end

function M:isGrounded()
    return self.grounded
end

function M:getVelocityY()
    return self.velocityY
end

function M:setAnimState(state)
    if self.animState == state then return end
    local prev = self.animState
    self.animState = state
    local target = (self.meshEntity and self.meshEntity ~= 0) and self.meshEntity or self.entity
    local anim = World:getAnimator(target)
    if anim then
        local blend = (prev == "Jump") and 0.5 or 0.2
        anim:play(state, blend)
    end
end

function M:onUpdate(dt)
    local rootEntity = (self.meshEntity and self.meshEntity ~= 0) and self.meshEntity or self.entity
    local t = World:getTransform(rootEntity)
    if not t then return end

    -- Mesh rotation toward movement direction
    if self.moving then
        local vx, vz = self.velocityXZ.x, self.velocityXZ.z
        local len = math.sqrt(vx * vx + vz * vz)
        if len > 0.001 then
            local targetYaw = math.deg(math.atan2(vx / len, vz / len))
            local diff = ((targetYaw - self.meshYaw) + 540) % 360 - 180
            self.meshYaw = self.meshYaw + diff * math.min(1.0, self.rotationSpeed * dt)
            t.rotation = quatAxisAngle(Vec3(0, 1, 0), self.meshYaw)
        end
    end

    -- Horizontal movement
    t.position = t.position + self.velocityXZ * dt

    -- Vertical velocity
    if not self.grounded then
        self.velocityY = self.velocityY - self.gravity * dt
    else
        self.velocityY = -self.gravity * dt
    end
    t.position = Vec3(t.position.x, t.position.y + self.velocityY * dt, t.position.z)

    World:setTransform(rootEntity, t)

    -- Collision resolution
    local wasGrounded = self.grounded
    self.grounded = false
    local push = World:resolveCollisions(self.entity)
    if push.x ~= 0 or push.y ~= 0 or push.z ~= 0 then
        local rt = World:getTransform(rootEntity)
        if rt then
            local pushLen = math.sqrt(push.x*push.x + push.y*push.y + push.z*push.z)
            local normalY = (pushLen > 0) and (push.y / pushLen) or 0
            if normalY > 0.15 then
                rt.position = rt.position + Vec3(0, push.y, 0)
                self.velocityY = 0.0
                self.grounded = true
            elseif push.y < 0 then
                rt.position = rt.position + push
                self.velocityY = 0.0
            else
                rt.position = rt.position + Vec3(push.x, 0, push.z)
                self.grounded = wasGrounded
            end
            World:setTransform(rootEntity, rt)
        end
    end

    -- Ground snap: probe downward at several depths to stay glued across collider seams.
    if wasGrounded and not self.grounded and self.velocityY <= 0 then
        local rt = World:getTransform(rootEntity)
        if rt then
            local probeSteps = { 0.05, 0.10, 0.15 }
            local baseY = rt.position.y
            local snapped = false
            for _, dist in ipairs(probeSteps) do
                rt.position = Vec3(rt.position.x, baseY - dist, rt.position.z)
                World:setTransform(rootEntity, rt)
                local snapPush = World:resolveCollisions(self.entity)
                local snapLen = math.sqrt(snapPush.x*snapPush.x + snapPush.y*snapPush.y + snapPush.z*snapPush.z)
                local snapNormalY = (snapLen > 0) and (snapPush.y / snapLen) or 0
                if snapNormalY > 0.15 then
                    rt.position = Vec3(rt.position.x, rt.position.y + snapPush.y, rt.position.z)
                    self.velocityY = 0.0
                    self.grounded = true
                    snapped = true
                    break
                end
            end
            if not snapped then
                rt.position = Vec3(rt.position.x, baseY, rt.position.z)
            end
            World:setTransform(rootEntity, rt)
        end
    end

    -- Animation
    if not self.grounded and self.velocityY > 0 then
        self:setAnimState("Jump")
    elseif not self.grounded then
        self:setAnimState(self.moving and "Run" or "Idle")
    elseif self.moving then
        self:setAnimState("Run")
    else
        self:setAnimState("Idle")
    end
end

return M
