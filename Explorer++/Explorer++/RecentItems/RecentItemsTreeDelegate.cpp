// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentItemsTreeDelegate.h"
#include "MainResource.h"
#include "MouseEvent.h"
#include "RecentTreeViewNode.h"
#include "ResourceLoader.h"
#include "../Helper/Clipboard.h"
#include "../Helper/ClipboardStore.h"

RecentItemsTreeDelegate::RecentItemsTreeDelegate(RecentItemsHost *host,
	const ResourceLoader *resourceLoader) :
	m_host(host),
	m_resourceLoader(resourceLoader)
{
}

void RecentItemsTreeDelegate::OnSelectionChanged(TreeViewNode *selectedNode)
{
	if (m_navigationSuppressed)
	{
		return;
	}

	m_host->NavigateInActiveTab(GetRecentNode(selectedNode));
}

void RecentItemsTreeDelegate::SetNavigationSuppressed(bool suppressed)
{
	m_navigationSuppressed = suppressed;
}

void RecentItemsTreeDelegate::OnNodeMiddleClicked(TreeViewNode *targetNode, const MouseEvent &event)
{
	m_host->OpenLocation(GetRecentNode(targetNode), event.ctrlKey, event.shiftKey);
}

void RecentItemsTreeDelegate::OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen)
{
	auto *recentNode = GetRecentNode(targetNode);
	m_host->ShowContextMenu(recentNode, GetContextMenuActions(recentNode), ptScreen);
}

std::vector<RecentItemsContextMenu::Action> RecentItemsTreeDelegate::GetContextMenuActions(
	const RecentTreeViewNode *node) const
{
	std::vector<RecentItemsContextMenu::Action> actions;

	actions.push_back({ m_resourceLoader->LoadString(IDS_RECENT_ITEMS_CONTEXT_MENU_COPY_PATH),
		[this, node] { CopyNodePath(node); } });

	return actions;
}

void RecentItemsTreeDelegate::CopyNodePath(const RecentTreeViewNode *node) const
{
	Clipboard clipboard(m_host->GetClipboardStore());
	clipboard.WriteText(node->GetFullPath());
}

const RecentTreeViewNode *RecentItemsTreeDelegate::GetRecentNode(const TreeViewNode *node)
{
	// All nodes in the recent items tree are RecentTreeViewNode instances (the root node is never
	// passed here).
	return static_cast<const RecentTreeViewNode *>(node);
}
