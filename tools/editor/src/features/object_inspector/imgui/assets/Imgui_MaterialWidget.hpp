#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "../../../../../../../runtime/src/graphics/assets/Material.hpp"
#include "../../../../services/EditorSelection.hpp"
#include "../../../../services/common_editing/AssetStore.hpp"
#include "../Imgui_InspectorWidget.hpp"

class Imgui_MaterialWidget : public Imgui_InspectorWidget {
public:
    Imgui_MaterialWidget(AssetStore& assetStore, EditorSelection& editorSelection) :
        Imgui_InspectorWidget("Material"),
        assetStore_(assetStore),
        editorSelection_(editorSelection) {}

private:
    AssetStore& assetStore_;
    EditorSelection& editorSelection_;

    void buildProperties() override {
        const auto& inspected = editorSelection_.getInspectedAsset();
        if (!inspected.has_value()) return;
        const AssetHandle& asset = *inspected;

        try {
            assetStore_.getAsset<Material>(asset); // validate exists
            bool immutable = !AssetStore::isMutable(asset);

            std::vector<std::unique_ptr<InspectorProperty>> props;

            props.emplace_back(std::make_unique<LabelProperty>("Name", [asset] { return std::string(asset); }));

            props.emplace_back(std::make_unique<InputTextProperty>(
                    "Albedo",
                    [this, asset] {
                        const Material& m = assetStore_.getAsset<Material>(asset);
                        return (m.albedoTexture == EMPTY_TEXTURE) ? std::string{} : m.albedoTexture;
                    },
                    "texture_asset",
                    [this, asset](const char* tex) {
                        assetStore_.mutateAsset<Material>(asset, [tex](Material& m) { m.albedoTexture = tex; });
                    }));

            props.emplace_back(std::make_unique<ColorEdit4Property>(
                    "Tint",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).tint; },
                    [this, asset](glm::vec4 v) {
                        assetStore_.mutateAsset<Material>(asset, [v](Material& m) { m.tint = v; });
                    }));

            props.emplace_back(std::make_unique<SeparatorProperty>());

            props.emplace_back(std::make_unique<CheckboxProperty>(
                    "Scale Invariant UV",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).scaleInvariantUV; },
                    [this, asset](bool v) {
                        assetStore_.mutateAsset<Material>(asset, [v](Material& m) { m.scaleInvariantUV = v; });
                    }));

            props.emplace_back(std::make_unique<Vec2Property>(
                    "Tiling",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).tiling; },
                    [this, asset](glm::vec2 v) {
                        assetStore_.mutateAsset<Material>(asset, [v](Material& m) { m.tiling = v; });
                    }));

            props.emplace_back(std::make_unique<Vec2Property>(
                    "Offset",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).offset; },
                    [this, asset](glm::vec2 v) {
                        assetStore_.mutateAsset<Material>(asset, [v](Material& m) { m.offset = v; });
                    }));

            if (assetStore_.getAsset<Material>(asset).isTerrainBlend()) {
                props.emplace_back(std::make_unique<SeparatorProperty>());
                props.emplace_back(std::make_unique<LabelProperty>(
                        "", [] { return std::string("Terrain Paint Layers (drag materials from the Assets panel)"); }));

                auto layerProperty = [this, asset](const char* label, auto getter, auto setter) {
                    return std::make_unique<InputTextProperty>(
                            label,
                            [this, asset, getter] {
                                const Material& m = assetStore_.getAsset<Material>(asset);
                                std::string mat = getter(m);
                                return (mat == SOLID_COLOR_MATERIAL) ? std::string{} : mat;
                            },
                            "MATERIAL_ASSET",
                            [this, asset, setter](const char* mat) {
                                assetStore_.mutateAsset<Material>(asset,
                                                                  [mat, setter](Material& m) { setter(m, mat); });
                            });
                };

                props.emplace_back(layerProperty(
                        "Layer 0",
                        [](const Material& m) { return m.layerMaterial0; },
                        [](Material& m, const char* mat) { m.layerMaterial0 = mat; }));
                props.emplace_back(layerProperty(
                        "Layer 1",
                        [](const Material& m) { return m.layerMaterial1; },
                        [](Material& m, const char* mat) { m.layerMaterial1 = mat; }));
                props.emplace_back(layerProperty(
                        "Layer 2",
                        [](const Material& m) { return m.layerMaterial2; },
                        [](Material& m, const char* mat) { m.layerMaterial2 = mat; }));
                props.emplace_back(layerProperty(
                        "Layer 3",
                        [](const Material& m) { return m.layerMaterial3; },
                        [](Material& m, const char* mat) { m.layerMaterial3 = mat; }));
            }

            if (immutable) {
                addRaw(std::make_unique<DisabledGroupProperty>(std::move(props)));
                add<SpacingProperty>();
                add<LabelProperty>("", [] { return std::string("(Builtin materials cannot be edited)"); });
            } else {
                for (auto& p: props)
                    addRaw(std::move(p));
            }

        } catch (const std::exception&) {
        }
    }
};
