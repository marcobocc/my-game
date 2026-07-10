#pragma once
#include <algorithm>
#include <glm/glm.hpp>
#include <vector>
#include "AABB.hpp"

struct Item {
    AABB aabb;
    std::string objectID;
};

struct BVHNode {
    AABB bounds;
    int left = -1;
    int right = -1;
    uint32_t begin = 0;
    uint32_t count = 0;

    bool isLeaf() const { return count > 0; }
};

class BVH {
public:
    std::vector<BVHNode> nodes;
    std::vector<Item> items;

    void build(std::vector<Item> input) {
        items = std::move(input);
        nodes.clear();
        nodes.reserve(items.size() * 2);
        buildNode(0, static_cast<uint32_t>(items.size()));
    }

private:
    static void expand(AABB& subject, const AABB& other) {
        subject.min = glm::min(subject.min, other.min);
        subject.max = glm::max(subject.max, other.max);
    }

    int buildNode(uint32_t start, uint32_t end) {
        BVHNode node;
        node.bounds = AABB();


        for (uint32_t i = start; i < end; i++)
            expand(node.bounds, items[i].aabb);

        int index = nodes.size();
        nodes.push_back(node);

        uint32_t count = end - start;
        if (count <= 4) {
            nodes[index].begin = start;
            nodes[index].count = count;
            return index;
        }

        glm::vec3 extent = node.bounds.max - node.bounds.min;
        int axis = 0;
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent[axis]) axis = 2;
        uint32_t mid = start + count / 2;
        std::nth_element(
                items.begin() + start, items.begin() + mid, items.begin() + end, [&](const Item& a, const Item& b) {
                    return a.aabb.getCentroid()[axis] < b.aabb.getCentroid()[axis];
                });

        int left = buildNode(start, mid);
        int right = buildNode(mid, end);

        nodes[index].left = left;
        nodes[index].right = right;
        return index;
    }
};
