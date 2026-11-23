// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "RecentItem.h"
#include "RecentItemsPaneMode.h"
#include "RecentItemsPaneToolbar.h"
#include "RecentItemsTreeDelegate.h"
#include "Theme.h"
#include "TreeView.h"
#include "../Helper/Pidl.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class AppServices;
class BrowserWindow;
class FileSystemPathStatusChecker;
class IconFetcher;
class RecentItemsDataSource;
class RecentItemsIconManager;
class RecentItemsPaneView;
class RecentTreeViewAdapter;
class RecentTreeNode;
class Runtime;

// Manages the recent items pane: the tree, the mode toggle and the interaction with the rest of
// the application.
class RecentItemsPane : private RecentItemsHost
{
public:
	// resizedCallback is invoked when the pane is resized, with the new width of the pane.
	RecentItemsPane(RecentItemsPaneView *view, BrowserWindow *browser, AppServices *appServices,
		IconFetcher *iconFetcher, std::function<void(int newWidth)> resizedCallback);
	~RecentItemsPane();

private:
	static HWND CreateTreeView(HWND parent);

	// RecentItemsHost
	void NavigateInActiveTab(const RecentTreeViewNode *node) override;
	void OpenLocation(const RecentTreeViewNode *node, bool isCtrlKeyDown,
		bool isShiftKeyDown) override;
	void ShowContextMenu(const RecentTreeViewNode *node,
		const std::vector<RecentItemsContextMenu::Action> &actions, const POINT &ptScreen) override;
	ClipboardStore *GetClipboardStore() override;

	std::optional<PidlAbsolute> MaybeGetPidlForNode(const RecentTreeViewNode *node) const;

	std::unique_ptr<RecentItemsDataSource> CreateDataSourceForMode(RecentItemsPaneMode mode) const;
	void UpdateDataSource();

	void BuildTree(bool force);
	void ExpandAllNodes(TreeViewNode *node);
	std::optional<std::wstring> MaybeGetSelectedNodePath() const;
	bool MaybeSelectNodeForPath(const RecentTreeNode *recentTreeNode,
		const std::wstring &selectedPath);
	bool RecentItemsListsEqual(const std::vector<RecentItem> &first,
		const std::vector<RecentItem> &second) const;
	void UpdateTreeColors();
	void UpdateCaptionText();
	void OnThemeUpdated(Theme newTheme);
	void OnModeChanged(RecentItemsPaneMode mode);
	void OnDataSourceChanged();
	void OnPathStatusesChanged();
	void OnShowFilesChanged(bool showFiles);
	void ScheduleRebuild(bool force);
	static concurrencpp::null_result ScheduleRebuildTask(WeakPtr<RecentItemsPane> self,
		Runtime *runtime);

	RecentItemsPaneView *const m_view;
	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	IconFetcher *const m_iconFetcher;
	RecentItemsPaneMode m_mode;
	bool m_showFiles;

	std::unique_ptr<RecentItemsIconManager> m_iconManager;
	std::unique_ptr<RecentTreeViewAdapter> m_adapter;
	std::unique_ptr<RecentItemsPaneToolbar> m_toolbar;
	std::unique_ptr<RecentItemsDataSource> m_dataSource;
	std::unique_ptr<FileSystemPathStatusChecker> m_pathStatusChecker;
	TreeView m_treeView;
	RecentItemsTreeDelegate m_delegate;
	MainFontSetter m_fontSetter;

	std::vector<boost::signals2::scoped_connection> m_connections;
	std::vector<boost::signals2::scoped_connection> m_dataSourceConnections;

	// Indicates whether a rebuild has been scheduled but hasn't run yet (see ScheduleRebuild).
	bool m_rebuildScheduled = false;
	bool m_scheduledRebuildForce = false;

	// The items the tree was last built from. Used to detect updates that don't change the
	// visible items (in which case the tree doesn't need to be rebuilt).
	std::optional<std::vector<RecentItem>> m_lastBuiltItems;

	// Declared last, so that it's destroyed first (invalidating any weak pointers before the
	// other members are destroyed).
	WeakPtrFactory<RecentItemsPane> m_weakPtrFactory{ this };
};
