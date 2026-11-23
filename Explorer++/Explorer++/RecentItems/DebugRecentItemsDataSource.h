// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PathStatusChecker.h"
#include "RecentItemsDataSource.h"

#ifdef _DEBUG

// Debug-only data source that supplies a fixed set of items, used to exercise the recent items
// tree without having to navigate around first. In particular, the items cover cases that don't
// currently occur in practice (such as files, which aren't tracked yet).
//
// This class isn't included in release builds.
class DebugRecentItemsDataSource : public RecentItemsDataSource
{
public:
	std::vector<RecentItem> GetItems() const override;
	boost::signals2::connection AddChangedObserver(
		const ChangedSignal::slot_type &observer) override;
};

// Debug-only path status checker that works alongside DebugRecentItemsDataSource. Most paths are
// reported as reachable, with a couple of paths designated as unreachable/deleted, so that the
// handling of those cases can be verified.
//
// This class isn't included in release builds.
class DebugPathStatusChecker : public PathStatusChecker
{
public:
	PathStatus GetStatus(const std::wstring &path) const override;
};

#endif
