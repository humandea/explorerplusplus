// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileHistoryDataSource.h"
#include "HistoryModel.h"
#include "../Helper/ShellHelper.h"

FileHistoryDataSource::FileHistoryDataSource(HistoryModel *historyModel) :
	m_historyModel(historyModel)
{
}

std::vector<RecentItem> FileHistoryDataSource::GetItems() const
{
	std::vector<RecentItem> items;
	int rank = 0;

	for (const auto &pidl : m_historyModel->GetHistoryItems())
	{
		items.emplace_back(GetDisplayNameWithFallback(pidl.Raw(), SHGDN_FORPARSING),
			RecentItem::Kind::File, rank++, pidl);
	}

	return items;
}

boost::signals2::connection FileHistoryDataSource::AddChangedObserver(
	const ChangedSignal::slot_type &observer)
{
	return m_historyModel->AddHistoryChangedObserver(observer);
}
