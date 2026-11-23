// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItem.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

// A single node in the tree built by RecentTreeBuilder.
//
// A tracked node represents an item from a RecentItemsDataSource. A chain node is a display-only
// node that represents the untracked folders between a tracked ancestor and a tracked item; it
// doesn't correspond to any stored item and is never written back to a data source.
class RecentTreeNode
{
public:
	enum class Type
	{
		Tracked,
		Chain
	};

	RecentTreeNode(Type type, const std::wstring &name, const std::wstring &fullPath, int rank,
		std::optional<RecentItem> item);

	Type GetType() const;

	// The name displayed for the node. For a tracked node, this is the final segment of the item
	// path. For a chain node, this is the path in between the tracked ancestor and the tracked
	// item.
	const std::wstring &GetName() const;

	// The full path this node represents. For a chain node, this is the path of the untracked
	// folder it represents.
	const std::wstring &GetFullPath() const;

	// The rank of the node. For a tracked node, this is the rank of the underlying item. For a
	// chain node, this is the rank of its best-ranked child.
	int GetRank() const;

	// Returns nullptr for a chain node.
	const RecentItem *GetItem() const;

	void AddChild(std::unique_ptr<RecentTreeNode> node);
	const std::vector<std::unique_ptr<RecentTreeNode>> &GetChildren() const;

private:
	Type m_type;
	std::wstring m_name;
	std::wstring m_fullPath;
	int m_rank;
	std::optional<RecentItem> m_item;
	std::vector<std::unique_ptr<RecentTreeNode>> m_children;
};
