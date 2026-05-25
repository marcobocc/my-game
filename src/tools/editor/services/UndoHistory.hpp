#pragma once
#include <functional>
#include <log4cxx/logger.h>
#include <random>
#include <string>
#include <vector>

class UndoHistory {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("UndoHistory");

public:
    using Action = std::function<void()>;

    static std::string randomGroupId(const std::string& tag = "") {
        thread_local std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist;
        auto rand = std::to_string(dist(rng));
        return tag.empty() ? rand : tag + "#" + rand;
    }

    void push(Action undo, Action redo, const std::string& description, const std::string& groupId = "");
    void undo();
    void redo();
    void clear();

private:
    struct Entry {
        Action undo;
        Action redo;
        std::string description;
        std::string groupId;
    };

    static std::string entryToString(const Entry& e);
    static std::string groupToString(const std::vector<int>& indices, const std::vector<Entry>& stack);

private:
    std::vector<Entry> stack_;
    int position_ = 0;
};
