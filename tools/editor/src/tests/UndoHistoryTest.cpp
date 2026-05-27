#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "../services/UndoHistory.hpp"

using ::testing::InSequence;

class MockAction {
public:
    MOCK_METHOD(void, Call, ());
};

class UndoHistoryTest : public ::testing::Test {
protected:
    UndoHistory history;
    MockAction action1;
    MockAction action2;
    MockAction action3;
};

TEST_F(UndoHistoryTest, TwoDifferentGroupsDifferentUndo) {
    std::string groupId1 = UndoHistory::randomGroupId("group1");
    std::string groupId2 = UndoHistory::randomGroupId("group2");

    history.push([this]() { action1.Call(); }, [this]() {}, "Action 1", groupId1);
    history.push([this]() { action2.Call(); }, [this]() {}, "Action 2", groupId2);

    EXPECT_CALL(action2, Call()).Times(1);
    history.undo();

    EXPECT_CALL(action1, Call()).Times(1);
    history.undo();
}

TEST_F(UndoHistoryTest, TwoEmptyGroupsDifferentUndo) {
    history.push([this]() { action1.Call(); }, [this]() {}, "Action 1", "");
    history.push([this]() { action2.Call(); }, [this]() {}, "Action 2", "");

    EXPECT_CALL(action2, Call()).Times(1);
    history.undo();

    EXPECT_CALL(action1, Call()).Times(1);
    history.undo();
}

TEST_F(UndoHistoryTest, TwoSameGroupUndoTogether) {
    std::string groupId = UndoHistory::randomGroupId("group");

    history.push([this]() { action1.Call(); }, [this]() {}, "Action 1", groupId);
    history.push([this]() { action2.Call(); }, [this]() {}, "Action 2", groupId);

    InSequence seq;
    EXPECT_CALL(action2, Call()).Times(1);
    EXPECT_CALL(action1, Call()).Times(1);

    history.undo();
}

TEST_F(UndoHistoryTest, TwoDifferentGroupsDifferentRedo) {
    std::string groupId1 = UndoHistory::randomGroupId("group1");
    std::string groupId2 = UndoHistory::randomGroupId("group2");

    history.push([this]() { action1.Call(); }, [this]() { action1.Call(); }, "Action 1", groupId1);
    history.push([this]() { action2.Call(); }, [this]() { action2.Call(); }, "Action 2", groupId2);

    history.undo();
    history.undo();

    EXPECT_CALL(action1, Call()).Times(1);
    history.redo();

    EXPECT_CALL(action2, Call()).Times(1);
    history.redo();
}

TEST_F(UndoHistoryTest, TwoEmptyGroupsDifferentRedo) {
    history.push([this]() { action1.Call(); }, [this]() { action1.Call(); }, "Action 1", "");
    history.push([this]() { action2.Call(); }, [this]() { action2.Call(); }, "Action 2", "");

    history.undo();
    history.undo();

    EXPECT_CALL(action1, Call()).Times(1);
    history.redo();

    EXPECT_CALL(action2, Call()).Times(1);
    history.redo();
}

TEST_F(UndoHistoryTest, TwoSameGroupRedoTogether) {
    std::string groupId = UndoHistory::randomGroupId("group");

    history.push([this]() { action1.Call(); }, [this]() { action1.Call(); }, "Action 1", groupId);
    history.push([this]() { action2.Call(); }, [this]() { action2.Call(); }, "Action 2", groupId);

    history.undo();

    InSequence seq;
    EXPECT_CALL(action1, Call()).Times(1);
    EXPECT_CALL(action2, Call()).Times(1);

    history.redo();
}

TEST_F(UndoHistoryTest, ThreeActionsMixedGroupsUndoSequence) {
    std::string groupId1 = UndoHistory::randomGroupId("group1");
    std::string groupId2 = UndoHistory::randomGroupId("group2");

    history.push([this]() { action1.Call(); }, [this]() { action1.Call(); }, "Action 1", groupId1);
    history.push([this]() { action2.Call(); }, [this]() { action2.Call(); }, "Action 2", groupId2);
    history.push([this]() { action3.Call(); }, [this]() { action3.Call(); }, "Action 3", groupId1);

    EXPECT_CALL(action3, Call()).Times(1);
    history.undo();

    EXPECT_CALL(action2, Call()).Times(1);
    history.undo();

    EXPECT_CALL(action1, Call()).Times(1);
    history.undo();
}

TEST_F(UndoHistoryTest, ThreeActionsGroupedUndoTogether) {
    std::string groupId = UndoHistory::randomGroupId("group");

    history.push([this]() { action1.Call(); }, [this]() { action1.Call(); }, "Action 1", groupId);
    history.push([this]() { action2.Call(); }, [this]() { action2.Call(); }, "Action 2", groupId);
    history.push([this]() { action3.Call(); }, [this]() { action3.Call(); }, "Action 3", groupId);

    InSequence seq;
    EXPECT_CALL(action3, Call()).Times(1);
    EXPECT_CALL(action2, Call()).Times(1);
    EXPECT_CALL(action1, Call()).Times(1);

    history.undo();
}

TEST_F(UndoHistoryTest, RandomGroupIdGeneratesUniqueIds) {
    auto groupId1 = UndoHistory::randomGroupId("test");
    auto groupId2 = UndoHistory::randomGroupId("test");

    EXPECT_NE(groupId1, groupId2);
    EXPECT_TRUE(groupId1.find("test#") == 0);
    EXPECT_TRUE(groupId2.find("test#") == 0);
}

TEST_F(UndoHistoryTest, ClearHistoryResetsState) {
    history.push([this]() { action1.Call(); }, [this]() {}, "Action 1");
    history.push([this]() { action2.Call(); }, [this]() {}, "Action 2");

    history.clear();

    EXPECT_CALL(action1, Call()).Times(0);
    EXPECT_CALL(action2, Call()).Times(0);
    history.undo();
}

TEST_F(UndoHistoryTest, UndoAtBeginningDoesNothing) {
    EXPECT_CALL(action1, Call()).Times(0);
    history.undo();
}

TEST_F(UndoHistoryTest, RedoAtEndDoesNothing) {
    history.push([this]() { action1.Call(); }, [this]() { action1.Call(); }, "Action 1");

    EXPECT_CALL(action1, Call()).Times(0);
    history.redo();
}
