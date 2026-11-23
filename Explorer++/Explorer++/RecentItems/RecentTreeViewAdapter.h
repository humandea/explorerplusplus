// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TreeViewAdapter.h"
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

class RecentItemsIconManager;
class RecentTreeNode;
class RecentTreeViewNode;

// Bridges the tree built by RecentTreeBuilder and a TreeView instance. The tree can be rebuilt
// from a full set of builder nodes via Reset(), or updated incrementally via Update() (which only
// adds, removes, moves and updates the nodes that actually changed).
class RecentTreeViewAdapter : public TreeViewAdapter
{
public:
	// Replaces the current set of nodes with the nodes produced by RecentTreeBuilder. Icons are
	// retrieved via iconManager; nodes are updated as their icons become available.
	void Reset(const std::vector<std::unique_ptr<RecentTreeNode>> &roots,
		RecentItemsIconManager *iconManager);

	// Updates the tree to match the nodes produced by RecentTreeBuilder, applying only the
	// changes needed to get there (nodes are added, removed, re-parented and re-ranked as
	// necessary; unchanged nodes are left untouched). Icons are retrieved via iconManager; nodes
	// are updated as their icons become available.
	void Update(const std::vector<std::unique_ptr<RecentTreeNode>> &roots,
		RecentItemsIconManager *iconManager);

	RecentTreeViewNode *MaybeGetNodeForRecentTreeNode(const RecentTreeNode *recentTreeNode) const;

protected:
	std::weak_ordering CompareNodes(const TreeViewNode *first,
		const TreeViewNode *second) const override;

private:
	// Provides case-insensitive ordering of paths, for use with associative containers.
	struct CaseInsensitiveLess
	{
		bool operator()(const std::wstring &first, const std::wstring &second) const
		{
			return _wcsicmp(first.c_str(), second.c_str()) < 0;
		}
	};

	using CurrentNodesMap = std::map<std::wstring, RecentTreeViewNode *, CaseInsensitiveLess>;

	void AddRecentTreeNodes(const RecentTreeNode *recentTreeNode, RecentTreeViewNode *parentNode,
		RecentItemsIconManager *iconManager);
	std::unique_ptr<RecentTreeViewNode> CreateViewNode(const RecentTreeNode *recentTreeNode,
		RecentItemsIconManager *iconManager);
	void CollectCurrentNodes(TreeViewNode *currentNode, CurrentNodesMap &currentNodes);
	void UpdateNode(const RecentTreeNode *recentTreeNode, TreeViewNode *desiredParentViewNode,
		CurrentNodesMap &currentNodes, std::set<std::wstring, CaseInsensitiveLess> &desiredPaths,
		std::vector<std::pair<RecentTreeViewNode *, TreeViewNode *>> &pendingMoves,
		RecentItemsIconManager *iconManager);
	void CollectObsoleteNodes(TreeViewNode *currentNode,
		const std::set<std::wstring, CaseInsensitiveLess> &desiredPaths,
		std::vector<TreeViewNode *> &obsoleteNodes);

	std::unordered_map<const RecentTreeNode *, RecentTreeViewNode *> m_recentTreeNodeMap;
};
