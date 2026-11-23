// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItem.h"
#include "TreeViewNode.h"
#include "../Helper/WeakPtrFactory.h"
#include <optional>

// A node in the recent items tree. Tracked nodes represent items from a RecentItemsDataSource;
// chain nodes are display-only nodes that represent the untracked folders in between a tracked
// ancestor and a tracked item.
class RecentTreeViewNode : public TreeViewNode
{
public:
	RecentTreeViewNode(std::optional<RecentItem> item, const std::wstring &text,
		const std::wstring &fullPath, int rank, int iconIndex);

	std::wstring GetText() const override;
	std::optional<std::wstring> MaybeGetEditingText() const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;
	bool IsGhosted() const override;
	bool IsFile() const override;

	std::optional<std::wstring> MaybeGetSecondaryText() const override;
	std::optional<std::wstring> MaybeGetTooltipText() const override;

	// Returns nullptr for a chain node.
	const RecentItem *GetItem() const;
	int GetRank() const;
	const std::wstring &GetFullPath() const;

	// Updates the rank of the node. Note that the node is deliberately not repositioned here; the
	// caller is responsible for repositioning it (once all rank updates have been applied).
	void SetRank(int rank);
	void SetIconIndex(int iconIndex);

	WeakPtr<RecentTreeViewNode> GetWeakPtr();

private:
	std::optional<RecentItem> m_item;
	const std::wstring m_text;
	const std::wstring m_fullPath;
	int m_rank;
	int m_iconIndex;

	WeakPtrFactory<RecentTreeViewNode> m_weakPtrFactory{ this };
};
