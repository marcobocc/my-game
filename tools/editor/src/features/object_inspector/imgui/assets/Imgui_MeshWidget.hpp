#pragma once
#include "../../../../services/EditorSelection.hpp"
#include "../../../../services/common_editing/AssetStore.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/asset_management/asset_types/Mesh.hpp"

class Imgui_MeshWidget : public Imgui_InspectorWidget {
public:
    Imgui_MeshWidget(AssetStore& assetStore, EditorSelection& editorSelection) :
        Imgui_InspectorWidget("Mesh"),
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
            const Mesh& mesh = assetStore_.getAsset<Mesh>(asset);

            add<LabelProperty>("Name", [asset] { return std::string(asset); });
            add<SeparatorProperty>();

            const size_t verts = mesh.getVertexCount();
            const size_t tris = mesh.hasIndices() ? mesh.getIndices().size() / 3 : verts / 3;
            add<LabelProperty>("Vertices", [verts] { return std::to_string(verts); });
            add<LabelProperty>("Triangles", [tris] { return std::to_string(tris); });
            add<SeparatorProperty>();

            const bool skinned = mesh.isSkinned();
            add<LabelProperty>("Skinned", [skinned] { return skinned ? std::string("Yes") : std::string("No"); });

            if (skinned) {
                const std::string& skelName = mesh.getSkeletonName();
                add<LabelProperty>("Skeleton", [skelName] { return skelName; });

                const std::vector<std::string>& clips = mesh.getClipNames();
                if (clips.empty()) {
                    add<LabelProperty>("Clips", [] { return std::string("None"); });
                } else {
                    add<LabelProperty>("Clips", [&clips] { return std::to_string(clips.size()); });
                    for (const auto& clip: clips) {
                        auto slash = clip.rfind('/');
                        auto dot = clip.rfind('.');
                        std::string display = (slash != std::string::npos && dot != std::string::npos && dot > slash)
                                                      ? clip.substr(slash + 1, dot - slash - 1)
                                                      : clip;
                        add<LabelProperty>(display, [clip] { return clip; });
                    }
                }
            }

            add<SeparatorProperty>();
            const auto& aabb = mesh.getAABB();
            add<LabelProperty>("AABB Min", [&aabb] {
                return "(" + std::to_string(aabb.min.x) + ", " + std::to_string(aabb.min.y) + ", " +
                       std::to_string(aabb.min.z) + ")";
            });
            add<LabelProperty>("AABB Max", [&aabb] {
                return "(" + std::to_string(aabb.max.x) + ", " + std::to_string(aabb.max.y) + ", " +
                       std::to_string(aabb.max.z) + ")";
            });

        } catch (const std::exception&) {
        }
    }
};
