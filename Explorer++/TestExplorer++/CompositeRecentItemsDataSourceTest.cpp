// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/CompositeRecentItemsDataSource.h"
#include "HistoryModel.h"
#include "PidlTestHelper.h"
#include "RecentItems/FileHistoryDataSource.h"
#include "RecentItems/FolderHistoryDataSource.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{

std::vector<std::unique_ptr<RecentItemsDataSource>> CreateSources(HistoryModel *folderModel,
	HistoryModel *fileModel)
{
	std::vector<std::unique_ptr<RecentItemsDataSource>> sources;
	sources.push_back(std::make_unique<FolderHistoryDataSource>(folderModel));
	sources.push_back(std::make_unique<FileHistoryDataSource>(fileModel));
	return sources;
}

}

class CompositeRecentItemsDataSourceTest : public Test
{
protected:
	CompositeRecentItemsDataSourceTest() :
		m_compositeDataSource(CreateSources(&m_folderHistoryModel, &m_fileHistoryModel))
	{
	}

	HistoryModel m_folderHistoryModel;
	HistoryModel m_fileHistoryModel;
	CompositeRecentItemsDataSource m_compositeDataSource;
};

TEST_F(CompositeRecentItemsDataSourceTest, ItemsMergedInSourceOrder)
{
	// Note that the paths below use uppercase drive letters, since that's how the paths are
	// normalized when retrieved from the pidls.
	m_folderHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder1"));
	m_folderHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder2"));
	m_fileHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file1.txt"));

	auto items = m_compositeDataSource.GetItems();

	// The items from each source are concatenated, in the order in which the sources appear, and
	// ranked sequentially. The items from each source are in recency order (i.e. the most recent
	// item first).
	ASSERT_EQ(items.size(), 3u);
	EXPECT_EQ(items[0].GetPath(), L"C:\\folder2");
	EXPECT_EQ(items[0].GetKind(), RecentItem::Kind::Folder);
	EXPECT_EQ(items[0].GetRank(), 0);
	EXPECT_EQ(items[1].GetPath(), L"C:\\folder1");
	EXPECT_EQ(items[1].GetKind(), RecentItem::Kind::Folder);
	EXPECT_EQ(items[1].GetRank(), 1);
	EXPECT_EQ(items[2].GetPath(), L"C:\\file1.txt");
	EXPECT_EQ(items[2].GetKind(), RecentItem::Kind::File);
	EXPECT_EQ(items[2].GetRank(), 2);
}

TEST_F(CompositeRecentItemsDataSourceTest, ChangedObserverNotifiedForFolderSource)
{
	bool notified = false;
	m_compositeDataSource.AddChangedObserver([&notified] { notified = true; });

	m_folderHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\folder1"));

	EXPECT_TRUE(notified);
}

TEST_F(CompositeRecentItemsDataSourceTest, ChangedObserverNotifiedForFileSource)
{
	bool notified = false;
	m_compositeDataSource.AddChangedObserver([&notified] { notified = true; });

	m_fileHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file1.txt"));

	EXPECT_TRUE(notified);
}

TEST_F(CompositeRecentItemsDataSourceTest, ItemsCappedPerSource)
{
	// Add 25 items to the folder source. Only the 20 most recent items should be returned (the
	// history model returns items with the most recent item first).
	for (int i = 1; i <= 25; i++)
	{
		m_folderHistoryModel.AddHistoryItem(
			CreateSimplePidlForTest(L"c:\\folder" + std::to_wstring(i)));
	}

	auto items = m_compositeDataSource.GetItems();

	ASSERT_EQ(items.size(), 20u);
	EXPECT_EQ(items[0].GetPath(), L"C:\\folder25");
	EXPECT_EQ(items[0].GetRank(), 0);
	EXPECT_EQ(items[19].GetPath(), L"C:\\folder6");
	EXPECT_EQ(items[19].GetRank(), 19);
}

TEST_F(CompositeRecentItemsDataSourceTest, CapAppliedToEachSourceIndependently)
{
	// Both sources should contribute up to the limit, so that items from the file source aren't
	// crowded out by the items from the folder source.
	for (int i = 1; i <= 25; i++)
	{
		m_folderHistoryModel.AddHistoryItem(
			CreateSimplePidlForTest(L"c:\\folder" + std::to_wstring(i)));
	}

	m_fileHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file1.txt"));
	m_fileHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file2.txt"));
	m_fileHistoryModel.AddHistoryItem(CreateSimplePidlForTest(L"c:\\file3.txt"));

	auto items = m_compositeDataSource.GetItems();

	ASSERT_EQ(items.size(), 23u);
	EXPECT_EQ(items[0].GetPath(), L"C:\\folder25");
	EXPECT_EQ(items[19].GetPath(), L"C:\\folder6");
	EXPECT_EQ(items[20].GetPath(), L"C:\\file3.txt");
	EXPECT_EQ(items[20].GetKind(), RecentItem::Kind::File);
	EXPECT_EQ(items[20].GetRank(), 20);
	EXPECT_EQ(items[22].GetPath(), L"C:\\file1.txt");
	EXPECT_EQ(items[22].GetRank(), 22);
}
