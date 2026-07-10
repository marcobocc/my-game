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

            props.emplace_back(std::make_unique<InputTextProperty>(
                    "Normal Texture",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).normalTexture; },
                    "texture_asset",
                    [this, asset](const char* tex) {
                        assetStore_.mutateAsset<Material>(asset, [tex](Material& m) { m.normalTexture = tex; });
                    }));

            props.emplace_back(std::make_unique<InputTextProperty>(
                    "Roughness Texture",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).roughnessTexture; },
                    "texture_asset",
                    [this, asset](const char* tex) {
                        assetStore_.mutateAsset<Material>(asset, [tex](Material& m) { m.roughnessTexture = tex; });
                    }));

            props.emplace_back(std::make_unique<InputTextProperty>(
                    "Metallic Texture",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).metallicTexture; },
                    "texture_asset",
                    [this, asset](const char* tex) {
                        assetStore_.mutateAsset<Material>(asset, [tex](Material& m) { m.metallicTexture = tex; });
                    }));

            props.emplace_back(std::make_unique<InputTextProperty>(
                    "AO Texture",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).aoTexture; },
                    "texture_asset",
                    [this, asset](const char* tex) {
                        assetStore_.mutateAsset<Material>(asset, [tex](Material& m) { m.aoTexture = tex; });
                    }));

            props.emplace_back(std::make_unique<SliderFloatProperty>(
                    "Metallic",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).metallic; },
                    [this, asset](float v) {
                        assetStore_.mutateAsset<Material>(asset, [v](Material& m) { m.metallic = v; });
                    }));

            props.emplace_back(std::make_unique<SliderFloatProperty>(
                    "Roughness",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).roughness; },
                    [this, asset](float v) {
                        assetStore_.mutateAsset<Material>(asset, [v](Material& m) { m.roughness = v; });
                    }));

            props.emplace_back(std::make_unique<SliderFloatProperty>(
                    "Ambient Occlusion",
                    [this, asset] { return assetStore_.getAsset<Material>(asset).ao; },
                    [this, asset](float v) {
                        assetStore_.mutateAsset<Material>(asset, [v](Material& m) { m.ao = v; });
                    }));

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
