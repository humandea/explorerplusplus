// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentTreeNode.h"

RecentTreeNode::RecentTreeNode(Type type, const std::wstring &name, const std::wstring &fullPath,
	int rank, std::optional<RecentItem> item) :
	m_type(type),
	m_name(name),
	m_fullPath(fullPath),
	m_rank(rank),
	m_item(std::move(item))
{
}

RecentTreeNode::Type RecentTreeNode::GetType() const
{
	return m_type;
}

const std::wstring &RecentTreeNode::GetName() const
{
	return m_name;
}

const std::wstring &RecentTreeNode::GetFullPath() const
{
	return m_fullPath;
}

int RecentTreeNode::GetRank() const
{
	return m_rank;
}

const RecentItem *RecentTreeNode::GetItem() const
{
	return m_item ? &*m_item : nullptr;
}

void RecentTreeNode::AddChild(std::unique_ptr<RecentTreeNode> node)
{
	m_children.push_back(std::move(node));
}

const std::vector<std::unique_ptr<RecentTreeNode>> &RecentTreeNode::GetChildren() const
{
	return m_children;
}
