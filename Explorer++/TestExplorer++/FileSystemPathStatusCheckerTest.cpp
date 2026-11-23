// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RecentItems/FileSystemPathStatusChecker.h"
#include "ScopedTestDir.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <optional>

using namespace testing;

namespace
{

// Returns a drive letter that doesn't exist on the current system (or nullopt if every letter
// from D to Z is in use). Drives A and C are skipped, since A may be a floppy drive (which can be
// slow to query) and C is typically the system drive.
std::optional<wchar_t> MaybeGetAbsentDriveLetter()
{
	DWORD drives = GetLogicalDrives();

	for (wchar_t letter = L'D'; letter <= L'Z'; letter++)
	{
		if (!(drives & (1 << (letter - L'A'))))
		{
			return letter;
		}
	}

	return std::nullopt;
}

// Returns the root path of one of the existing drives on the current system (e.g. "C:\").
std::wstring GetExistingDriveRoot()
{
	WCHAR driveStrings[MAX_PATH] = {};
	UINT res = GetLogicalDriveStrings(ARRAYSIZE(driveStrings), driveStrings);
	EXPECT_NE(res, 0u);
	EXPECT_LT(res, ARRAYSIZE(driveStrings));

	EXPECT_NE(driveStrings[0], L'\0');
	return driveStrings;
}

}

class FileSystemPathStatusCheckerTest : public Test
{
protected:
	ScopedTestDir m_testDir;
};

TEST_F(FileSystemPathStatusCheckerTest, ExistingDirectoryIsReachable)
{
	auto path = m_testDir.GetPath() / L"folder";
	std::filesystem::create_directory(path);

	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(path.wstring()), PathStatus::Reachable);
}

TEST_F(FileSystemPathStatusCheckerTest, ExistingFileIsReachable)
{
	auto path = m_testDir.GetPath() / L"file.txt";
	std::ofstream file(path);
	ASSERT_TRUE(file.is_open());

	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(path.wstring()), PathStatus::Reachable);
}

TEST_F(FileSystemPathStatusCheckerTest, MissingFileInExistingDirectoryIsDeleted)
{
	auto path = m_testDir.GetPath() / L"missing.txt";

	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(path.wstring()), PathStatus::Deleted);
}

TEST_F(FileSystemPathStatusCheckerTest, MissingDirectoryInExistingDirectoryIsDeleted)
{
	auto path = m_testDir.GetPath() / L"missing-folder";

	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(path.wstring()), PathStatus::Deleted);
}

TEST_F(FileSystemPathStatusCheckerTest, ExistingDriveRootIsReachable)
{
	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(GetExistingDriveRoot()),
		PathStatus::Reachable);
}

TEST_F(FileSystemPathStatusCheckerTest, PathOnAbsentDriveIsUnreachable)
{
	auto driveLetter = MaybeGetAbsentDriveLetter();

	if (!driveLetter)
	{
		GTEST_SKIP() << "No absent drive letter available";
	}

	std::wstring path;
	path += *driveLetter;
	path += L":\\missing-folder\\missing.txt";

	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(path), PathStatus::Unreachable);
}

TEST_F(FileSystemPathStatusCheckerTest, AbsentDriveRootIsUnreachable)
{
	auto driveLetter = MaybeGetAbsentDriveLetter();

	if (!driveLetter)
	{
		GTEST_SKIP() << "No absent drive letter available";
	}

	std::wstring root;
	root += *driveLetter;
	root += L":\\";

	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(root), PathStatus::Unreachable);
}

TEST_F(FileSystemPathStatusCheckerTest, PathWithoutRecognizableRootIsUnreachable)
{
	// A relative path doesn't have a root, so there's no way to determine whether the path has
	// been deleted.
	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(L"relative\\path"),
		PathStatus::Unreachable);

	// A UNC path that only contains a server name doesn't identify a volume that can be checked.
	EXPECT_EQ(FileSystemPathStatusChecker::CheckPathStatus(L"\\\\server"), PathStatus::Unreachable);
}

TEST_F(FileSystemPathStatusCheckerTest, VirtualPathIsReachable)
{
	// Virtual (non-filesystem) items aren't checked at all.
	EXPECT_EQ(
		FileSystemPathStatusChecker::CheckPathStatus(L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"),
		PathStatus::Reachable);
}
