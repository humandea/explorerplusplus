// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/FileFrequentLocationsDataSource.h"
#include "FileFrequentLocationsModel.h"
#include "PidlTestHelper.h"
#include "SystemClockFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class FileFrequentLocationsDataSourceTest : public Test
{
protected:
	FileFrequentLocationsDataSourceTest() : m_model(&m_systemClock), m_dataSource(&m_model)
	{
	}

	SystemClockFake m_systemClock;
	FileFrequentLocationsModel m_model;
	FileFrequentLocationsDataSource m_dataSource;
};

TEST_F(FileFrequentLocationsDataSourceTest, ItemsReturnedInVisitOrder)
{
	// Note that the paths below use uppercase drive letters, since that's how the paths are
	// normalized when retrieved from the pidls.
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\rarely.txt"));
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\often.txt"));
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\often.txt"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 2u);

	// The most frequently opened file should appear first.
	EXPECT_EQ(items[0].GetPath(), L"C:\\often.txt");
	EXPECT_EQ(items[0].GetRank(), 0);
	EXPECT_EQ(items[1].GetPath(), L"C:\\rarely.txt");
	EXPECT_EQ(items[1].GetRank(), 1);
}

TEST_F(FileFrequentLocationsDataSourceTest, ItemsAreFiles)
{
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\file1.txt"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].GetKind(), RecentItem::Kind::File);
	EXPECT_EQ(items[0].GetPidl(), CreateSimplePidlForTest(L"c:\\file1.txt"));
}

TEST_F(FileFrequentLocationsDataSourceTest, ChangedObserverNotified)
{
	bool notified = false;
	m_dataSource.AddChangedObserver([&notified] { notified = true; });

	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\file1.txt"));

	EXPECT_TRUE(notified);
}
