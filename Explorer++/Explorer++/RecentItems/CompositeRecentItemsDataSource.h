// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItemsDataSource.h"
#include <boost/signals2.hpp>
#include <memory>
#include <vector>

// Combines multiple data sources into a single one. Used to merge the folder and file sources
// for a particular mode (e.g. history and file history).
//
// The items from each source are concatenated, in the order in which the sources appear, and
// ranked sequentially. That means that among the children of any folder, folders (which come from
// an earlier source) will sort above files, with each group in the order in which it's stored by
// its source.
//
// A limited number of items is taken from each source (MAX_ITEMS_PER_SOURCE), to keep the tree
// manageable. The limit is applied per source, rather than to the merged list, so that items from
// a later source (i.e. files) aren't crowded out by items from an earlier one.
class CompositeRecentItemsDataSource : public RecentItemsDataSource
{
public:
	explicit CompositeRecentItemsDataSource(
		std::vector<std::unique_ptr<RecentItemsDataSource>> sources);

	std::vector<RecentItem> GetItems() const override;
	boost::signals2::connection AddChangedObserver(
		const ChangedSignal::slot_type &observer) override;

private:
	// The maximum number of items taken from each source.
	static constexpr int MAX_ITEMS_PER_SOURCE = 20;

	std::vector<std::unique_ptr<RecentItemsDataSource>> m_sources;
	ChangedSignal m_changedSignal;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
