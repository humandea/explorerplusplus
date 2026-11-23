// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainToolbar.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "CoreInterface.h"
#include "MainToolbarView.h"
#include "ShellIconLoaderFake.h"

namespace
{

class CoreInterfaceFake : public CoreInterface
{
public:
	ShellBrowserImpl *GetActiveShellBrowserImpl() const override
	{
		return nullptr;
	}

	TabBar *GetTabBar() const override
	{
		return nullptr;
	}

	BOOL CanPaste(PasteType pasteType) const override
	{
		UNREFERENCED_PARAMETER(pasteType);

		return FALSE;
	}
};

}

class MainToolbarTest : public BrowserTestBase
{
protected:
	MainToolbarTest() : m_browser(AddBrowser())
	{
	}

	BrowserWindowFake *const m_browser;
	ShellIconLoaderFake m_shellIconLoader;
	CoreInterfaceFake m_coreInterface;
};

TEST_F(MainToolbarTest, GetLayoutForStorage)
{
	MainToolbarLayout initialLayout({
		MainToolbarButtonId::Bookmarks,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Search,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Back,
		MainToolbarButtonId::Forward,
		MainToolbarButtonId::Up,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Properties,
		MainToolbarButtonId::Refresh,
	});

	MainToolbarView mainToolbarView(m_browser->GetView(), m_browser, &m_config);
	MainToolbar mainToolbar(&mainToolbarView, initialLayout, m_browser, &m_appServices,
		&m_coreInterface, &m_shellIconLoader);

	auto layout = mainToolbar.GetLayoutForStorage();

	// The Recent button was added after toolbar layout persistence was introduced, so it should be
	// added to a saved layout that doesn't contain it. This layout doesn't contain the Folders
	// button, so the Recent button is added at the end.
	MainToolbarLayout expectedLayout = initialLayout;
	expectedLayout.AddButton(MainToolbarButtonId::Recent);

	EXPECT_EQ(layout.GetButtons(), expectedLayout.GetButtons());
}

TEST_F(MainToolbarTest, GetLayoutForStorageRecentButtonRetainsPosition)
{
	MainToolbarLayout initialLayout({
		MainToolbarButtonId::Back,
		MainToolbarButtonId::Recent,
		MainToolbarButtonId::Forward,
	});

	MainToolbarView mainToolbarView(m_browser->GetView(), m_browser, &m_config);
	MainToolbar mainToolbar(&mainToolbarView, initialLayout, m_browser, &m_appServices,
		&m_coreInterface, &m_shellIconLoader);

	// A saved layout that already contains the Recent button should round-trip unchanged.
	auto layout = mainToolbar.GetLayoutForStorage();
	EXPECT_EQ(layout.GetButtons(), initialLayout.GetButtons());
}

TEST_F(MainToolbarTest, TextButtonsSizedCorrectlyAfterInsertion)
{
	MainToolbarView mainToolbarView(m_browser->GetView(), m_browser, &m_config);
	MainToolbar mainToolbar(&mainToolbarView, std::nullopt, m_browser, &m_appServices,
		&m_coreInterface, &m_shellIconLoader);

	ToolbarButton *foldersButton = nullptr;
	ToolbarButton *backButton = nullptr;

	for (const auto &item : mainToolbarView.GetItems())
	{
		if (!item.IsButton())
		{
			continue;
		}

		auto *button = item.GetButton();

		if (button->GetText() == L"Folders")
		{
			foldersButton = button;
		}
		else if (button->GetText() == L"Back")
		{
			backButton = button;
		}
	}

	ASSERT_NE(foldersButton, nullptr);
	ASSERT_NE(backButton, nullptr);

	// Text buttons should be sized correctly as soon as they're inserted (previously, the width
	// only tightened after the button was updated).
	RECT foldersRectBefore = mainToolbarView.GetButtonRect(foldersButton);
	RECT backRect = mainToolbarView.GetButtonRect(backButton);

	mainToolbarView.UpdateButton(foldersButton);
	RECT foldersRectAfter = mainToolbarView.GetButtonRect(foldersButton);

	int foldersWidthBefore = foldersRectBefore.right - foldersRectBefore.left;
	int foldersWidthAfter = foldersRectAfter.right - foldersRectAfter.left;
	int backWidth = backRect.right - backRect.left;

	// The Back button is icon-only, so it provides a baseline for the fixed part of a button's
	// width (icon + padding).
	EXPECT_EQ(foldersWidthBefore, foldersWidthAfter);

	// The text should fit snugly: the width before the update shouldn't be any larger than the
	// width after it.
	EXPECT_LE(foldersWidthBefore, foldersWidthAfter);
}
