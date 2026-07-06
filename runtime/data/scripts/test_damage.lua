-- test_damage.lua
-- Simulates receiving damage every 5 seconds. Spawns a TextComponent on this
-- entity to show the damage amount, then removes it after 3 seconds.

Properties = {
    { name = "damageInterval",  type = "float", default = 5.0 },
    { name = "displayDuration", type = "float", default = 3.0 },
}

local M = {}

function M:onStart()
    self.timer        = 0.0
    self.displayTimer = 0.0
    self.showing      = false
end

function M:showDamage(amount)
    local tc = TextComponent()
    tc.text      = "-" .. tostring(amount)
    tc.fontSize  = 14.0
    tc.billboard = true
    World:addTextComponent(self.entity, tc)

    self.showing      = true
    self.displayTimer = self.displayDuration
end

function M:hideText()
    World:removeTextComponent(self.entity)
    self.showing = false
end

function M:onUpdate(dt)
    self.timer = self.timer + dt

    if self.timer >= self.damageInterval then
        self.timer = self.timer - self.damageInterval
        if self.showing then self:hideText() end
        local dmg = math.random(5, 50)
        self:showDamage(dmg)
    end

    if self.showing then
        self.displayTimer = self.displayTimer - dt
        if self.displayTimer <= 0.0 then
            self:hideText()
        end
    end
end

return M
