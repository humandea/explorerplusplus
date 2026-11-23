// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItemsDataSource.h"

class FrequentLocationsModel;

// Provides the set of folders that have been visited in Explorer++, with more frequently visited
// folders appearing first.
class FolderFrequentLocationsDataSource : public RecentItemsDataSource
{
public:
	explicit FolderFrequentLocationsDataSource(FrequentLocationsModel *frequentLocationsModel);

	std::vector<RecentItem> GetItems() const override;
	boost::signals2::connection AddChangedObserver(
		const ChangedSignal::slot_type &observer) override;

private:
	FrequentLocationsModel *const m_frequentLocationsModel;
};
