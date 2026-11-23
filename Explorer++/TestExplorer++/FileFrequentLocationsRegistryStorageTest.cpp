// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsRegistryStorage.h"
#include "FrequentLocationsStorageHelper.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include "../Helper/SystemClockImpl.h"
#include <gtest/gtest.h>

class FileFrequentLocationsRegistryStorageTest : public RegistryStorageTest
{
protected:
	SystemClockImpl m_systemClock;
};

TEST_F(FileFrequentLocationsRegistryStorageTest, SaveLoadRoundTrip)
{
	FrequentLocationsModel referenceModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	FrequentLocationsRegistryStorage::SaveFiles(m_applicationTestKey.get(), &referenceModel);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsRegistryStorage::LoadFiles(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(FileFrequentLocationsRegistryStorageTest, LoadWithoutFileDataReturnsEmptyModel)
{
	// No file data has been saved, so loading should result in an empty model (this also covers
	// the case of settings that were created by an older version without file data).
	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsRegistryStorage::LoadFiles(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel.GetVisits().size(), 0u);
}

TEST_F(FileFrequentLocationsRegistryStorageTest, FolderEntriesAreNotReturned)
{
	FrequentLocationsModel folderModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&folderModel);

	// Save folder entries, then verify that loading the file entries doesn't return them.
	FrequentLocationsRegistryStorage::Save(m_applicationTestKey.get(), &folderModel);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsRegistryStorage::LoadFiles(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel.GetVisits().size(), 0u);
}

TEST_F(FileFrequentLocationsRegistryStorageTest, OnlyTopItemsSaved)
{
	// Save more entries than the storage limit. Only the top entries (by visit count) should be
	// saved.
	FrequentLocationsModel model(&m_systemClock);
	std::vector<LocationVisitInfo> visits;

	for (int i = 0; i < FrequentLocationsStorageHelper::MAX_ITEMS_TO_STORE + 10; i++)
	{
		visits.push_back(FrequentLocationsStorageTestHelper::BuildFrequentLocation(L"c:\\file"
				+ std::to_wstring(i) + L".txt",
			i + 1, SystemClock::TimePoint(std::chrono::microseconds(i))));
	}

	model.SetLocationVisits(visits);

	FrequentLocationsRegistryStorage::SaveFiles(m_applicationTestKey.get(), &model);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsRegistryStorage::LoadFiles(m_applicationTestKey.get(), &loadedModel);

	ASSERT_EQ(loadedModel.GetVisits().size(), FrequentLocationsStorageHelper::MAX_ITEMS_TO_STORE);

	// The least visited entry should have been evicted. The entries were saved with visit counts
	// of 1 to (MAX_ITEMS_TO_STORE + 10), so the lowest remaining count is
	// (10 + 1).
	EXPECT_EQ(loadedModel.GetVisits().rbegin()->GetNumVisits(),
		FrequentLocationsStorageHelper::MAX_ITEMS_TO_STORE + 10
			- FrequentLocationsStorageHelper::MAX_ITEMS_TO_STORE + 1);
}
