// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItemsDataSource.h"

class HistoryModel;

// Provides the set of files that have been opened in Explorer++, with more recently opened files
// appearing first.
class FileHistoryDataSource : public RecentItemsDataSource
{
public:
	explicit FileHistoryDataSource(HistoryModel *historyModel);

	std::vector<RecentItem> GetItems() const override;
	boost::signals2::connection AddChangedObserver(
		const ChangedSignal::slot_type &observer) override;

private:
	HistoryModel *const m_historyModel;
};
