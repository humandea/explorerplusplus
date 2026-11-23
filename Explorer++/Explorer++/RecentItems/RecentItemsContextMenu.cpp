// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentItemsContextMenu.h"
#include "MenuItemParams.h"
#include "MenuView.h"

RecentItemsContextMenu::RecentItemsContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, const std::vector<Action> &actions) :
	MenuBase(menuView, acceleratorManager)
{
	BuildMenu(actions);
}

void RecentItemsContextMenu::BuildMenu(const std::vector<Action> &actions)
{
	for (const auto &action : actions)
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(action.text).SetSelectedCallback(action.callback));
	}
}
