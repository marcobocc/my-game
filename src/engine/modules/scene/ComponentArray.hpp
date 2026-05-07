#pragma once
#include <cstdint>
#include <limits>
#include <vector>

template<typename Component>
class ComponentArray {
public:
    void clear() {
        sparse_.clear();
        dense_.clear();
        denseIndices_.clear();
    }

    Component* add(uint32_t index, const Component& value = Component{}) {
        if (index >= sparse_.size()) sparse_.resize(index + 1, INVALID_SPARSE);
        if (has(index)) return get(index);
        sparse_[index] = dense_.size();
        denseIndices_.push_back(index);
        dense_.push_back(value);
        return &dense_.back();
    }

    void remove(uint32_t index) {
        if (!has(index)) return;
        size_t denseIndex = sparse_[index];
        size_t last = dense_.size() - 1;
        dense_[denseIndex] = dense_[last];
        sparse_[denseIndices_[last]] = denseIndex;
        denseIndices_[denseIndex] = denseIndices_[last];
        dense_.pop_back();
        denseIndices_.pop_back();
        sparse_[index] = INVALID_SPARSE;
    }

    Component* get(uint32_t index) {
        if (!has(index)) return nullptr;
        return &dense_[sparse_[index]];
    }

    const Component* get(uint32_t index) const {
        if (!has(index)) return nullptr;
        return &dense_[sparse_[index]];
    }

    bool has(uint32_t index) const { return index < sparse_.size() && sparse_[index] != INVALID_SPARSE; }

private:
    static constexpr size_t INVALID_SPARSE = std::numeric_limits<size_t>::max();
    std::vector<size_t> sparse_;
    std::vector<uint32_t> denseIndices_;
    std::vector<Component> dense_;
};
