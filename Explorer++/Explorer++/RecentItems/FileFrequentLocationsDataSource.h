// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItemsDataSource.h"

class FileFrequentLocationsModel;

// Provides the set of files that have been opened in Explorer++, with more frequently opened
// files appearing first.
class FileFrequentLocationsDataSource : public RecentItemsDataSource
{
public:
	explicit FileFrequentLocationsDataSource(FileFrequentLocationsModel *frequentLocationsModel);

	std::vector<RecentItem> GetItems() const override;
	boost::signals2::connection AddChangedObserver(
		const ChangedSignal::slot_type &observer) override;

private:
	FileFrequentLocationsModel *const m_frequentLocationsModel;
};
