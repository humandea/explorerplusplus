// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileFrequentLocationsDataSource.h"
#include "FileFrequentLocationsModel.h"
#include "../Helper/ShellHelper.h"

FileFrequentLocationsDataSource::FileFrequentLocationsDataSource(
	FileFrequentLocationsModel *frequentLocationsModel) :
	m_frequentLocationsModel(frequentLocationsModel)
{
}

std::vector<RecentItem> FileFrequentLocationsDataSource::GetItems() const
{
	std::vector<RecentItem> items;
	int rank = 0;

	for (const auto &visit : m_frequentLocationsModel->GetVisits())
	{
		items.emplace_back(GetDisplayNameWithFallback(visit.GetLocation().Raw(), SHGDN_FORPARSING),
			RecentItem::Kind::File, rank++, visit.GetLocation());
	}

	return items;
}

boost::signals2::connection FileFrequentLocationsDataSource::AddChangedObserver(
	const ChangedSignal::slot_type &observer)
{
	return m_frequentLocationsModel->AddLocationsChangedObserver(observer);
}
