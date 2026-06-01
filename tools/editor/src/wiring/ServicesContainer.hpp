#pragma once
#include "../services/ClipboardService.hpp"
#include "../services/EditorContext.hpp"
#include "../services/EditorSelection.hpp"
#include "../services/EditorSettings.hpp"
#include "../services/UndoHistory.hpp"
#include "../services/common_editing/AssetQuickActions.hpp"
#include "../services/common_editing/AssetStore.hpp"
#include "../services/common_editing/SceneQuickActions.hpp"

class AssetLoader;
class AssetCache;
class VirtualFileSystem;
struct RendererSettings;

class ServicesContainer {
public:
    explicit ServicesContainer(AssetLoader& assetLoader,
                               RendererSettings& rendererSettings,
                               VirtualFileSystem& vfs,
                               World& entityManager) :
        editorSettings_(rendererSettings),
        assetStore_(assetLoader, vfs, undoHistory_),
        project_(entityManager, editorSelection_, assetStore_, vfs, sceneQuickActions_, undoHistory_),
        sceneQuickActions_(project_.getCurrentScene(), editorSelection_, clipboardService_, assetStore_),
        assetQuickActions_(assetStore_, editorSelection_, undoHistory_) {}

    UndoHistory& undoHistory() { return undoHistory_; }
    EditorSelection& editorSelection() { return editorSelection_; }
    EditorSettings& editorSettings() { return editorSettings_; }
    ClipboardService& clipboardService() { return clipboardService_; }
    AssetStore& assetStore() { return assetStore_; }
    SceneQuickActions& sceneQuickActions() { return sceneQuickActions_; }
    AssetQuickActions& assetQuickActions() { return assetQuickActions_; }
    RuntimeScene& scene() { return project_.getCurrentScene(); }
    EditorContext& project() { return project_; }

private:
    UndoHistory undoHistory_;
    EditorSelection editorSelection_;
    EditorSettings editorSettings_;
    ClipboardService clipboardService_;
    AssetStore assetStore_;
    EditorContext project_;
    SceneQuickActions sceneQuickActions_;
    AssetQuickActions assetQuickActions_;
};
