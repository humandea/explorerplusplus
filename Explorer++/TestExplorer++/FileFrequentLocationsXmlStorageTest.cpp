// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "FrequentLocationsXmlStorage.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/SystemClockImpl.h"
#include <gtest/gtest.h>

class FileFrequentLocationsXmlStorageTest : public XmlStorageTest
{
protected:
	SystemClockImpl m_systemClock;
};

TEST_F(FileFrequentLocationsXmlStorageTest, SaveLoadRoundTrip)
{
	FrequentLocationsModel referenceModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	auto xmlDocumentData = CreateXmlDocument();

	FrequentLocationsXmlStorage::SaveFiles(xmlDocumentData.xmlDocument.get(),
		xmlDocumentData.rootNode.get(), &referenceModel);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsXmlStorage::LoadFiles(xmlDocumentData.rootNode.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(FileFrequentLocationsXmlStorageTest, LoadWithoutFileDataReturnsEmptyModel)
{
	// No file data has been saved, so loading should result in an empty model (this also covers
	// the case of settings that were created by an older version without file data).
	auto xmlDocumentData = CreateXmlDocument();

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsXmlStorage::LoadFiles(xmlDocumentData.rootNode.get(), &loadedModel);

	EXPECT_EQ(loadedModel.GetVisits().size(), 0u);
}

TEST_F(FileFrequentLocationsXmlStorageTest, FolderEntriesAreNotReturned)
{
	FrequentLocationsModel folderModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&folderModel);

	auto xmlDocumentData = CreateXmlDocument();

	// Save folder entries, then verify that loading the file entries doesn't return them.
	FrequentLocationsXmlStorage::Save(xmlDocumentData.xmlDocument.get(),
		xmlDocumentData.rootNode.get(), &folderModel);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsXmlStorage::LoadFiles(xmlDocumentData.rootNode.get(), &loadedModel);

	EXPECT_EQ(loadedModel.GetVisits().size(), 0u);
}
