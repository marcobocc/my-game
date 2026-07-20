#pragma once

// --- Primitive Geometries ---
static constexpr auto PRIMITIVE_GEOMETRY_CUBE = "_PRIMITIVE_CUBE";
static constexpr auto PRIMITIVE_GEOMETRY_PLANE = "_PRIMITIVE_PLANE";

// --- Textures ---
static constexpr auto EMPTY_TEXTURE = "assets/internal.core/textures/white1x1.jpg";

// --- Materials ---
static constexpr auto SOLID_COLOR_MATERIAL = "assets/internal.core/materials/solid_color.mat";

// --- Shaders ---
static constexpr auto OBJECT_ID_SHADER = "assets/internal.core/shaders/object_id/object_id.shad";
static constexpr auto OBJECT_ID_SKINNED_SHADER = "assets/internal.core/shaders/object_id/object_id_skinned.shad";
static constexpr auto GIZMO_SHADER = "assets/internal.core/shaders/gizmo/gizmo.shad";
static constexpr auto GIZMO_OVERLAY_SHADER = "assets/internal.core/shaders/gizmo/gizmo_overlay.shad";
static constexpr auto GBUFFER_SHADER = "assets/internal.core/shaders/geometry/geometry.shad";
static constexpr auto GBUFFER_SKINNED_SHADER = "assets/internal.core/shaders/geometry_skinned/geometry_skinned.shad";
static constexpr auto GBUFFER_TERRAIN_SHADER = "assets/internal.core/shaders/geometry_terrain/geometry_terrain.shad";
static constexpr auto LIGHTING_SHADER = "assets/internal.core/shaders/lighting/lighting.shad";
static constexpr auto SHADOW_SHADER = "assets/internal.core/shaders/shadow/shadow.shad";
static constexpr auto SHADOW_SKINNED_SHADER = "assets/internal.core/shaders/shadow/shadow_skinned.shad";
static constexpr auto PARTICLES_SHADER = "assets/internal.core/shaders/particles/particles.shad";
static constexpr auto PARTICLES_SPAWN_SHADER = "assets/internal.core/shaders/particles/particles_spawn.shad";
static constexpr auto PARTICLES_SIMULATE_SHADER = "assets/internal.core/shaders/particles/particles_simulate.shad";
static constexpr auto TEXT_SHADER = "assets/internal.core/shaders/text/text.shad";
static constexpr auto UI_SHADER = "assets/internal.core/shaders/ui/ui.shad";
static constexpr auto UI_TEXT_SHADER = "assets/internal.core/shaders/ui/ui_text.shad";

static constexpr auto OUTLINE_SHADER = "assets/internal.editor/shaders/selection_outline/selection_outline.shad";
static constexpr auto SKYDOME_SHADER = "assets/internal.editor/shaders/skydome/skydome.shad";
static constexpr auto GRID_SHADER = "assets/internal.editor/shaders/grid/grid.shad";

// --- Fonts ---
static constexpr auto DEFAULT_FONT = "assets/internal.core/fonts/Arial.ttf";
