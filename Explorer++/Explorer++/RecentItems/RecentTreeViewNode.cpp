// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentTreeViewNode.h"

RecentTreeViewNode::RecentTreeViewNode(std::optional<RecentItem> item, const std::wstring &text,
	const std::wstring &fullPath, int rank, int iconIndex) :
	m_item(std::move(item)),
	m_text(text),
	m_fullPath(fullPath),
	m_rank(rank),
	m_iconIndex(iconIndex)
{
}

std::wstring RecentTreeViewNode::GetText() const
{
	return m_text;
}

std::optional<std::wstring> RecentTreeViewNode::MaybeGetEditingText() const
{
	return std::nullopt;
}

std::optional<int> RecentTreeViewNode::GetIconIndex() const
{
	return m_iconIndex;
}

bool RecentTreeViewNode::CanRename() const
{
	return false;
}

bool RecentTreeViewNode::CanRemove() const
{
	return false;
}

bool RecentTreeViewNode::IsGhosted() const
{
	return false;
}

bool RecentTreeViewNode::IsFile() const
{
	return m_item && m_item->GetKind() == RecentItem::Kind::File;
}

std::optional<std::wstring> RecentTreeViewNode::MaybeGetSecondaryText() const
{
	if (!m_item)
	{
		// For a chain node, only the main text is shown, though it's drawn in the smaller font.
		return std::wstring();
	}

	if (m_item->IsVirtualPath())
	{
		// A virtual item doesn't have a meaningful path to display (its parsing path is a
		// GUID-style path), so only the main text is shown.
		return std::nullopt;
	}

	return m_fullPath;
}

std::optional<std::wstring> RecentTreeViewNode::MaybeGetTooltipText() const
{
	if (m_item && m_item->IsVirtualPath())
	{
		return std::nullopt;
	}

	return m_fullPath;
}

const RecentItem *RecentTreeViewNode::GetItem() const
{
	return m_item ? &*m_item : nullptr;
}

int RecentTreeViewNode::GetRank() const
{
	return m_rank;
}

const std::wstring &RecentTreeViewNode::GetFullPath() const
{
	return m_fullPath;
}

void RecentTreeViewNode::SetRank(int rank)
{
	if (rank == m_rank)
	{
		return;
	}

	m_rank = rank;
}

void RecentTreeViewNode::SetIconIndex(int iconIndex)
{
	if (iconIndex == m_iconIndex)
	{
		return;
	}

	m_iconIndex = iconIndex;
	NotifyUpdated();
}

WeakPtr<RecentTreeViewNode> RecentTreeViewNode::GetWeakPtr()
{
	return m_weakPtrFactory.GetWeakPtr();
}
