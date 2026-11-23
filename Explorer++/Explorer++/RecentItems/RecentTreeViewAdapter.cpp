// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentTreeViewAdapter.h"
#include "RecentItemsIconManager.h"
#include "RecentTreeNode.h"
#include "RecentTreeViewNode.h"
#include "../Helper/ShellHelper.h"
#include <set>

void RecentTreeViewAdapter::Reset(const std::vector<std::unique_ptr<RecentTreeNode>> &roots,
	RecentItemsIconManager *iconManager)
{
	// Remove any existing nodes first. The child pointers are copied here, since removing a node
	// will modify the collection the nodes belong to.
	std::vector<TreeViewNode *> children;

	for (const auto &child : GetRoot()->GetChildren())
	{
		children.push_back(child.get());
	}

	for (auto *child : children)
	{
		RemoveNode(child);
	}

	m_recentTreeNodeMap.clear();

	for (const auto &root : roots)
	{
		AddRecentTreeNodes(root.get(), nullptr, iconManager);
	}
}

void RecentTreeViewAdapter::Update(const std::vector<std::unique_ptr<RecentTreeNode>> &roots,
	RecentItemsIconManager *iconManager)
{
	CurrentNodesMap currentNodes;
	CollectCurrentNodes(GetRoot(), currentNodes);

	m_recentTreeNodeMap.clear();

	// The desired nodes are processed top-down:
	//
	// 1. The rank of each existing node is updated. This is done silently (i.e. nodes aren't
	//    repositioned as part of the update), since repositioning a node while other nodes still
	//    have out-of-date ranks wouldn't converge to the correct order.
	// 2. New nodes are added. They're inserted at their sorted position, based on the ranks
	//    updated in step 1.
	// 3. Existing nodes are repositioned. This also handles re-parenting (a node's new parent may
	//    be one of the nodes added in step 2).
	// 4. Nodes that are no longer desired are removed. Removals are deliberately performed last,
	//    since removing a node also removes its children.
	std::set<std::wstring, CaseInsensitiveLess> desiredPaths;
	std::vector<std::pair<RecentTreeViewNode *, TreeViewNode *>> pendingMoves;

	for (const auto &root : roots)
	{
		UpdateNode(root.get(), nullptr, currentNodes, desiredPaths, pendingMoves, iconManager);
	}

	for (const auto &pendingMove : pendingMoves)
	{
		MoveNode(pendingMove.first, pendingMove.second);
	}

	std::vector<TreeViewNode *> obsoleteNodes;
	CollectObsoleteNodes(GetRoot(), desiredPaths, obsoleteNodes);

	for (auto *obsoleteNode : obsoleteNodes)
	{
		RemoveNode(obsoleteNode);
	}
}

void RecentTreeViewAdapter::CollectCurrentNodes(TreeViewNode *currentNode,
	CurrentNodesMap &currentNodes)
{
	if (!IsRoot(currentNode))
	{
		auto *recentNode = static_cast<RecentTreeViewNode *>(currentNode);
		currentNodes.insert({ recentNode->GetFullPath(), recentNode });
	}

	for (const auto &child : currentNode->GetChildren())
	{
		CollectCurrentNodes(child.get(), currentNodes);
	}
}

void RecentTreeViewAdapter::UpdateNode(const RecentTreeNode *recentTreeNode,
	TreeViewNode *desiredParentViewNode, CurrentNodesMap &currentNodes,
	std::set<std::wstring, CaseInsensitiveLess> &desiredPaths,
	std::vector<std::pair<RecentTreeViewNode *, TreeViewNode *>> &pendingMoves,
	RecentItemsIconManager *iconManager)
{
	desiredPaths.insert(recentTreeNode->GetFullPath());

	RecentTreeViewNode *viewNode = nullptr;
	auto currentItr = currentNodes.find(recentTreeNode->GetFullPath());

	if (currentItr != currentNodes.end())
	{
		// The node already exists, so its rank is updated (silently - nodes are repositioned
		// explicitly once all ranks have been updated).
		viewNode = currentItr->second;
		viewNode->SetRank(recentTreeNode->GetRank());

		auto *currentParent = viewNode->GetParent();
		auto *desiredParent = desiredParentViewNode ? desiredParentViewNode : GetRoot();

		// The node is repositioned below (within its parent, or under its new parent), once all
		// ranks have been updated.
		pendingMoves.emplace_back(viewNode, desiredParent);
	}
	else
	{
		auto node = CreateViewNode(recentTreeNode, iconManager);
		viewNode = static_cast<RecentTreeViewNode *>(
			AddNode(desiredParentViewNode ? desiredParentViewNode : GetRoot(), std::move(node)));
	}

	m_recentTreeNodeMap.emplace(recentTreeNode, viewNode);

	for (const auto &child : recentTreeNode->GetChildren())
	{
		UpdateNode(child.get(), viewNode, currentNodes, desiredPaths, pendingMoves, iconManager);
	}
}

void RecentTreeViewAdapter::CollectObsoleteNodes(TreeViewNode *currentNode,
	const std::set<std::wstring, CaseInsensitiveLess> &desiredPaths,
	std::vector<TreeViewNode *> &obsoleteNodes)
{
	if (IsRoot(currentNode))
	{
		for (const auto &child : currentNode->GetChildren())
		{
			CollectObsoleteNodes(child.get(), desiredPaths, obsoleteNodes);
		}

		return;
	}

	auto *recentNode = static_cast<RecentTreeViewNode *>(currentNode);

	if (!desiredPaths.contains(recentNode->GetFullPath()))
	{
		// The node is no longer desired, so it's removed along with its children (which are
		// deliberately not processed here, since they'll be removed as well).
		obsoleteNodes.push_back(currentNode);
		return;
	}

	for (const auto &child : currentNode->GetChildren())
	{
		CollectObsoleteNodes(child.get(), desiredPaths, obsoleteNodes);
	}
}

void RecentTreeViewAdapter::AddRecentTreeNodes(const RecentTreeNode *recentTreeNode,
	RecentTreeViewNode *parentNode, RecentItemsIconManager *iconManager)
{
	auto node = CreateViewNode(recentTreeNode, iconManager);
	auto *rawNode = static_cast<RecentTreeViewNode *>(
		AddNode(parentNode ? parentNode : GetRoot(), std::move(node)));
	m_recentTreeNodeMap.emplace(recentTreeNode, rawNode);

	for (const auto &child : recentTreeNode->GetChildren())
	{
		AddRecentTreeNodes(child.get(), rawNode, iconManager);
	}
}

std::unique_ptr<RecentTreeViewNode> RecentTreeViewAdapter::CreateViewNode(
	const RecentTreeNode *recentTreeNode, RecentItemsIconManager *iconManager)
{
	const RecentItem *item = recentTreeNode->GetItem();
	std::optional<RecentItem> optionalItem;

	if (item)
	{
		optionalItem = *item;
	}

	auto kind = item ? item->GetKind() : RecentItem::Kind::Folder;
	auto &path = recentTreeNode->GetFullPath();

	// The name is retrieved from the item's pidl, rather than being derived from the parsing
	// path, so that items like virtual folders and drive roots are shown with their proper
	// display names (e.g. "This PC" rather than a GUID-style path). This matches how items are
	// named in the frequent locations menu. The name derived from the path is used as a fallback.
	std::wstring nodeText = recentTreeNode->GetName();

	if (item && item->GetPidl())
	{
		std::wstring displayName;
		HRESULT hr = GetDisplayName(item->GetPidl()->Raw(), SHGDN_NORMAL, displayName);

		if (SUCCEEDED(hr))
		{
			nodeText = displayName;
		}
	}

	// A default icon index is used initially. If the actual icon for the item isn't cached, it
	// will be retrieved in the background and the node will be updated once it's available.
	auto cachedIconIndex = iconManager->MaybeGetCachedIconIndex(path, kind);
	int iconIndex = cachedIconIndex ? *cachedIconIndex : iconManager->GetDefaultIconIndex(kind);

	auto node = std::make_unique<RecentTreeViewNode>(std::move(optionalItem), nodeText, path,
		recentTreeNode->GetRank(), iconIndex);

	if (!cachedIconIndex)
	{
		iconManager->GetIconIndexForPath(path, kind,
			[weakNode = node->GetWeakPtr()](int updatedIconIndex)
			{
				if (!weakNode)
				{
					return;
				}

				weakNode->SetIconIndex(updatedIconIndex);
			});
	}

	return node;
}

RecentTreeViewNode *RecentTreeViewAdapter::MaybeGetNodeForRecentTreeNode(
	const RecentTreeNode *recentTreeNode) const
{
	auto itr = m_recentTreeNodeMap.find(recentTreeNode);

	if (itr == m_recentTreeNodeMap.end())
	{
		return nullptr;
	}

	return itr->second;
}

std::weak_ordering RecentTreeViewAdapter::CompareNodes(const TreeViewNode *first,
	const TreeViewNode *second) const
{
	// All nodes in this adapter are RecentTreeViewNode instances (the root node is never passed
	// here).
	auto *firstNode = static_cast<const RecentTreeViewNode *>(first);
	auto *secondNode = static_cast<const RecentTreeViewNode *>(second);

	// The siblings in the tree built by RecentTreeBuilder are already ordered by rank, so the
	// existing order simply has to be preserved here.
	return firstNode->GetRank() <=> secondNode->GetRank();
}
