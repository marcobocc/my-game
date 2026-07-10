#pragma once
#include <sstream>
#include <string>
#include "IConsoleCommand.hpp"
#include "console/messages/StringResponse.hpp"
#include "graphics/debug/DebugDraw.hpp"

// debug <subcommand> [on|off]
//
// Subcommands:
//   colliders  [on|off]   — toggle box + capsule collider outlines
//   bvh        [on|off]   — toggle BVH tree outlines
//   list                  — show current state of all toggles
//
// With no on/off argument each subcommand acts as a toggle.
class DebugDrawCommand : public IConsoleCommand {
public:
    explicit DebugDrawCommand(DebugDraw& debugDraw) : debugDraw_(debugDraw) {}

    std::string cmdName() const override { return "debug"; }

    void parse(const std::string& args) override { args_ = args; }

    std::unique_ptr<IConsoleMessage> execute() override {
        auto tokens = splitArgs(args_);
        // tokens[0] is always the dummy empty string from splitArgs
        if (tokens.size() < 2) return usage();

        const std::string& sub = tokens[1];

        if (sub == "list") return list();

        if (tokens.size() >= 3) {
            const std::string& val = tokens[2];
            if (val != "on" && val != "off") return usage();
            bool enable = (val == "on");
            return set(sub, enable);
        }

        // No on/off — toggle
        return toggle(sub);
    }

private:
    DebugDraw& debugDraw_;
    std::string args_;

    std::unique_ptr<IConsoleMessage> set(const std::string& sub, bool enable) {
        auto& f = debugDraw_.flags();
        if (sub == "colliders") {
            f.boxColliders = enable;
            f.capsuleColliders = enable;
            return respond("colliders " + std::string(enable ? "on" : "off"));
        }
        if (sub == "bvh") {
            f.bvh = enable;
            return respond("bvh " + std::string(enable ? "on" : "off"));
        }
        return respond("unknown subcommand: " + sub);
    }

    std::unique_ptr<IConsoleMessage> toggle(const std::string& sub) {
        auto& f = debugDraw_.flags();
        if (sub == "colliders") {
            bool next = !(f.boxColliders || f.capsuleColliders);
            f.boxColliders = f.capsuleColliders = next;
            return respond("colliders " + std::string(next ? "on" : "off"));
        }
        if (sub == "bvh") {
            f.bvh = !f.bvh;
            return respond("bvh " + std::string(f.bvh ? "on" : "off"));
        }
        return respond("unknown subcommand: " + sub);
    }

    std::unique_ptr<IConsoleMessage> list() const {
        const auto& f = debugDraw_.flags();
        std::ostringstream ss;
        ss << "colliders : " << (f.boxColliders ? "on" : "off") << "\n";
        ss << "bvh       : " << (f.bvh ? "on" : "off");
        return respond(ss.str());
    }

    std::unique_ptr<IConsoleMessage> usage() const { return respond("usage: debug <colliders|bvh|list> [on|off]"); }

    static std::unique_ptr<IConsoleMessage> respond(const std::string& msg) {
        return std::make_unique<StringResponse>(msg);
    }
};
