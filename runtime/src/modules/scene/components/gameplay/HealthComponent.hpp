#pragma once
#include "BehaviourComponent.hpp"

class HealthComponent final : public BehaviourComponent {
public:
    std::string typeName() const override { return "HealthComponent"; }
    nlohmann::json serialize() const override { return {{"hp", hp_}, {"maxHp", maxHp_}}; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<HealthComponent>(*this); }

    void onStart() override {}
    void onUpdate(float /*dt*/) override {}

    void takeDamage(float amount) {
        hp_ = std::max(0.0f, hp_ - amount);
        logInfo("took " + std::to_string(static_cast<int>(amount)) +
                " damage — hp: " + std::to_string(static_cast<int>(hp_)));
        if (isDead()) this->logWarn("dead");
    }

    void heal(float amount) {
        hp_ = std::min(maxHp_, hp_ + amount);
        logInfo("healed " + std::to_string(static_cast<int>(amount)) +
                " — hp: " + std::to_string(static_cast<int>(hp_)));
    }

    float getHp() const { return hp_; }
    float getMaxHp() const { return maxHp_; }
    bool isDead() const { return hp_ <= 0.0f; }

public:
    float hp_ = 100.0f;
    float maxHp_ = 100.0f;
};
