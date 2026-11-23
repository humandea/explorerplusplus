// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItem.h"
#include <boost/signals2.hpp>
#include <vector>

// Provides the set of items displayed in the recent items tree. Implementations include the
// history model, the frequent locations model and a debug-only dummy source.
class RecentItemsDataSource
{
public:
	using ChangedSignal = boost::signals2::signal<void()>;

	virtual ~RecentItemsDataSource() = default;

	// Returns the full set of items, in rank order (i.e. the order in which they should be
	// displayed).
	virtual std::vector<RecentItem> GetItems() const = 0;

	virtual boost::signals2::connection AddChangedObserver(
		const ChangedSignal::slot_type &observer) = 0;
};
