// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Literals.h"
#include "RecentItemsPaneToolbarView.h"
#include "ViewBase.h"
#include <wil/resource.h>
#include <functional>
#include <optional>

struct Config;
class MainFontSetter;
class MenuHelpTextHost;
class ResourceLoader;
class ThemeManager;

// The shell of the recent items pane. Contains a caption (with the mode toggle), along with the
// tree view. The pane is resized by dragging its left edge.
class RecentItemsPaneView : public ViewBase
{
public:
	using ResizedCallback = std::function<void(int newWidth)>;

	RecentItemsPaneView(ViewBase *parent, MenuHelpTextHost *menuHelpTextHost,
		const ThemeManager *themeManager, const Config *config,
		const ResourceLoader *resourceLoader);

	RecentItemsPaneToolbarView *GetToolbarView();
	RecentItemsPaneToolbarView *GetFileToolbarView();

	void SetTreeView(HWND treeView);
	void SetResizedCallback(ResizedCallback callback);
	void SetCaptionText(const std::wstring &text);

private:
	static constexpr WCHAR CLASS_NAME[] = L"Explorer++RecentItemsPaneWindowClass";

	static constexpr int CAPTION_SECTION_HORIZONTAL_PADDING = 4_px;
	static constexpr int CAPTION_SECTION_VERTICAL_PADDING = 1_px;

	// Padding added above and below the toolbar row (which contains the mode toggle).
	static constexpr int TOOLBAR_SECTION_VERTICAL_PADDING = 2_px;

	// The tree view is inset from the left edge of the pane, so that the pane itself receives the
	// mouse input for the resize behavior at the left edge (mirroring the right edge of the
	// folders pane).
	static constexpr int CONTENT_SECTION_LEFT_PADDING = 4_px;

	static constexpr int CONTENT_SECTION_RIGHT_PADDING = 4_px;

	// A resize can be started if the cursor is this many pixels from the left edge of the window.
	static constexpr int RESIZE_START_RANGE = 6_px;

	static HWND CreateRecentItemsPane(ViewBase *parent);
	static ATOM RegisterRecentItemsPaneClass();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnSetFont(HFONT font, bool redraw);
	void SetFont(HFONT font);
	void OnCaptionMetricsChanged();
	void OnPaint();
	void OnPrintClient(HDC hdc);
	void Paint(HDC hdc, const RECT &paintRect);
	void OnSize(int width, int height);
	void UpdateLayout();
	void UpdateLayout(int width, int height);
	int GetCaptionSectionHeight();
	int CalculateCaptionSectionHeight();
	void OnLButtonDown(const POINT &pt);
	int OnMouseMove(const POINT &pt);
	void OnCaptureChanged(HWND target);
	bool OnSetCursor(HWND target);
	bool IsCursorInResizeStartRange(const POINT &ptCursor);

	RecentItemsPaneToolbarView m_toolbarView;
	RecentItemsPaneToolbarView m_fileToolbarView;
	std::wstring m_captionText;
	const ThemeManager *const m_themeManager;
	HWND m_treeView = nullptr;
	HFONT m_font = nullptr;
	wil::unique_hfont m_defaultFont;
	std::unique_ptr<MainFontSetter> m_fontSetter;
	std::optional<int> m_captionSectionHeight;

	HCURSOR m_sizingCursor;
	bool m_resizing = false;
	ResizedCallback m_resizedCallback;
};
