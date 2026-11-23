// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentItemsPaneToolbar.h"
#include "Config.h"
#include "MainResource.h"
#include "RecentItemsPaneToolbarView.h"
#include "ResourceHelper.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"
#include <wil/resource.h>

namespace
{

// These match the image sizes used by the main toolbar, so that the icons in this toolbar follow
// the same size as the main toolbar icons (which is controlled by the "Use large toolbar icons"
// setting).
constexpr int TOOLBAR_IMAGE_SIZE_SMALL = 16;
constexpr int TOOLBAR_IMAGE_SIZE_LARGE = 24;

// Returns a grayscale version of the specified bitmap, with the alpha channel preserved. Used to
// draw the file toggle button in a muted state when it's not checked.
wil::unique_hbitmap CreateGrayscaleBitmap(HBITMAP source)
{
	BITMAP bitmap = {};
	if (GetObject(source, sizeof(bitmap), &bitmap) == 0)
	{
		return nullptr;
	}

	int width = bitmap.bmWidth;
	int height = abs(bitmap.bmHeight);

	BITMAPINFO info = {};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;

	std::vector<BYTE> pixels(static_cast<size_t>(width) * height * 4);
	HDC dc = GetDC(nullptr);
	if (!GetDIBits(dc, source, 0, height, pixels.data(), &info, DIB_RGB_COLORS))
	{
		ReleaseDC(nullptr, dc);
		return nullptr;
	}
	ReleaseDC(nullptr, dc);

	for (size_t i = 0; (i + 3) < pixels.size(); i += 4)
	{
		BYTE gray =
			static_cast<BYTE>((pixels[i + 2] * 299 + pixels[i + 1] * 587 + pixels[i] * 114) / 1000);
		pixels[i] = gray;
		pixels[i + 1] = gray;
		pixels[i + 2] = gray;
	}

	void *bits = nullptr;
	wil::unique_hbitmap grayBitmap(
		CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0));

	if (!grayBitmap || !bits)
	{
		return nullptr;
	}

	memcpy(bits, pixels.data(), pixels.size());

	return grayBitmap;
}

}

RecentItemsPaneToolbar::ModeButton::ModeButton(RecentItemsPaneToolbar *toolbar,
	RecentItemsPaneMode mode, const std::wstring &text, int imageIndex) :
	ToolbarButton(std::bind(&ModeButton::OnClicked, this)),
	m_toolbar(toolbar),
	m_mode(mode),
	m_text(text),
	m_imageIndex(imageIndex)
{
}

std::wstring RecentItemsPaneToolbar::ModeButton::GetText() const
{
	return m_text;
}

std::optional<int> RecentItemsPaneToolbar::ModeButton::GetImageIndex() const
{
	return m_imageIndex;
}

bool RecentItemsPaneToolbar::ModeButton::ShouldShowText() const
{
	// These buttons are icon-only.
	return false;
}

bool RecentItemsPaneToolbar::ModeButton::IsChecked() const
{
	return m_toolbar->GetMode() == m_mode;
}

void RecentItemsPaneToolbar::ModeButton::NotifyUpdated()
{
	NotifyParentOfUpdate();
}

void RecentItemsPaneToolbar::ModeButton::OnClicked()
{
	m_toolbar->OnModeButtonClicked(m_mode);
}

RecentItemsPaneToolbar::FileToggleButton::FileToggleButton(RecentItemsPaneToolbar *toolbar,
	const std::wstring &text, int colorImageIndex, int grayscaleImageIndex, bool initialShowFiles) :
	ToolbarButton(std::bind(&FileToggleButton::OnClicked, this)),
	m_toolbar(toolbar),
	m_text(text),
	m_colorImageIndex(colorImageIndex),
	m_grayscaleImageIndex(grayscaleImageIndex),
	m_checked(initialShowFiles)
{
}

std::wstring RecentItemsPaneToolbar::FileToggleButton::GetText() const
{
	return m_text;
}

std::optional<int> RecentItemsPaneToolbar::FileToggleButton::GetImageIndex() const
{
	// The button is drawn in gray when unchecked, to indicate that file entries aren't currently
	// shown.
	return m_checked ? m_colorImageIndex : m_grayscaleImageIndex;
}

bool RecentItemsPaneToolbar::FileToggleButton::ShouldShowText() const
{
	// This button is icon-only.
	return false;
}

bool RecentItemsPaneToolbar::FileToggleButton::IsChecked() const
{
	return m_checked;
}

void RecentItemsPaneToolbar::FileToggleButton::NotifyUpdated()
{
	NotifyParentOfUpdate();
}

void RecentItemsPaneToolbar::FileToggleButton::OnClicked()
{
	m_checked = !m_checked;
	NotifyParentOfUpdate();
	m_toolbar->OnFileToggleButtonClicked(m_checked);
}

RecentItemsPaneToolbar::RecentItemsPaneToolbar(RecentItemsPaneToolbarView *toolbarView,
	RecentItemsPaneToolbarView *fileToolbarView, const ResourceLoader *resourceLoader,
	const Config *config, RecentItemsPaneMode initialMode, bool initialShowFiles,
	ModeChangedCallback modeChangedCallback, ShowFilesChangedCallback showFilesChangedCallback) :
	m_toolbarView(toolbarView),
	m_fileToolbarView(fileToolbarView),
	m_config(config),
	m_mode(initialMode),
	m_showFiles(initialShowFiles),
	m_modeChangedCallback(modeChangedCallback),
	m_showFilesChangedCallback(showFilesChangedCallback)
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(toolbarView->GetHWND());

	CreateImageLists(resourceLoader, dpi);
	SetToolbarImageList();

	m_connections.push_back(m_config->useLargeToolbarIcons.addObserver(
		std::bind_front(&RecentItemsPaneToolbar::OnUseLargeToolbarIconsUpdated, this)));

	auto historyButton = std::make_unique<ModeButton>(this, RecentItemsPaneMode::History,
		resourceLoader->LoadString(IDS_RECENT_ITEMS_PANE_HISTORY),
		m_imageListMappings.at(Icon::History));
	m_historyButton = historyButton.get();
	toolbarView->AddButton(std::move(historyButton), 0);

	auto frequentLocationsButton =
		std::make_unique<ModeButton>(this, RecentItemsPaneMode::FrequentLocations,
			resourceLoader->LoadString(IDS_RECENT_ITEMS_PANE_FREQUENT_LOCATIONS),
			m_imageListMappings.at(Icon::Frequent));
	m_frequentLocationsButton = frequentLocationsButton.get();
	toolbarView->AddButton(std::move(frequentLocationsButton), 1);

	auto fileToggleButton = std::make_unique<FileToggleButton>(this,
		resourceLoader->LoadString(IDS_RECENT_ITEMS_PANE_FILES), m_imageListMappings.at(Icon::File),
		m_fileGrayscaleImageIndex, m_showFiles);
	m_fileToggleButton = fileToggleButton.get();
	fileToolbarView->AddButton(std::move(fileToggleButton), 0);
}

void RecentItemsPaneToolbar::CreateImageLists(const ResourceLoader *resourceLoader, UINT dpi)
{
	int dpiScaledSizeSmall = MulDiv(TOOLBAR_IMAGE_SIZE_SMALL, dpi, USER_DEFAULT_SCREEN_DPI);
	int dpiScaledSizeLarge = MulDiv(TOOLBAR_IMAGE_SIZE_LARGE, dpi, USER_DEFAULT_SCREEN_DPI);

	std::tie(m_imageListSmall, m_imageListMappings) =
		ResourceHelper::CreateIconImageList(resourceLoader, dpiScaledSizeSmall, dpiScaledSizeSmall,
			{ Icon::History, Icon::Frequent, Icon::File });

	IconImageListMapping largeImageListMappings;
	std::tie(m_imageListLarge, largeImageListMappings) =
		ResourceHelper::CreateIconImageList(resourceLoader, dpiScaledSizeLarge, dpiScaledSizeLarge,
			{ Icon::History, Icon::Frequent, Icon::File });

	// When the config option to use large toolbar icons is toggled, the image list assigned to
	// the toolbar controls will be changed. The image indexes for the buttons won't be updated,
	// so it's important that the image indexes are the same for both the small/large image lists.
	// That should be the case, since the image lists are created with the same set of images.
	CHECK_EQ(m_imageListMappings.at(Icon::History), largeImageListMappings.at(Icon::History));
	CHECK_EQ(m_imageListMappings.at(Icon::Frequent), largeImageListMappings.at(Icon::Frequent));
	CHECK_EQ(m_imageListMappings.at(Icon::File), largeImageListMappings.at(Icon::File));

	// A grayscale copy of the file icon is added as well; it's shown when the file toggle is
	// unchecked (i.e. when file entries aren't currently displayed). The index of the grayscale
	// image needs to be the same across both image lists, for the same reason as above.
	wil::unique_hbitmap fileSmallBitmap = resourceLoader->LoadBitmapFromPNGAndScale(Icon::File,
		dpiScaledSizeSmall, dpiScaledSizeSmall);
	wil::unique_hbitmap fileSmallGrayscaleBitmap = CreateGrayscaleBitmap(fileSmallBitmap.get());
	int fileGrayscaleImageIndex =
		ImageList_Add(m_imageListSmall.get(), fileSmallGrayscaleBitmap.get(), nullptr);
	CHECK_NE(fileGrayscaleImageIndex, -1);

	wil::unique_hbitmap fileLargeBitmap = resourceLoader->LoadBitmapFromPNGAndScale(Icon::File,
		dpiScaledSizeLarge, dpiScaledSizeLarge);
	wil::unique_hbitmap fileLargeGrayscaleBitmap = CreateGrayscaleBitmap(fileLargeBitmap.get());
	int fileLargeGrayscaleImageIndex =
		ImageList_Add(m_imageListLarge.get(), fileLargeGrayscaleBitmap.get(), nullptr);
	CHECK_NE(fileLargeGrayscaleImageIndex, -1);
	CHECK_EQ(fileLargeGrayscaleImageIndex, fileGrayscaleImageIndex);

	m_fileGrayscaleImageIndex = fileGrayscaleImageIndex;
}

void RecentItemsPaneToolbar::SetToolbarImageList()
{
	HIMAGELIST imageList;

	if (m_config->useLargeToolbarIcons.get())
	{
		imageList = m_imageListLarge.get();
	}
	else
	{
		imageList = m_imageListSmall.get();
	}

	m_toolbarView->SetImageList(imageList);
	m_fileToolbarView->SetImageList(imageList);
}

void RecentItemsPaneToolbar::OnUseLargeToolbarIconsUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	SetToolbarImageList();
}

RecentItemsPaneMode RecentItemsPaneToolbar::GetMode() const
{
	return m_mode;
}

bool RecentItemsPaneToolbar::GetShowFiles() const
{
	return m_showFiles;
}

void RecentItemsPaneToolbar::OnModeButtonClicked(RecentItemsPaneMode mode)
{
	if (mode == m_mode)
	{
		return;
	}

	SetMode(mode);
}

void RecentItemsPaneToolbar::SetMode(RecentItemsPaneMode mode)
{
	m_mode = mode;

	// The checked state of the buttons is based on the current mode, so both buttons need to be
	// updated.
	m_historyButton->NotifyUpdated();
	m_frequentLocationsButton->NotifyUpdated();

	if (m_modeChangedCallback)
	{
		m_modeChangedCallback(m_mode);
	}
}

void RecentItemsPaneToolbar::OnFileToggleButtonClicked(bool showFiles)
{
	SetShowFiles(showFiles);
}

void RecentItemsPaneToolbar::SetShowFiles(bool showFiles)
{
	m_showFiles = showFiles;

	m_fileToggleButton->NotifyUpdated();

	if (m_showFilesChangedCallback)
	{
		m_showFilesChangedCallback(m_showFiles);
	}
}
