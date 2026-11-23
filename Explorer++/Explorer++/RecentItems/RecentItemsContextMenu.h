// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <functional>
#include <vector>

class AcceleratorManager;

// The context menu for the recent items pane. The menu is built from a list of actions, so that
// entries can be added over time without restructuring the menu.
class RecentItemsContextMenu : public MenuBase
{
public:
	struct Action
	{
		std::wstring text;
		std::function<void()> callback;
	};

	RecentItemsContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const std::vector<Action> &actions);

private:
	void BuildMenu(const std::vector<Action> &actions);
};
