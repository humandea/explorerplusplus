// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/FolderFrequentLocationsDataSource.h"
#include "FrequentLocationsModel.h"
#include "PidlTestHelper.h"
#include "SystemClockFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class FolderFrequentLocationsDataSourceTest : public Test
{
protected:
	FolderFrequentLocationsDataSourceTest() : m_model(&m_systemClock), m_dataSource(&m_model)
	{
	}

	SystemClockFake m_systemClock;
	FrequentLocationsModel m_model;
	FolderFrequentLocationsDataSource m_dataSource;
};

TEST_F(FolderFrequentLocationsDataSourceTest, ItemsReturnedInVisitOrder)
{
	// Note that the paths below use uppercase drive letters, since that's how the paths are
	// normalized when retrieved from the pidls.
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\rarely-visited"));
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\often-visited"));
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\often-visited"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 2u);

	// The most frequently visited folder should appear first.
	EXPECT_EQ(items[0].GetPath(), L"C:\\often-visited");
	EXPECT_EQ(items[0].GetRank(), 0);
	EXPECT_EQ(items[1].GetPath(), L"C:\\rarely-visited");
	EXPECT_EQ(items[1].GetRank(), 1);
}

TEST_F(FolderFrequentLocationsDataSourceTest, ItemsAreFolders)
{
	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\folder1"));

	auto items = m_dataSource.GetItems();

	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].GetKind(), RecentItem::Kind::Folder);
	EXPECT_EQ(items[0].GetPidl(), CreateSimplePidlForTest(L"c:\\folder1"));
}

TEST_F(FolderFrequentLocationsDataSourceTest, ChangedObserverNotified)
{
	bool notified = false;
	m_dataSource.AddChangedObserver([&notified] { notified = true; });

	m_model.RegisterLocationVisit(CreateSimplePidlForTest(L"c:\\folder1"));

	EXPECT_TRUE(notified);
}
