// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItemsPaneMode.h"
#include "ResourceHelper.h"
#include "ToolbarViewBase.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <functional>
#include <vector>

struct Config;
class RecentItemsPaneToolbarView;
class ResourceLoader;
class ToolbarButton;

// Manages the toolbars shown at the top of the recent items pane. The mode toggle (two buttons,
// one for each mode, with the active mode shown as checked) is on the left toolbar. The file
// toggle is on a separate right-aligned toolbar (part of a section for additional icons that
// appear on the right). The file button is drawn in gray when unchecked, to indicate that file
// entries aren't currently shown.
class RecentItemsPaneToolbar
{
public:
	using ModeChangedCallback = std::function<void(RecentItemsPaneMode mode)>;
	using ShowFilesChangedCallback = std::function<void(bool showFiles)>;

	RecentItemsPaneToolbar(RecentItemsPaneToolbarView *toolbarView,
		RecentItemsPaneToolbarView *fileToolbarView, const ResourceLoader *resourceLoader,
		const Config *config, RecentItemsPaneMode initialMode, bool initialShowFiles,
		ModeChangedCallback modeChangedCallback, ShowFilesChangedCallback showFilesChangedCallback);

	RecentItemsPaneMode GetMode() const;
	bool GetShowFiles() const;

private:
	class ModeButton : public ToolbarButton
	{
	public:
		ModeButton(RecentItemsPaneToolbar *toolbar, RecentItemsPaneMode mode,
			const std::wstring &text, int imageIndex);

		std::wstring GetText() const override;
		std::optional<int> GetImageIndex() const override;
		bool ShouldShowText() const override;
		bool IsChecked() const override;

		// Updates the appearance of the button (e.g. when the checked state changes).
		void NotifyUpdated();

	private:
		void OnClicked();

		RecentItemsPaneToolbar *const m_toolbar;
		const RecentItemsPaneMode m_mode;
		const std::wstring m_text;
		const int m_imageIndex;
	};

	class FileToggleButton : public ToolbarButton
	{
	public:
		FileToggleButton(RecentItemsPaneToolbar *toolbar, const std::wstring &text,
			int colorImageIndex, int grayscaleImageIndex, bool initialShowFiles);

		std::wstring GetText() const override;
		std::optional<int> GetImageIndex() const override;
		bool ShouldShowText() const override;
		bool IsChecked() const override;

		// Updates the appearance of the button (e.g. when the checked state changes).
		void NotifyUpdated();

	private:
		void OnClicked();

		RecentItemsPaneToolbar *const m_toolbar;
		const std::wstring m_text;
		const int m_colorImageIndex;
		const int m_grayscaleImageIndex;
		bool m_checked;
	};

	void SetMode(RecentItemsPaneMode mode);
	void OnModeButtonClicked(RecentItemsPaneMode mode);
	void OnFileToggleButtonClicked(bool showFiles);
	void SetShowFiles(bool showFiles);
	void CreateImageLists(const ResourceLoader *resourceLoader, UINT dpi);
	void SetToolbarImageList();
	void OnUseLargeToolbarIconsUpdated(BOOL newValue);

	RecentItemsPaneToolbarView *m_toolbarView = nullptr;
	RecentItemsPaneToolbarView *m_fileToolbarView = nullptr;
	const Config *m_config = nullptr;
	RecentItemsPaneMode m_mode;
	bool m_showFiles;
	ModeChangedCallback m_modeChangedCallback;
	ShowFilesChangedCallback m_showFilesChangedCallback;
	ModeButton *m_historyButton = nullptr;
	ModeButton *m_frequentLocationsButton = nullptr;
	FileToggleButton *m_fileToggleButton = nullptr;
	int m_fileGrayscaleImageIndex = -1;
	wil::unique_himagelist m_imageListSmall;
	wil::unique_himagelist m_imageListLarge;
	IconImageListMapping m_imageListMappings;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
