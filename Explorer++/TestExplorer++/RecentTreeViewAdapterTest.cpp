// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/RecentTreeViewAdapter.h"
#include "IconFetcherFake.h"
#include "RecentItems/DebugRecentItemsDataSource.h"
#include "RecentItems/RecentItem.h"
#include "RecentItems/RecentItemsIconManager.h"
#include "RecentItems/RecentTreeBuilder.h"
#include "RecentItems/RecentTreeViewNode.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace testing;

// The debug data source (and its associated status checker) are only compiled into debug builds.
#ifdef _DEBUG

namespace
{

const RecentTreeViewNode *FindNode(const TreeViewNode *parentNode, const std::wstring &fullPath)
{
	for (const auto &child : parentNode->GetChildren())
	{
		auto *recentNode = static_cast<const RecentTreeViewNode *>(child.get());

		if (_wcsicmp(recentNode->GetFullPath().c_str(), fullPath.c_str()) == 0)
		{
			return recentNode;
		}

		const RecentTreeViewNode *node = FindNode(recentNode, fullPath);

		if (node)
		{
			return node;
		}
	}

	return nullptr;
}

std::vector<std::wstring> GetChildNames(const TreeViewNode *parentNode)
{
	std::vector<std::wstring> names;

	for (const auto &child : parentNode->GetChildren())
	{
		names.push_back(child->GetText());
	}

	return names;
}

class RecentTreeViewAdapterTest : public Test
{
protected:
	RecentTreeViewAdapterTest() : m_iconManager(&m_iconFetcher, 16, 16)
	{
	}

	void BuildTree()
	{
		DebugRecentItemsDataSource dataSource;
		auto roots = RecentTreeBuilder::Build(dataSource.GetItems(), &m_pathStatusChecker);
		m_adapter.Reset(roots, &m_iconManager);
	}

	IconFetcherFake m_iconFetcher;
	DebugPathStatusChecker m_pathStatusChecker;
	RecentItemsIconManager m_iconManager;
	RecentTreeViewAdapter m_adapter;
};

}

TEST_F(RecentTreeViewAdapterTest, NodesAreCreatedFromBuilderTree)
{
	BuildTree();

	// A tracked node should show the item name, with the full path as the secondary text.
	const RecentTreeViewNode *uiNode = FindNode(m_adapter.GetRoot(), L"C:\\Projects\\App\\src\\ui");
	ASSERT_NE(uiNode, nullptr);
	EXPECT_EQ(uiNode->GetText(), L"ui");
	ASSERT_TRUE(uiNode->MaybeGetSecondaryText());
	EXPECT_EQ(*uiNode->MaybeGetSecondaryText(), L"C:\\Projects\\App\\src\\ui");
	EXPECT_EQ(*uiNode->MaybeGetTooltipText(), L"C:\\Projects\\App\\src\\ui");
	EXPECT_FALSE(uiNode->IsFile());
	ASSERT_NE(uiNode->GetItem(), nullptr);
	EXPECT_EQ(uiNode->GetItem()->GetKind(), RecentItem::Kind::Folder);
	EXPECT_FALSE(uiNode->CanRename());
	EXPECT_FALSE(uiNode->CanRemove());

	// A file node should be identified as a file.
	const RecentTreeViewNode *fileNode =
		FindNode(m_adapter.GetRoot(), L"C:\\Projects\\App\\src\\ui\\main.cpp");
	ASSERT_NE(fileNode, nullptr);
	EXPECT_TRUE(fileNode->IsFile());
	ASSERT_NE(fileNode->GetItem(), nullptr);
	EXPECT_EQ(fileNode->GetItem()->GetKind(), RecentItem::Kind::File);

	// A chain node should show only the gap, in the smaller font (indicated by an empty secondary
	// text value).
	const RecentTreeViewNode *testsChainNode =
		FindNode(m_adapter.GetRoot(), L"C:\\Projects\\App\\tests");
	ASSERT_NE(testsChainNode, nullptr);
	EXPECT_EQ(testsChainNode->GetText(), L"tests");
	ASSERT_TRUE(testsChainNode->MaybeGetSecondaryText());
	EXPECT_TRUE(testsChainNode->MaybeGetSecondaryText()->empty());
	EXPECT_EQ(testsChainNode->GetItem(), nullptr);
	EXPECT_EQ(*testsChainNode->MaybeGetTooltipText(), L"C:\\Projects\\App\\tests");
}

TEST_F(RecentTreeViewAdapterTest, SiblingsAreOrderedByRank)
{
	BuildTree();

	// The children of C:\Tools should be ordered by rank (i.e. the order in the source list).
	const RecentTreeViewNode *toolsNode = FindNode(m_adapter.GetRoot(), L"C:\\Tools");
	ASSERT_NE(toolsNode, nullptr);
	EXPECT_THAT(GetChildNames(toolsNode),
		ElementsAre(L"archives", L"logs", L"notes.txt", L"build"));
}

TEST_F(RecentTreeViewAdapterTest, ResetReplacesExistingNodes)
{
	BuildTree();
	BuildTree();

	// Rebuilding shouldn't result in any duplicate nodes.
	const RecentTreeViewNode *srcNode = FindNode(m_adapter.GetRoot(), L"C:\\Projects\\App\\src");
	ASSERT_NE(srcNode, nullptr);
	EXPECT_THAT(GetChildNames(srcNode), ElementsAre(L"ui"));

	const RecentTreeViewNode *uiNode = FindNode(m_adapter.GetRoot(), L"C:\\Projects\\App\\src\\ui");
	ASSERT_NE(uiNode, nullptr);
	EXPECT_THAT(GetChildNames(uiNode), ElementsAre(L"main.cpp", L"widgets", L"themes", L"main.h"));
}

class RecentTreeViewAdapterUpdateTest : public Test
{
protected:
	RecentTreeViewAdapterUpdateTest() : m_iconManager(&m_iconFetcher, 16, 16)
	{
	}

	void UpdateTree(const std::vector<std::wstring> &paths)
	{
		std::vector<RecentItem> items;
		int rank = 0;

		for (const auto &path : paths)
		{
			items.emplace_back(path, RecentItem::Kind::Folder, rank++);
		}

		auto roots = RecentTreeBuilder::Build(items, nullptr);
		m_adapter.Update(roots, &m_iconManager);
	}

	IconFetcherFake m_iconFetcher;
	RecentItemsIconManager m_iconManager;
	RecentTreeViewAdapter m_adapter;
};

TEST_F(RecentTreeViewAdapterUpdateTest, UpdateWithNoChangesPreservesNodes)
{
	UpdateTree({ L"C:\\A", L"C:\\A\\B" });

	auto *nodeA = FindNode(m_adapter.GetRoot(), L"C:\\A");
	ASSERT_NE(nodeA, nullptr);

	int numAdded = 0;
	int numRemoved = 0;
	int numMoved = 0;
	m_adapter.nodeAddedSignal.AddObserver([&numAdded](TreeViewNode *) { numAdded++; });
	m_adapter.nodeRemovedSignal.AddObserver([&numRemoved](TreeViewNode *) { numRemoved++; });
	m_adapter.nodeMovedSignal.AddObserver([&numMoved](TreeViewNode *, const TreeViewNode *, size_t,
											  const TreeViewNode *, size_t) { numMoved++; });

	UpdateTree({ L"C:\\A", L"C:\\A\\B" });

	EXPECT_EQ(numAdded, 0);
	EXPECT_EQ(numRemoved, 0);
	EXPECT_EQ(numMoved, 0);

	// The existing nodes should have been preserved (rather than being removed and re-added).
	EXPECT_EQ(FindNode(m_adapter.GetRoot(), L"C:\\A"), nodeA);
}

TEST_F(RecentTreeViewAdapterUpdateTest, UpdateAddsAndRemovesNodes)
{
	UpdateTree({ L"C:\\A" });

	int numAdded = 0;
	int numRemoved = 0;
	m_adapter.nodeAddedSignal.AddObserver([&numAdded](TreeViewNode *) { numAdded++; });
	m_adapter.nodeRemovedSignal.AddObserver([&numRemoved](TreeViewNode *) { numRemoved++; });

	UpdateTree({ L"C:\\A", L"C:\\B" });

	EXPECT_EQ(numAdded, 1);
	EXPECT_EQ(numRemoved, 0);
	ASSERT_NE(FindNode(m_adapter.GetRoot(), L"C:\\B"), nullptr);

	UpdateTree({ L"C:\\A" });

	EXPECT_EQ(numAdded, 1);
	EXPECT_EQ(numRemoved, 1);
	EXPECT_EQ(FindNode(m_adapter.GetRoot(), L"C:\\B"), nullptr);
}

TEST_F(RecentTreeViewAdapterUpdateTest, UpdateReParentsNodes)
{
	// With X untracked, Y is a root node.
	UpdateTree({ L"C:\\X\\Y" });

	int numMoved = 0;
	m_adapter.nodeMovedSignal.AddObserver([&numMoved](TreeViewNode *, const TreeViewNode *, size_t,
											  const TreeViewNode *, size_t) { numMoved++; });

	// Once X is tracked, Y should be moved underneath it.
	UpdateTree({ L"C:\\X", L"C:\\X\\Y" });

	EXPECT_EQ(numMoved, 1);

	auto *nodeX = FindNode(m_adapter.GetRoot(), L"C:\\X");
	auto *nodeY = FindNode(m_adapter.GetRoot(), L"C:\\X\\Y");
	ASSERT_NE(nodeX, nullptr);
	ASSERT_NE(nodeY, nullptr);
	EXPECT_EQ(nodeY->GetParent(), nodeX);
}

TEST_F(RecentTreeViewAdapterUpdateTest, UpdateRepositionsNodesOnRankChange)
{
	UpdateTree({ L"C:\\A", L"C:\\B" });
	EXPECT_THAT(GetChildNames(m_adapter.GetRoot()), ElementsAre(L"A", L"B"));

	// B now has a better rank than A, so the nodes should swap position.
	UpdateTree({ L"C:\\B", L"C:\\A" });
	EXPECT_THAT(GetChildNames(m_adapter.GetRoot()), ElementsAre(L"B", L"A"));
}

#endif
