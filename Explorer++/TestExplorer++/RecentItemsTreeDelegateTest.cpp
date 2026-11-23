// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/RecentItemsTreeDelegate.h"
#include "MouseEvent.h"
#include "RecentItems/RecentTreeViewNode.h"
#include "ResourceLoaderFake.h"
#include "SimulatedClipboardStore.h"
#include "../Helper/Clipboard.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <utility>
#include <vector>

using namespace testing;

namespace
{

class RecentItemsHostFake : public RecentItemsHost
{
public:
	RecentItemsHostFake(ClipboardStore *clipboardStore) : m_clipboardStore(clipboardStore)
	{
	}

	void NavigateInActiveTab(const RecentTreeViewNode *node) override
	{
		m_navigatedNode = node;
	}

	void OpenLocation(const RecentTreeViewNode *node, bool isCtrlKeyDown,
		bool isShiftKeyDown) override
	{
		m_openedNode = node;
		m_openedWithCtrl = isCtrlKeyDown;
		m_openedWithShift = isShiftKeyDown;
	}

	void ShowContextMenu(const RecentTreeViewNode *node,
		const std::vector<RecentItemsContextMenu::Action> &actions, const POINT &ptScreen) override
	{
		m_contextMenuNode = node;
		m_contextMenuActions = actions;
		m_contextMenuPoint = ptScreen;
	}

	ClipboardStore *GetClipboardStore() override
	{
		return m_clipboardStore;
	}

	const RecentTreeViewNode *GetNavigatedNode() const
	{
		return m_navigatedNode;
	}

	const RecentTreeViewNode *GetOpenedNode() const
	{
		return m_openedNode;
	}

	bool WasOpenedWithCtrl() const
	{
		return m_openedWithCtrl;
	}

	bool WasOpenedWithShift() const
	{
		return m_openedWithShift;
	}

	const RecentTreeViewNode *GetContextMenuNode() const
	{
		return m_contextMenuNode;
	}

	const std::vector<RecentItemsContextMenu::Action> &GetContextMenuActions() const
	{
		return m_contextMenuActions;
	}

	const POINT &GetContextMenuPoint() const
	{
		return m_contextMenuPoint;
	}

private:
	const RecentTreeViewNode *m_navigatedNode = nullptr;
	const RecentTreeViewNode *m_openedNode = nullptr;
	bool m_openedWithCtrl = false;
	bool m_openedWithShift = false;
	const RecentTreeViewNode *m_contextMenuNode = nullptr;
	std::vector<RecentItemsContextMenu::Action> m_contextMenuActions;
	POINT m_contextMenuPoint = {};
	ClipboardStore *const m_clipboardStore;
};

std::unique_ptr<RecentTreeViewNode> CreateTrackedNode(const std::wstring &path)
{
	return std::make_unique<RecentTreeViewNode>(RecentItem(path, RecentItem::Kind::Folder, 0),
		L"Name", path, 0, 0);
}

class RecentItemsTreeDelegateTest : public Test
{
protected:
	RecentItemsTreeDelegateTest() : m_host(&m_clipboardStore)
	{
	}

	SimulatedClipboardStore m_clipboardStore;
	RecentItemsHostFake m_host;
	ResourceLoaderFake m_resourceLoader;
	RecentItemsTreeDelegate m_delegate{ &m_host, &m_resourceLoader };
};

}

TEST_F(RecentItemsTreeDelegateTest, SelectionChangedNavigates)
{
	auto node = CreateTrackedNode(L"C:\\Folder");

	m_delegate.OnSelectionChanged(node.get());

	EXPECT_EQ(m_host.GetNavigatedNode(), node.get());
}

TEST_F(RecentItemsTreeDelegateTest, SuppressedSelectionDoesntNavigate)
{
	auto node = CreateTrackedNode(L"C:\\Folder");

	m_delegate.SetNavigationSuppressed(true);
	m_delegate.OnSelectionChanged(node.get());
	EXPECT_EQ(m_host.GetNavigatedNode(), nullptr);

	m_delegate.SetNavigationSuppressed(false);
	m_delegate.OnSelectionChanged(node.get());
	EXPECT_EQ(m_host.GetNavigatedNode(), node.get());
}

TEST_F(RecentItemsTreeDelegateTest, MiddleClickOpensLocation)
{
	auto node = CreateTrackedNode(L"C:\\Folder");

	// Ctrl+Shift should be passed through to the host.
	m_delegate.OnNodeMiddleClicked(node.get(), MouseEvent({ 0, 0 }, true, true));

	EXPECT_EQ(m_host.GetOpenedNode(), node.get());
	EXPECT_TRUE(m_host.WasOpenedWithCtrl());
	EXPECT_TRUE(m_host.WasOpenedWithShift());
}

TEST_F(RecentItemsTreeDelegateTest, ContextMenuShowsCopyPathAction)
{
	auto node = CreateTrackedNode(L"C:\\Folder");

	auto actions = m_delegate.GetContextMenuActions(node.get());

	ASSERT_EQ(actions.size(), 1u);
}

TEST_F(RecentItemsTreeDelegateTest, CopyPathActionCopiesFullPathToClipboard)
{
	auto node = CreateTrackedNode(L"C:\\Folder");

	auto actions = m_delegate.GetContextMenuActions(node.get());
	ASSERT_EQ(actions.size(), 1u);

	actions[0].callback();

	Clipboard clipboard(&m_clipboardStore);
	auto text = clipboard.ReadText();
	ASSERT_NE(text, std::nullopt);
	EXPECT_EQ(*text, L"C:\\Folder");
}

TEST_F(RecentItemsTreeDelegateTest, ShowContextMenuForwardsToHost)
{
	auto node = CreateTrackedNode(L"C:\\Folder");
	POINT ptScreen = { 100, 200 };

	m_delegate.OnShowContextMenu(node.get(), ptScreen);

	EXPECT_EQ(m_host.GetContextMenuNode(), node.get());
	EXPECT_EQ(m_host.GetContextMenuActions().size(), 1u);
	EXPECT_EQ(m_host.GetContextMenuPoint().x, 100);
	EXPECT_EQ(m_host.GetContextMenuPoint().y, 200);
}
