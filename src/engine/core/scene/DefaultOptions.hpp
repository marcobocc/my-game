#pragma once
#include <glm/vec3.hpp>

static constexpr glm::vec3 DEFAULT_POSITION{0.0f, 0.0f, 0.0f};
static constexpr glm::vec3 DEFAULT_SCALE{1.0f, 1.0f, 1.0f};
static constexpr glm::vec3 DEFAULT_FORWARD{0.0f, 0.0f, -1.0f};
static constexpr glm::vec3 DEFAULT_UP{0.0f, 1.0f, 0.0f};
static constexpr float DEFAULT_FOV = 45.0f;
static constexpr float DEFAULT_ASPECT = 16.0f / 9.0f;
static constexpr float DEFAULT_NEAR_PLANE = 0.1f;
static constexpr float DEFAULT_FAR_PLANE = 100.0f;
