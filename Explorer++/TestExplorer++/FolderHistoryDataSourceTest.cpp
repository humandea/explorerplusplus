// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/FolderHistoryDataSource.h"
#include "HistoryModel.h"
#include "PidlTestHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class FolderHistoryDataSourceTest : public Test
{
protected:
	HistoryModel m_historyModel;
	FolderHistoryDataSource m_dataSource{ &m_historyModel };
};

TEST_F(FolderHistoryDataSourceTest, ItemsReturnedInRecencyOrder)
{
	// Note that the paths below use uppercase drive letters, since that's how the paths are
	// normalized when retrieved from the pidls.
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder1"));
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder2"));
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder3"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 3u);
	EXPECT_EQ(items[0].GetPath(), L"C:\\folder3");
	EXPECT_EQ(items[0].GetRank(), 0);
	EXPECT_EQ(items[1].GetPath(), L"C:\\folder2");
	EXPECT_EQ(items[1].GetRank(), 1);
	EXPECT_EQ(items[2].GetPath(), L"C:\\folder1");
	EXPECT_EQ(items[2].GetRank(), 2);
}

TEST_F(FolderHistoryDataSourceTest, ItemsAreFolders)
{
	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder1"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].GetKind(), RecentItem::Kind::Folder);
	EXPECT_EQ(items[0].GetPidl(), CreateSimplePidlForTest(L"c:\\folder1"));
}

TEST_F(FolderHistoryDataSourceTest, ChangedObserverNotified)
{
	bool notified = false;
	m_dataSource.AddChangedObserver([&notified] { notified = true; });

	m_historyModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder1"));

	EXPECT_TRUE(notified);
}
