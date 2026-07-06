-- damage_text.lua
-- Spawns a child entity above the target to display a floating damage number.
-- Call spawnDamageText(parentEntity, amount, offsetY) — offsetY defaults to 2.0.

local BASE_SIZE      = 20.0
local DAMAGE_SCALE   = 1.0
local DISPLAY_DUR    = 3.0
local GROW_PORTION   = 0.15
local SHRINK_PORTION = 0.35
local LINGER_SCALE   = 0.5
local DEFAULT_OFFSET_Y = 150.0
local RANDOM_OFFSET_X  = 50.0
local RANDOM_OFFSET_Y  = 50.0

-- string(entity) -> { entity, displayTimer, peakSize, displayDuration }
-- Entity is opaque userdata; tostring() gives a stable string key.
local active = {}

local M = {}

function M.spawnDamageText(parentEntity, amount, offsetY)
    offsetY = offsetY or DEFAULT_OFFSET_Y

    local textEntity = World:createEntity()
    local key = tostring(textEntity)

    local t = parentEntity:getTransform()

    local randomX = (math.random() * 2.0 - 1.0) * RANDOM_OFFSET_X
    local randomY = (math.random() * 2.0 - 1.0) * RANDOM_OFFSET_Y

    local spawnPos = t and Vec3(
            t.position.x + randomX,
            t.position.y + offsetY + randomY,
            t.position.z
    ) or Vec3(randomX, offsetY + randomY, 0)

    local nt = textEntity:getTransform()
    if nt then
        nt.position = spawnPos
        textEntity:setTransform(nt)
    end
    textEntity:setParent(parentEntity)

    local tc = TextComponent()
    tc.text      = "-" .. tostring(math.floor(amount))
    tc.fontSize  = BASE_SIZE
    tc.billboard = true
    tc.alignment = TextAlign.Center
    tc.r = 1.0
    tc.g = 1.0
    tc.b = 0.0
    tc.a = 1.0
    textEntity:addTextComponent(tc)

    active[key] = {
        entity          = textEntity,
        displayTimer    = DISPLAY_DUR,
        peakSize        = BASE_SIZE + amount * DAMAGE_SCALE,
        displayDuration = DISPLAY_DUR,
    }
end

function M.update(dt)
    for key, state in pairs(active) do
        state.displayTimer = state.displayTimer - dt

        if state.displayTimer <= 0.0 then
            state.entity:destroy()
            active[key] = nil
        else
            local tc = state.entity:getTextComponent()
            if tc then
                local elapsed    = state.displayDuration - state.displayTimer
                local progress   = elapsed / state.displayDuration
                local growEnd    = GROW_PORTION
                local shrinkEnd  = GROW_PORTION + SHRINK_PORTION
                local peakSize   = state.peakSize
                local lingerSize = math.max(BASE_SIZE, peakSize * LINGER_SCALE)

                if progress < growEnd then
                    local t = progress / growEnd
                    t = 1.0 - (1.0 - t) * (1.0 - t)
                    tc.fontSize = BASE_SIZE + (peakSize - BASE_SIZE) * t
                elseif progress < shrinkEnd then
                    local t = (progress - growEnd) / SHRINK_PORTION
                    t = t * t
                    tc.fontSize = peakSize + (lingerSize - peakSize) * t
                else
                    tc.fontSize = lingerSize
                end

                state.entity:setTextComponent(tc)
            end
        end
    end
end

return M
