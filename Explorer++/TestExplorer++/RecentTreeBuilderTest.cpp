// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/RecentTreeBuilder.h"
#include "RecentItems/DebugRecentItemsDataSource.h"
#include "RecentItems/PathStatusChecker.h"
#include "RecentItems/RecentItem.h"
#include "RecentItems/RecentTreeNode.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace testing;

namespace
{

// Returns the cached status of a path, as configured by the test.
class PathStatusCheckerFake : public PathStatusChecker
{
public:
	void SetStatus(const std::wstring &path, PathStatus status)
	{
		m_statuses.emplace_back(path, status);
	}

	PathStatus GetStatus(const std::wstring &path) const override
	{
		for (const auto &[statusPath, status] : m_statuses)
		{
			if (_wcsicmp(path.c_str(), statusPath.c_str()) == 0)
			{
				return status;
			}
		}

		return PathStatus::Reachable;
	}

private:
	std::vector<std::pair<std::wstring, PathStatus>> m_statuses;
};

// Finds a node anywhere in the tree, based on its full path (matched case-insensitively).
const RecentTreeNode *FindNode(const std::vector<std::unique_ptr<RecentTreeNode>> &roots,
	const std::wstring &fullPath)
{
	for (const auto &root : roots)
	{
		if (_wcsicmp(root->GetFullPath().c_str(), fullPath.c_str()) == 0)
		{
			return root.get();
		}

		const RecentTreeNode *node = FindNode(root->GetChildren(), fullPath);

		if (node)
		{
			return node;
		}
	}

	return nullptr;
}

std::vector<std::wstring> GetChildNames(const RecentTreeNode *node)
{
	std::vector<std::wstring> names;

	for (const auto &child : node->GetChildren())
	{
		names.push_back(child->GetName());
	}

	return names;
}

std::vector<std::wstring> GetRootNames(const std::vector<std::unique_ptr<RecentTreeNode>> &roots)
{
	std::vector<std::wstring> names;

	for (const auto &root : roots)
	{
		names.push_back(root->GetName());
	}

	return names;
}

}

class RecentTreeBuilderTest : public Test
{
protected:
	// The position of an item in the list is its rank.
	void AddFolderItem(const std::wstring &path)
	{
		m_items.emplace_back(path, RecentItem::Kind::Folder, static_cast<int>(m_items.size()));
	}

	void AddFileItem(const std::wstring &path)
	{
		m_items.emplace_back(path, RecentItem::Kind::File, static_cast<int>(m_items.size()));
	}

	// Adds an item with an explicit rank, which doesn't have to match the position of the item in
	// the list.
	void AddItem(const std::wstring &path, RecentItem::Kind kind, int rank)
	{
		m_items.emplace_back(path, kind, rank);
	}

	std::vector<std::unique_ptr<RecentTreeNode>> BuildTree(
		const PathStatusChecker *pathStatusChecker = nullptr) const
	{
		return RecentTreeBuilder::Build(m_items, pathStatusChecker);
	}

	std::vector<RecentItem> m_items;
};

TEST_F(RecentTreeBuilderTest, SingleItemBecomesRootNode)
{
	AddFolderItem(L"C:\\Folder");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetType(), RecentTreeNode::Type::Tracked);
	EXPECT_EQ(roots[0]->GetName(), L"Folder");
	EXPECT_EQ(roots[0]->GetFullPath(), L"C:\\Folder");
	EXPECT_EQ(roots[0]->GetRank(), 0);
	ASSERT_NE(roots[0]->GetItem(), nullptr);
	EXPECT_EQ(roots[0]->GetItem()->GetKind(), RecentItem::Kind::Folder);
	EXPECT_TRUE(roots[0]->GetChildren().empty());
}

TEST_F(RecentTreeBuilderTest, DuplicateItemIsIgnored)
{
	AddFolderItem(L"C:\\Folder");
	AddFolderItem(L"C:\\Folder");

	auto roots = BuildTree();

	EXPECT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetRank(), 0);
}

TEST_F(RecentTreeBuilderTest, MixedCaseDuplicateIsIgnored)
{
	AddFolderItem(L"C:\\Folder");
	AddFolderItem(L"c:\\folder");

	auto roots = BuildTree();

	EXPECT_EQ(roots.size(), 1u);
}

TEST_F(RecentTreeBuilderTest, DuplicateKeepsItemWithBestRank)
{
	// The items here aren't in rank order. The occurrence of the duplicate with the best rank
	// should be kept.
	AddItem(L"C:\\Folder", RecentItem::Kind::Folder, 5);
	AddItem(L"C:\\Folder", RecentItem::Kind::Folder, 2);

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetRank(), 2);
}

TEST_F(RecentTreeBuilderTest, ItemNestedUnderNearestTrackedAncestor)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\B\\C");
	AddFolderItem(L"C:\\A\\B");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetName(), L"A");

	// C:\A\B is tracked, so C:\A\B\C should nest underneath it (rather than underneath C:\A).
	ASSERT_EQ(roots[0]->GetChildren().size(), 1u);
	const RecentTreeNode *bNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(bNode->GetName(), L"B");
	EXPECT_EQ(bNode->GetType(), RecentTreeNode::Type::Tracked);
	ASSERT_EQ(bNode->GetChildren().size(), 1u);
	EXPECT_EQ(bNode->GetChildren()[0]->GetName(), L"C");
}

TEST_F(RecentTreeBuilderTest, ItemWithoutTrackedAncestorIsRoot)
{
	AddFolderItem(L"C:\\A\\B");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetName(), L"B");
	EXPECT_TRUE(roots[0]->GetChildren().empty());
}

TEST_F(RecentTreeBuilderTest, ConsecutiveTrackedLevelsNestDirectly)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\B");
	AddFolderItem(L"C:\\A\\B\\C");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	ASSERT_EQ(roots[0]->GetChildren().size(), 1u);
	const RecentTreeNode *bNode = roots[0]->GetChildren()[0].get();
	ASSERT_EQ(bNode->GetChildren().size(), 1u);

	// No chain nodes should have been inserted.
	EXPECT_EQ(bNode->GetChildren()[0]->GetType(), RecentTreeNode::Type::Tracked);
}

TEST_F(RecentTreeBuilderTest, ChainNodeInsertedForGap)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\untracked\\X");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	ASSERT_EQ(roots[0]->GetChildren().size(), 1u);

	const RecentTreeNode *chainNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(chainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_EQ(chainNode->GetName(), L"untracked");
	EXPECT_EQ(chainNode->GetFullPath(), L"C:\\A\\untracked");
	EXPECT_EQ(chainNode->GetRank(), 1);
	EXPECT_EQ(chainNode->GetItem(), nullptr);

	ASSERT_EQ(chainNode->GetChildren().size(), 1u);
	const RecentTreeNode *xNode = chainNode->GetChildren()[0].get();
	EXPECT_EQ(xNode->GetType(), RecentTreeNode::Type::Tracked);
	EXPECT_EQ(xNode->GetFullPath(), L"C:\\A\\untracked\\X");
}

TEST_F(RecentTreeBuilderTest, ChildrenWithIdenticalGapShareOneChainNode)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\gap\\X");
	AddFolderItem(L"C:\\A\\gap\\Y");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	ASSERT_EQ(roots[0]->GetChildren().size(), 1u);

	const RecentTreeNode *chainNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(chainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_EQ(chainNode->GetName(), L"gap");
	EXPECT_EQ(chainNode->GetFullPath(), L"C:\\A\\gap");

	EXPECT_THAT(GetChildNames(chainNode), ElementsAre(L"X", L"Y"));
}

TEST_F(RecentTreeBuilderTest, ChildrenWithDifferentGapsGetSeparateChainNodes)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\gap1\\X");
	AddFolderItem(L"C:\\A\\gap2\\Y");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	ASSERT_EQ(roots[0]->GetChildren().size(), 2u);

	const RecentTreeNode *firstChainNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(firstChainNode->GetName(), L"gap1");
	EXPECT_THAT(GetChildNames(firstChainNode), ElementsAre(L"X"));

	const RecentTreeNode *secondChainNode = roots[0]->GetChildren()[1].get();
	EXPECT_EQ(secondChainNode->GetName(), L"gap2");
	EXPECT_THAT(GetChildNames(secondChainNode), ElementsAre(L"Y"));
}

TEST_F(RecentTreeBuilderTest, ChainNodeRankIsBestChildRank)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\gap\\X");
	AddFolderItem(L"C:\\A\\D");
	AddFolderItem(L"C:\\A\\gap\\Y");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);

	// The chain node should be ordered based on the rank of its best-ranked child (rank 1), which
	// means it should come before the tracked node with rank 2.
	ASSERT_EQ(roots[0]->GetChildren().size(), 2u);
	EXPECT_EQ(roots[0]->GetChildren()[0]->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_EQ(roots[0]->GetChildren()[0]->GetRank(), 1);
	EXPECT_EQ(roots[0]->GetChildren()[1]->GetName(), L"D");
	EXPECT_EQ(roots[0]->GetChildren()[1]->GetRank(), 2);
}

TEST_F(RecentTreeBuilderTest, SiblingsOrderedByRank)
{
	// C is added before B, so C should appear before B, even though B comes first alphabetically.
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\C");
	AddFolderItem(L"C:\\A\\B");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_THAT(GetChildNames(roots[0].get()), ElementsAre(L"C", L"B"));
}

TEST_F(RecentTreeBuilderTest, ParentSitsAboveHigherRankedChild)
{
	// The child has a better rank than its parent, but should still be nested underneath it.
	AddFolderItem(L"C:\\Parent\\Child");
	AddFolderItem(L"C:\\Parent");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetName(), L"Parent");
	EXPECT_THAT(GetChildNames(roots[0].get()), ElementsAre(L"Child"));
}

TEST_F(RecentTreeBuilderTest, RootNodesOrderedByRank)
{
	AddItem(L"C:\\A", RecentItem::Kind::Folder, 2);
	AddItem(L"C:\\B", RecentItem::Kind::Folder, 0);
	AddItem(L"C:\\C", RecentItem::Kind::Folder, 1);

	auto roots = BuildTree();

	EXPECT_THAT(GetRootNames(roots), ElementsAre(L"B", L"C", L"A"));
}

TEST_F(RecentTreeBuilderTest, VirtualFolderIsAlwaysRoot)
{
	AddFolderItem(L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}");
	AddFolderItem(L"::{645FF040-5081-101B-9F08-00AA002F954E}");

	auto roots = BuildTree();

	EXPECT_EQ(roots.size(), 2u);
}

TEST_F(RecentTreeBuilderTest, VirtualItemIsNeverAncestor)
{
	AddFolderItem(L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}");
	AddFolderItem(L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\Subfolder");

	auto roots = BuildTree();

	// Both items should be root nodes; nothing nests underneath a virtual item.
	EXPECT_EQ(roots.size(), 2u);
	EXPECT_TRUE(roots[0]->GetChildren().empty());
	EXPECT_TRUE(roots[1]->GetChildren().empty());
}

TEST_F(RecentTreeBuilderTest, UnreachableItemIsKept)
{
	PathStatusCheckerFake pathStatusChecker;
	pathStatusChecker.SetStatus(L"\\\\server\\share", PathStatus::Unreachable);

	AddFolderItem(L"\\\\server\\share");

	auto roots = BuildTree(&pathStatusChecker);

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetName(), L"\\\\server\\share");
}

TEST_F(RecentTreeBuilderTest, DeletedItemIsRemoved)
{
	PathStatusCheckerFake pathStatusChecker;
	pathStatusChecker.SetStatus(L"C:\\Gone", PathStatus::Deleted);

	AddFolderItem(L"C:\\Gone");

	auto roots = BuildTree(&pathStatusChecker);

	EXPECT_TRUE(roots.empty());
}

TEST_F(RecentTreeBuilderTest, ChildrenOfDeletedItemReattachToNextTrackedAncestor)
{
	PathStatusCheckerFake pathStatusChecker;
	pathStatusChecker.SetStatus(L"C:\\A\\B", PathStatus::Deleted);

	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\B");
	AddFolderItem(L"C:\\A\\B\\C");

	auto roots = BuildTree(&pathStatusChecker);

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetName(), L"A");

	// The deleted item itself should be gone, with its child reattached to the next tracked
	// ancestor (C:\A) via a recomputed chain node.
	ASSERT_EQ(roots[0]->GetChildren().size(), 1u);
	const RecentTreeNode *chainNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(chainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_EQ(chainNode->GetName(), L"B");
	EXPECT_EQ(chainNode->GetFullPath(), L"C:\\A\\B");
	EXPECT_EQ(chainNode->GetItem(), nullptr);

	EXPECT_THAT(GetChildNames(chainNode), ElementsAre(L"C"));
}

TEST_F(RecentTreeBuilderTest, ChildrenOfDeletedItemBecomeRoots)
{
	PathStatusCheckerFake pathStatusChecker;
	pathStatusChecker.SetStatus(L"C:\\Gone", PathStatus::Deleted);

	AddFolderItem(L"C:\\Gone\\Child");

	auto roots = BuildTree(&pathStatusChecker);

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetName(), L"Child");
}

TEST_F(RecentTreeBuilderTest, DriveRootAncestor)
{
	AddFolderItem(L"D:\\");
	AddFolderItem(L"D:\\Work");
	AddFolderItem(L"D:\\Work\\Other\\X");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_EQ(roots[0]->GetName(), L"D:\\");

	// D:\Work is a direct child of the drive root.
	ASSERT_EQ(roots[0]->GetChildren().size(), 1u);
	const RecentTreeNode *workNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(workNode->GetName(), L"Work");
	EXPECT_EQ(workNode->GetType(), RecentTreeNode::Type::Tracked);

	ASSERT_EQ(workNode->GetChildren().size(), 1u);
	const RecentTreeNode *chainNode = workNode->GetChildren()[0].get();
	EXPECT_EQ(chainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_EQ(chainNode->GetName(), L"Other");
	EXPECT_EQ(chainNode->GetFullPath(), L"D:\\Work\\Other");

	EXPECT_THAT(GetChildNames(chainNode), ElementsAre(L"X"));
}

TEST_F(RecentTreeBuilderTest, UncRootsAreNotPrefixesOfEachOther)
{
	AddFolderItem(L"\\\\server\\share");
	AddFolderItem(L"\\\\server\\share\\data");
	AddFolderItem(L"\\\\server\\share2");

	auto roots = BuildTree();

	// \\server\share2 shouldn't be nested underneath \\server\share.
	ASSERT_EQ(roots.size(), 2u);

	const RecentTreeNode *shareNode = FindNode(roots, L"\\\\server\\share");
	ASSERT_NE(shareNode, nullptr);
	EXPECT_THAT(GetChildNames(shareNode), ElementsAre(L"data"));

	const RecentTreeNode *share2Node = FindNode(roots, L"\\\\server\\share2");
	ASSERT_NE(share2Node, nullptr);
	EXPECT_TRUE(share2Node->GetChildren().empty());
}

TEST_F(RecentTreeBuilderTest, FileIsNeverAncestor)
{
	AddFolderItem(L"C:\\A");
	AddFileItem(L"C:\\A\\file.txt");
	AddFolderItem(L"C:\\A\\file.txt\\Subfolder");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	ASSERT_EQ(roots[0]->GetChildren().size(), 2u);

	const RecentTreeNode *fileNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(fileNode->GetItem()->GetKind(), RecentItem::Kind::File);
	EXPECT_TRUE(fileNode->GetChildren().empty());

	// The item underneath the file path can't nest underneath the file, so it's nested underneath
	// the nearest tracked folder (C:\A) via a chain node representing the file path in between.
	const RecentTreeNode *chainNode = roots[0]->GetChildren()[1].get();
	EXPECT_EQ(chainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_EQ(chainNode->GetName(), L"file.txt");
	EXPECT_THAT(GetChildNames(chainNode), ElementsAre(L"Subfolder"));
}

TEST_F(RecentTreeBuilderTest, FileInsideUntrackedFolderAppearsUnderChainNode)
{
	AddFolderItem(L"C:\\A");
	AddFileItem(L"C:\\A\\untracked\\file.txt");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	ASSERT_EQ(roots[0]->GetChildren().size(), 1u);

	const RecentTreeNode *chainNode = roots[0]->GetChildren()[0].get();
	EXPECT_EQ(chainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_THAT(GetChildNames(chainNode), ElementsAre(L"file.txt"));

	const RecentTreeNode *fileNode = chainNode->GetChildren()[0].get();
	EXPECT_EQ(fileNode->GetItem()->GetKind(), RecentItem::Kind::File);
}

TEST_F(RecentTreeBuilderTest, TrailingSeparatorNormalized)
{
	AddFolderItem(L"C:\\A\\");
	AddFolderItem(L"C:\\A\\B");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_THAT(GetChildNames(roots[0].get()), ElementsAre(L"B"));
}

TEST_F(RecentTreeBuilderTest, TrailingSeparatorTreatedAsDuplicate)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:\\A\\");

	auto roots = BuildTree();

	EXPECT_EQ(roots.size(), 1u);
}

TEST_F(RecentTreeBuilderTest, ForwardSlashesNormalized)
{
	AddFolderItem(L"C:\\A");
	AddFolderItem(L"C:/A/B");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_THAT(GetChildNames(roots[0].get()), ElementsAre(L"B"));
}

TEST_F(RecentTreeBuilderTest, CaseInsensitiveAncestorMatching)
{
	AddFolderItem(L"c:\\projects");
	AddFolderItem(L"C:\\Projects\\App");

	auto roots = BuildTree();

	ASSERT_EQ(roots.size(), 1u);
	EXPECT_THAT(GetChildNames(roots[0].get()), ElementsAre(L"App"));
}

TEST_F(RecentTreeBuilderTest, EmptyPathIsSkipped)
{
	AddFolderItem(L"");
	AddFolderItem(L"C:\\A");

	auto roots = BuildTree();

	EXPECT_EQ(roots.size(), 1u);
}

#ifdef _DEBUG

TEST_F(RecentTreeBuilderTest, DebugDataSourceBuildsExpectedTree)
{
	DebugRecentItemsDataSource dataSource;
	DebugPathStatusChecker pathStatusChecker;

	auto roots = RecentTreeBuilder::Build(dataSource.GetItems(), &pathStatusChecker);

	// The tracked parent with the worse rank should sit above its children.
	const RecentTreeNode *projectsNode = FindNode(roots, L"C:\\Projects");
	ASSERT_NE(projectsNode, nullptr);

	// Once C:\Projects\App and C:\Projects\App\src are tracked, the deep items at the top of the
	// list should nest directly underneath them.
	const RecentTreeNode *srcNode = FindNode(roots, L"C:\\Projects\\App\\src");
	ASSERT_NE(srcNode, nullptr);
	const RecentTreeNode *uiNode = FindNode(roots, L"C:\\Projects\\App\\src\\ui");
	ASSERT_NE(uiNode, nullptr);

	// The two test items should share a single chain node.
	const RecentTreeNode *appNode = FindNode(roots, L"C:\\Projects\\App");
	ASSERT_NE(appNode, nullptr);
	const RecentTreeNode *testsChainNode = FindNode(roots, L"C:\\Projects\\App\\tests");
	ASSERT_NE(testsChainNode, nullptr);
	EXPECT_EQ(testsChainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_THAT(GetChildNames(testsChainNode), ElementsAre(L"unit", L"integration"));

	// C:\Tools should contain two chains: the archives items share the gap "archives" (their
	// parent folder is the same), while the logs item has its own gap.
	const RecentTreeNode *toolsNode = FindNode(roots, L"C:\\Tools");
	ASSERT_NE(toolsNode, nullptr);
	EXPECT_THAT(GetChildNames(toolsNode),
		ElementsAre(L"archives", L"logs", L"notes.txt", L"build"));

	const RecentTreeNode *archivesChainNode = FindNode(roots, L"C:\\Tools\\archives");
	ASSERT_NE(archivesChainNode, nullptr);
	EXPECT_EQ(archivesChainNode->GetType(), RecentTreeNode::Type::Chain);
	EXPECT_THAT(GetChildNames(archivesChainNode), ElementsAre(L"2023", L"2024"));

	// The deleted item should be removed, with its child reattached as a root node.
	EXPECT_EQ(FindNode(roots, L"C:\\DeletedFolder"), nullptr);
	const RecentTreeNode *deletedChildNode = FindNode(roots, L"C:\\DeletedFolder\\child");
	ASSERT_NE(deletedChildNode, nullptr);
	EXPECT_THAT(GetRootNames(roots), Contains(L"child"));

	// The unreachable item should be kept.
	EXPECT_NE(FindNode(roots, L"\\\\offline\\volume\\docs"), nullptr);

	// The virtual items should be root nodes.
	EXPECT_NE(FindNode(roots, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), nullptr);
	EXPECT_NE(FindNode(roots, L"::{645FF040-5081-101B-9F08-00AA002F954E}"), nullptr);
}

#endif
