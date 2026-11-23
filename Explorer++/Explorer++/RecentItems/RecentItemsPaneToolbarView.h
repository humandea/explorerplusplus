// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarView.h"

class RecentItemsPaneToolbarView : public ToolbarView
{
public:
	RecentItemsPaneToolbarView(ViewBase *parent, MenuHelpTextHost *menuHelpTextHost,
		const Config *config);
};
