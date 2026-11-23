// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CompositeRecentItemsDataSource.h"

CompositeRecentItemsDataSource::CompositeRecentItemsDataSource(
	std::vector<std::unique_ptr<RecentItemsDataSource>> sources) :
	m_sources(std::move(sources))
{
	for (const auto &source : m_sources)
	{
		m_connections.push_back(source->AddChangedObserver([this] { m_changedSignal(); }));
	}
}

std::vector<RecentItem> CompositeRecentItemsDataSource::GetItems() const
{
	std::vector<RecentItem> items;
	int rank = 0;

	for (const auto &source : m_sources)
	{
		auto sourceItems = source->GetItems();

		if (sourceItems.size() > MAX_ITEMS_PER_SOURCE)
		{
			sourceItems.erase(sourceItems.begin() + MAX_ITEMS_PER_SOURCE, sourceItems.end());
		}

		for (const auto &item : sourceItems)
		{
			items.emplace_back(item.GetPath(), item.GetKind(), rank++, item.GetPidl());
		}
	}

	return items;
}

boost::signals2::connection CompositeRecentItemsDataSource::AddChangedObserver(
	const ChangedSignal::slot_type &observer)
{
	return m_changedSignal.connect(observer);
}
