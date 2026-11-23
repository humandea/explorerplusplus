// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentItemsPane.h"
#include "AppServices.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "CompositeRecentItemsDataSource.h"
#include "Config.h"
#include "FileFrequentLocationsDataSource.h"
#include "FileFrequentLocationsModel.h"
#include "FileHistoryDataSource.h"
#include "FileHistoryModel.h"
#include "FileSystemPathStatusChecker.h"
#include "FolderFrequentLocationsDataSource.h"
#include "FolderHistoryDataSource.h"
#include "LabelEditHandler.h"
#include "MainResource.h"
#include "NavigationHelper.h"
#include "PlatformContext.h"
#include "PopupMenuView.h"
#include "RecentItemsDataSource.h"
#include "RecentItemsIconManager.h"
#include "RecentItemsPaneView.h"
#include "RecentTreeBuilder.h"
#include "RecentTreeViewAdapter.h"
#include "RecentTreeViewNode.h"
#include "ResourceLoader.h"
#include "Runtime.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ThemeManager.h"
#include "../Helper/ClipboardStore.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ShellHelper.h"

RecentItemsPane::RecentItemsPane(RecentItemsPaneView *view, BrowserWindow *browser,
	AppServices *appServices, IconFetcher *iconFetcher,
	std::function<void(int newWidth)> resizedCallback) :
	m_view(view),
	m_browser(browser),
	m_appServices(appServices),
	m_iconFetcher(iconFetcher),
	m_mode(RecentItemsPaneMode::History),
	m_showFiles(true),
	m_treeView(CreateTreeView(view->GetHWND()),
		appServices->GetPlatformContext()->GetKeyboardState(), LabelEditHandler::CreateForDialog),
	m_delegate(this, appServices->GetResourceLoader()),
	m_fontSetter(m_treeView.GetHWND(), appServices->GetConfig())
{
	auto &dpiCompatibility = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompatibility.GetDpiForWindow(view->GetHWND());
	int iconWidth = dpiCompatibility.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompatibility.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	m_iconManager = std::make_unique<RecentItemsIconManager>(iconFetcher, iconWidth, iconHeight);
	m_treeView.SetImageList(m_iconManager->TakeImageList());

	m_treeView.AddExtendedStyles(TVS_EX_DOUBLEBUFFER);

	// The tree is always fully expanded, so nodes can't be collapsed by the user.
	m_treeView.SetNodeCollapsingDisabled(true);

	m_adapter = std::make_unique<RecentTreeViewAdapter>();

	m_toolbar = std::make_unique<RecentItemsPaneToolbar>(view->GetToolbarView(),
		view->GetFileToolbarView(), appServices->GetResourceLoader(), appServices->GetConfig(),
		m_mode, m_showFiles, std::bind_front(&RecentItemsPane::OnModeChanged, this),
		std::bind_front(&RecentItemsPane::OnShowFilesChanged, this));

	m_treeView.SetAdapter(m_adapter.get());
	m_treeView.SetDelegate(&m_delegate);

	m_view->SetTreeView(m_treeView.GetHWND());
	m_view->SetResizedCallback(std::move(resizedCallback));

	UpdateCaptionText();

	UpdateTreeColors();

	m_connections.push_back(m_appServices->GetConfig()->theme.addObserver(
		std::bind_front(&RecentItemsPane::OnThemeUpdated, this)));

	m_pathStatusChecker = std::make_unique<FileSystemPathStatusChecker>(view->GetHWND());
	m_connections.push_back(m_pathStatusChecker->AddStatusChangedObserver(
		std::bind_front(&RecentItemsPane::OnPathStatusesChanged, this)));

	UpdateDataSource();
	BuildTree(false);
}

RecentItemsPane::~RecentItemsPane() = default;

HWND RecentItemsPane::CreateTreeView(HWND parent)
{
	// The tree view is created directly here, rather than via the CreateTreeView() helper in
	// Helper\Controls.h. That helper sets the system image list on the tree view, while this pane
	// needs to set its own image list (see RecentItemsIconManager).
	//
	// The vertical scroll bar is placed on the left side of the tree view (WS_EX_LEFTSCROLLBAR),
	// so that it sits in between the resize divider at the left edge of the pane and the tree
	// items. That mirrors the folders pane, where the divider at the right edge of the pane sits
	// on the outer side of the tree view's scroll bar.
	// The tree is always fully expanded and nodes can't be collapsed, so no expander buttons are
	// shown (the hierarchy lines are kept).
	HWND hwnd = CreateWindowEx(WS_EX_LEFTSCROLLBAR, WC_TREEVIEW, L"",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_SHOWSELALWAYS | TVS_HASLINES | TVS_INFOTIP
			| TVS_DISABLEDRAGDROP,
		0, 0, 0, 0, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);

	return hwnd;
}

std::unique_ptr<RecentItemsDataSource> RecentItemsPane::CreateDataSourceForMode(
	RecentItemsPaneMode mode) const
{
	switch (mode)
	{
	case RecentItemsPaneMode::History:
	{
		std::vector<std::unique_ptr<RecentItemsDataSource>> sources;
		sources.push_back(
			std::make_unique<FolderHistoryDataSource>(m_appServices->GetHistoryModel()));
		sources.push_back(
			std::make_unique<FileHistoryDataSource>(m_appServices->GetFileHistoryModel()));
		return std::make_unique<CompositeRecentItemsDataSource>(std::move(sources));
	}

	case RecentItemsPaneMode::FrequentLocations:
	{
		std::vector<std::unique_ptr<RecentItemsDataSource>> sources;
		sources.push_back(std::make_unique<FolderFrequentLocationsDataSource>(
			m_appServices->GetFrequentLocationsModel()));
		sources.push_back(std::make_unique<FileFrequentLocationsDataSource>(
			m_appServices->GetFileFrequentLocationsModel()));
		return std::make_unique<CompositeRecentItemsDataSource>(std::move(sources));
	}

	default:
		LOG(FATAL) << "Recent items pane mode value not found";
		__assume(0);
	}
}

void RecentItemsPane::UpdateDataSource()
{
	// The connections to the previous data source are dropped here, since the previous source is
	// replaced below (and destroyed if nothing else references it).
	m_dataSourceConnections.clear();
	m_dataSource = CreateDataSourceForMode(m_mode);

	m_dataSourceConnections.push_back(m_dataSource->AddChangedObserver(
		std::bind_front(&RecentItemsPane::OnDataSourceChanged, this)));
}

void RecentItemsPane::BuildTree(bool force)
{
	// Rebuilding the tree can change the selection as a side effect, which shouldn't result in a
	// navigation.
	m_delegate.SetNavigationSuppressed(true);

	auto items = m_dataSource->GetItems();

	if (!m_showFiles)
	{
		std::erase_if(items,
			[](const RecentItem &item) { return item.GetKind() == RecentItem::Kind::File; });
	}

	// If the visible items haven't changed, the tree doesn't need to be rebuilt. Note that
	// status-driven rebuilds are always performed: deleted paths are excluded from the tree, but
	// they're still part of the item list, so an unchanged item list doesn't imply an unchanged
	// tree.
	if (!force && m_lastBuiltItems && RecentItemsListsEqual(items, *m_lastBuiltItems))
	{
		m_delegate.SetNavigationSuppressed(false);
		return;
	}

	// The selection is captured before the tree is rebuilt, so that it can be restored afterwards.
	auto selectedPath = MaybeGetSelectedNodePath();

	auto roots = RecentTreeBuilder::Build(items, m_pathStatusChecker.get());
	m_adapter->Update(roots, m_iconManager.get());

	// The tree is always fully expanded (node collapsing is disabled), so the expansion state
	// doesn't need to be preserved across rebuilds. Any nodes added as part of a rebuild are
	// therefore expanded as well, as long as they have children.
	ExpandAllNodes(m_adapter->GetRoot());

	if (selectedPath)
	{
		for (const auto &root : roots)
		{
			if (MaybeSelectNodeForPath(root.get(), *selectedPath))
			{
				break;
			}
		}
	}

	if (!m_treeView.MaybeGetSelectedNode())
	{
		// Nothing is selected (e.g. because the previously selected node was removed). A
		// selection is restored here, since the tree is expected to always have a selection (the
		// first node added to an empty tree is selected automatically).
		const auto &rootChildren = m_adapter->GetRoot()->GetChildren();

		if (!rootChildren.empty())
		{
			m_treeView.SelectNode(rootChildren.front().get());
		}
	}

	m_delegate.SetNavigationSuppressed(false);

	m_lastBuiltItems = std::move(items);
}

void RecentItemsPane::ExpandAllNodes(TreeViewNode *node)
{
	for (const auto &child : node->GetChildren())
	{
		m_treeView.ExpandNode(child.get());
		ExpandAllNodes(child.get());
	}
}

std::optional<std::wstring> RecentItemsPane::MaybeGetSelectedNodePath() const
{
	const auto *node = m_treeView.MaybeGetSelectedNode();

	if (!node)
	{
		return std::nullopt;
	}

	return static_cast<const RecentTreeViewNode *>(node)->GetFullPath();
}

// Attempts to select the node with the specified path, searching the tree recursively. Returns
// true (stopping the search) once the node has been selected.
bool RecentItemsPane::MaybeSelectNodeForPath(const RecentTreeNode *recentTreeNode,
	const std::wstring &selectedPath)
{
	const auto &path = recentTreeNode->GetFullPath();

	if (_wcsicmp(path.c_str(), selectedPath.c_str()) == 0)
	{
		auto *viewNode = m_adapter->MaybeGetNodeForRecentTreeNode(recentTreeNode);
		CHECK(viewNode);
		m_treeView.SelectNode(viewNode);
		return true;
	}

	for (const auto &child : recentTreeNode->GetChildren())
	{
		if (MaybeSelectNodeForPath(child.get(), selectedPath))
		{
			return true;
		}
	}

	return false;
}

void RecentItemsPane::UpdateTreeColors()
{
	auto *colorProvider = m_appServices->GetThemeManager()->GetCurrentColorProvider();
	TreeView_SetBkColor(m_treeView.GetHWND(), colorProvider->GetBackgroundColor());
	TreeView_SetTextColor(m_treeView.GetHWND(), colorProvider->GetTextColor());
}

void RecentItemsPane::UpdateCaptionText()
{
	auto *resourceLoader = m_appServices->GetResourceLoader();
	auto baseText = resourceLoader->LoadString(IDS_RECENT_ITEMS_WINDOW_TEXT);
	auto modeText = resourceLoader->LoadString(m_mode == RecentItemsPaneMode::History
			? IDS_RECENT_ITEMS_PANE_HISTORY
			: IDS_RECENT_ITEMS_PANE_FREQUENT_LOCATIONS);

	m_view->SetCaptionText(baseText + L" - " + modeText);
}

void RecentItemsPane::OnThemeUpdated(Theme newTheme)
{
	UNREFERENCED_PARAMETER(newTheme);

	UpdateTreeColors();
}

void RecentItemsPane::OnModeChanged(RecentItemsPaneMode mode)
{
	m_mode = mode;

	UpdateCaptionText();

	// The tree is rebuilt from the data source associated with the new mode.
	UpdateDataSource();
	ScheduleRebuild(true);
}

void RecentItemsPane::OnDataSourceChanged()
{
	ScheduleRebuild(false);
}

void RecentItemsPane::OnPathStatusesChanged()
{
	// The tree is rebuilt so that items whose paths have been confirmed to be deleted are removed
	// (and items that have come back are added back). Deleted paths are excluded from the tree
	// but not from the item list, so this rebuild is always performed, even when the item list
	// itself is unchanged.
	ScheduleRebuild(true);
}

void RecentItemsPane::OnShowFilesChanged(bool showFiles)
{
	m_showFiles = showFiles;

	ScheduleRebuild(false);
}

// Schedules a rebuild of the tree. Multiple requests are coalesced into a single rebuild, which
// runs once the UI thread is idle (e.g. when tabs are restored, multiple navigation events can
// fire in quick succession).
void RecentItemsPane::ScheduleRebuild(bool force)
{
	m_scheduledRebuildForce = m_scheduledRebuildForce || force;

	if (m_rebuildScheduled)
	{
		return;
	}

	m_rebuildScheduled = true;
	ScheduleRebuildTask(m_weakPtrFactory.GetWeakPtr(), m_appServices->GetRuntime());
}

concurrencpp::null_result RecentItemsPane::ScheduleRebuildTask(WeakPtr<RecentItemsPane> self,
	Runtime *runtime)
{
	// This function is designed to be called from the UI thread and the call here will also
	// resume on the UI thread. Rather than immediately resuming, however, this call will result
	// in a message being posted. Therefore, this function will only resume once the message has
	// been processed (at which point any outstanding change notifications will have been
	// processed as well).
	co_await concurrencpp::resume_on(runtime->GetUiThreadExecutor());

	if (!self)
	{
		co_return;
	}

	self->m_rebuildScheduled = false;
	bool force = self->m_scheduledRebuildForce;
	self->m_scheduledRebuildForce = false;
	self->BuildTree(force);
}

bool RecentItemsPane::RecentItemsListsEqual(const std::vector<RecentItem> &first,
	const std::vector<RecentItem> &second) const
{
	if (first.size() != second.size())
	{
		return false;
	}

	// The lists are in rank order, so items at the same position should have the same path and
	// kind (the rank itself is implied by the position).
	for (size_t i = 0; i < first.size(); i++)
	{
		if (_wcsicmp(first[i].GetPath().c_str(), second[i].GetPath().c_str()) != 0)
		{
			return false;
		}

		if (first[i].GetKind() != second[i].GetKind())
		{
			return false;
		}
	}

	return true;
}

void RecentItemsPane::NavigateInActiveTab(const RecentTreeViewNode *node)
{
	auto pidl = MaybeGetPidlForNode(node);

	if (!pidl)
	{
		return;
	}

	auto *shellBrowser =
		m_browser->GetActivePane()->GetTabBar()->GetSelectedTab().GetShellBrowserImpl();

	if (ArePidlsEquivalent(pidl->Raw(), shellBrowser->GetDirectory().Raw()))
	{
		return;
	}

	auto navigateParams = NavigateParams::Normal(pidl->Raw());
	shellBrowser->GetNavigationController()->Navigate(navigateParams);
}

void RecentItemsPane::OpenLocation(const RecentTreeViewNode *node, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	auto pidl = MaybeGetPidlForNode(node);

	if (!pidl)
	{
		return;
	}

	m_browser->OpenItem(pidl->Raw(), DetermineOpenDisposition(true, isCtrlKeyDown, isShiftKeyDown));
}

void RecentItemsPane::ShowContextMenu(const RecentTreeViewNode *node,
	const std::vector<RecentItemsContextMenu::Action> &actions, const POINT &ptScreen)
{
	UNREFERENCED_PARAMETER(node);

	PopupMenuView popupMenu(m_browser->GetHWND(), m_browser);
	RecentItemsContextMenu menu(&popupMenu, m_appServices->GetAcceleratorManager(), actions);
	popupMenu.Show(ptScreen);
}

ClipboardStore *RecentItemsPane::GetClipboardStore()
{
	return m_appServices->GetPlatformContext()->GetClipboardStore();
}

std::optional<PidlAbsolute> RecentItemsPane::MaybeGetPidlForNode(
	const RecentTreeViewNode *node) const
{
	if (node->GetItem() && node->GetItem()->GetPidl())
	{
		return *node->GetItem()->GetPidl();
	}

	// Items supplied by the debug data source (and chain nodes) don't have a pidl, so one is
	// derived from the path. This can fail if the path can't be resolved.
	unique_pidl_absolute pidl;
	HRESULT hr = ParseDisplayNameForNavigation(node->GetFullPath(), pidl);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return PidlAbsolute(pidl.get());
}
