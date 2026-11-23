// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItemsDataSource.h"

class HistoryModel;

// Provides the set of folders that have been navigated to in Explorer++, with more recently
// visited folders appearing first.
class FolderHistoryDataSource : public RecentItemsDataSource
{
public:
	explicit FolderHistoryDataSource(HistoryModel *historyModel);

	std::vector<RecentItem> GetItems() const override;
	boost::signals2::connection AddChangedObserver(
		const ChangedSignal::slot_type &observer) override;

private:
	HistoryModel *const m_historyModel;
};
