// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItemsContextMenu.h"
#include "TreeViewDelegate.h"
#include <vector>

class ClipboardStore;
class RecentTreeViewNode;
class ResourceLoader;

// Provides the system-level operations the recent items tree needs. This interface exists so
// that the behavior of the tree can be tested without a real browser window.
class RecentItemsHost
{
public:
	virtual ~RecentItemsHost() = default;

	// Navigates the active tab to the specified node.
	virtual void NavigateInActiveTab(const RecentTreeViewNode *node) = 0;

	// Opens the specified node, respecting the modifier keys that were held down.
	virtual void OpenLocation(const RecentTreeViewNode *node, bool isCtrlKeyDown,
		bool isShiftKeyDown) = 0;

	// Shows the context menu for the specified node, with the given actions.
	virtual void ShowContextMenu(const RecentTreeViewNode *node,
		const std::vector<RecentItemsContextMenu::Action> &actions, const POINT &ptScreen) = 0;

	virtual ClipboardStore *GetClipboardStore() = 0;
};

// Handles the events raised by a TreeView instance that shows recent items.
class RecentItemsTreeDelegate : public TreeViewDelegate
{
public:
	RecentItemsTreeDelegate(RecentItemsHost *host, const ResourceLoader *resourceLoader);

	void OnSelectionChanged(TreeViewNode *selectedNode) override;
	void OnNodeMiddleClicked(TreeViewNode *targetNode, const MouseEvent &event) override;
	void OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen) override;

	// While navigation is suppressed, selection changes won't result in a navigation. This is
	// used when the tree is rebuilt, since rebuilding the tree can change the selection as a
	// side effect.
	void SetNavigationSuppressed(bool suppressed);

	// Builds the set of actions that can be shown in the context menu for a node. The actions are
	// returned separately from the menu itself, so that they can be tested without having to
	// show a menu.
	std::vector<RecentItemsContextMenu::Action> GetContextMenuActions(
		const RecentTreeViewNode *node) const;

private:
	static const RecentTreeViewNode *GetRecentNode(const TreeViewNode *node);

	void CopyNodePath(const RecentTreeViewNode *node) const;

	RecentItemsHost *const m_host;
	const ResourceLoader *const m_resourceLoader;
	bool m_navigationSuppressed = false;
};
