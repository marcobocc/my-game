#pragma once
#include <memory>
#include <sstream>
#include <string>
#include "IConsoleCommand.hpp"
#include "modules/debug/console/messages/StringResponse.hpp"
#include "modules/scene/World.hpp"

class ListActorsCommand : public IConsoleCommand {
public:
    explicit ListActorsCommand(World& world) : world_(world) {}
    ~ListActorsCommand() override = default;

    void parse(const std::string& args) override {}

    std::unique_ptr<IConsoleMessage> execute() override {
        const auto& actors = world_.getActors();
        std::stringstream ss;
        if (actors.empty()) {
            ss << "(no actors)";
        } else {
            for (size_t i = 0; i < actors.size(); ++i) {
                const auto& actor = actors[i];
                ss << "Actor " << actor->handle();
                const auto& components = actor->getComponents();
                if (!components.empty()) {
                    ss << " [";
                    for (size_t j = 0; j < components.size(); ++j) {
                        if (j > 0) ss << ", ";
                        ss << components[j]->typeName();
                    }
                    ss << "]";
                }
                if (i < actors.size() - 1) ss << "\n";
            }
        }
        return std::make_unique<StringResponse>(ss.str());
    }

    std::string cmdName() const override { return "list-actors"; }

private:
    World& world_;
};
