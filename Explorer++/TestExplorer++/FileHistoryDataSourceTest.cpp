// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/FileHistoryDataSource.h"
#include "HistoryModel.h"
#include "PidlTestHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class FileHistoryDataSourceTest : public Test
{
protected:
	HistoryModel m_historyModel;
	FileHistoryDataSource m_dataSource{ &m_historyModel };
};

TEST_F(FileHistoryDataSourceTest, ItemsReturnedInRecencyOrder)
{
	// Note that the paths below use uppercase drive letters, since that's how the paths are
	// normalized when retrieved from the pidls.
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file1.txt"));
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file2.txt"));
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file3.txt"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 3u);
	EXPECT_EQ(items[0].GetPath(), L"C:\\file3.txt");
	EXPECT_EQ(items[0].GetRank(), 0);
	EXPECT_EQ(items[1].GetPath(), L"C:\\file2.txt");
	EXPECT_EQ(items[1].GetRank(), 1);
	EXPECT_EQ(items[2].GetPath(), L"C:\\file1.txt");
	EXPECT_EQ(items[2].GetRank(), 2);
}

TEST_F(FileHistoryDataSourceTest, ItemsAreFiles)
{
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file1.txt"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].GetKind(), RecentItem::Kind::File);
	EXPECT_EQ(items[0].GetPidl(), CreateSimplePidlForTest(L"c:\\file1.txt"));
}

TEST_F(FileHistoryDataSourceTest, ChangedObserverNotified)
{
	bool notified = false;
	m_dataSource.AddChangedObserver([&notified] { notified = true; });

	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file1.txt"));

	EXPECT_TRUE(notified);
}
