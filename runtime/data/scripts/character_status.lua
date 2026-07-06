-- character_status.lua
-- Manages health and combat for a character.
-- Set targetCharacter to the entity this character attacks.

Properties = {
    { name = "maxHealth",       type = "float",  default = 100.0 },
    { name = "weaponDamageBase",    type = "float",  default = 15.0  },
    { name = "weaponDamageSpread",    type = "float",  default = 15.0  },
    { name = "targetCharacter", type = "entity", default = 0     },
}

local DamageText = require("damage_text")
local Logger     = require("logger")

local M = {}

function M:onStart()
    self.health = self.maxHealth
end

function M:applyDamage(amount)
    self.health = math.max(0.0, self.health - amount)
    Logger.info("entity " .. tostring(self.entity) ..
                " took " .. tostring(amount) ..
                " damage  (hp=" .. tostring(self.health) .. ")")
    DamageText.spawnDamageText(self.entity, amount)
    if self.health <= 0.0 then
        Logger.info("entity " .. tostring(self.entity) .. " died")
    end
end

function M:attack()
    if not self.targetCharacter or not self.targetCharacter:isValid() then return end
    local targetStatus = self.targetCharacter:getScript("character_status")
    if not targetStatus then
        Logger.warn("character_status: no character_status on target entity " .. tostring(self.targetCharacter))
        return
    end
    local weaponDamage = self.weaponDamage + math.random(-self.weaponDamageSpread, self.weaponDamageSpread);
    targetStatus:applyDamage(weaponDamage)
end

function M:onUpdate(dt)
    DamageText.update(dt)
end

return M
