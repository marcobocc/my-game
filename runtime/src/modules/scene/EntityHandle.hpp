#pragma once
#include <cstdint>
#include <limits>

using EntityHandle = uint64_t;

static constexpr EntityHandle INVALID_ENTITY_HANDLE = std::numeric_limits<EntityHandle>::max();
