// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DebugRecentItemsDataSource.h"

#ifdef _DEBUG

namespace
{

const wchar_t UNREACHABLE_PATH[] = L"\\\\offline\\volume\\docs";
const wchar_t DELETED_PATH[] = L"C:\\DeletedFolder";

// The paths below don't have to exist on disk; they're purely used to exercise the tree building
// and display logic. The position of an item in this list is its rank.
const std::vector<RecentItem> CreateItems()
{
	int rank = 0;

	// A deep child with several untracked folders in between. Once the items further down the
	// list are added, the items nest directly underneath each other.
	// (C:\Projects -> App\src -> ui)
	std::vector<RecentItem> items;
	items.emplace_back(L"C:\\Projects\\App\\src\\ui", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Projects\\App\\src\\ui\\main.cpp", RecentItem::Kind::File, rank++);
	items.emplace_back(L"C:\\Projects\\App\\tests\\unit\\runner.cpp", RecentItem::Kind::File,
		rank++);

	// These two items share a single chain node ("tests" underneath C:\Projects\App).
	items.emplace_back(L"C:\\Projects\\App\\tests\\unit", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Projects\\App\\tests\\integration", RecentItem::Kind::Folder, rank++);

	items.emplace_back(L"C:\\Tools\\build\\release", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Tools\\build\\debug", RecentItem::Kind::Folder, rank++);

	// The first two items share a single chain node ("archives" underneath C:\Tools, since their
	// parent folder is the same), while the third item gets its own chain node.
	items.emplace_back(L"C:\\Tools\\archives\\2023", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Tools\\archives\\2024", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Tools\\logs", RecentItem::Kind::Folder, rank++);

	items.emplace_back(L"D:\\Work\\archive\\2023", RecentItem::Kind::Folder, rank++);

	// These two items share a single chain node ("reports" underneath D:\Work).
	items.emplace_back(L"D:\\Work\\reports\\invoices", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"D:\\Work\\reports\\expenses", RecentItem::Kind::Folder, rank++);

	// UNC paths.
	items.emplace_back(L"\\\\server\\share\\data", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"\\\\server\\share\\data\\exports", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"\\\\server\\share\\data\\exports\\report.xlsx", RecentItem::Kind::File,
		rank++);
	items.emplace_back(L"\\\\server\\share\\data\\exports\\summary.pdf", RecentItem::Kind::File,
		rank++);

	// A file with no tracked ancestor, so it becomes a root node.
	items.emplace_back(L"\\\\server\\share\\readme.txt", RecentItem::Kind::File, rank++);

	// Unreachable items are kept unchanged.
	items.emplace_back(UNREACHABLE_PATH, RecentItem::Kind::Folder, rank++);

	// The parent of this item is deleted, so this item becomes a root node.
	items.emplace_back(L"C:\\DeletedFolder\\child", RecentItem::Kind::Folder, rank++);

	// Virtual items are always root nodes.
	items.emplace_back(L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", RecentItem::Kind::Folder,
		rank++);
	items.emplace_back(L"::{645FF040-5081-101B-9F08-00AA002F954E}", RecentItem::Kind::Folder,
		rank++);

	// Files that are direct children of tracked folders.
	items.emplace_back(L"C:\\Projects\\README.md", RecentItem::Kind::File, rank++);
	items.emplace_back(L"C:\\Tools\\notes.txt", RecentItem::Kind::File, rank++);

	// This item is ranked lower than its children above, but still sits above them in the tree.
	items.emplace_back(L"C:\\Projects", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Tools", RecentItem::Kind::Folder, rank++);

	// A mixed-case duplicate of the item above; ignored.
	items.emplace_back(L"c:\\tools", RecentItem::Kind::Folder, rank++);

	// Once these items are added, the untracked folders in between the items at the top of the
	// list disappear (i.e. items then nest directly underneath each other).
	items.emplace_back(L"C:\\Tools\\build", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Projects\\App", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Projects\\App\\src", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Projects\\App\\src\\ui\\widgets", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Projects\\App\\src\\ui\\themes", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"C:\\Projects\\App\\src\\ui\\main.h", RecentItem::Kind::File, rank++);

	// A tracked drive root and a consecutive tracked level underneath it.
	items.emplace_back(L"D:\\", RecentItem::Kind::Folder, rank++);
	items.emplace_back(L"D:\\Work", RecentItem::Kind::Folder, rank++);

	items.emplace_back(L"D:\\Work\\reports\\invoices\\2024", RecentItem::Kind::Folder, rank++);

	items.emplace_back(L"C:\\Tools\\logs\\app.log", RecentItem::Kind::File, rank++);

	// A file inside untracked folders, which will appear underneath a chain node
	// ("docs" underneath C:\Projects\App).
	items.emplace_back(L"C:\\Projects\\App\\docs\\api.md", RecentItem::Kind::File, rank++);

	// An exact duplicate of an item above; ignored.
	items.emplace_back(L"C:\\Projects", RecentItem::Kind::Folder, rank++);

	// Deleted items are removed from the tree.
	items.emplace_back(DELETED_PATH, RecentItem::Kind::Folder, rank++);

	return items;
}

}

std::vector<RecentItem> DebugRecentItemsDataSource::GetItems() const
{
	return CreateItems();
}

boost::signals2::connection DebugRecentItemsDataSource::AddChangedObserver(
	const ChangedSignal::slot_type &observer)
{
	// The set of items never changes.
	UNREFERENCED_PARAMETER(observer);

	return {};
}

PathStatus DebugPathStatusChecker::GetStatus(const std::wstring &path) const
{
	if (_wcsicmp(path.c_str(), UNREACHABLE_PATH) == 0)
	{
		return PathStatus::Unreachable;
	}

	if (_wcsicmp(path.c_str(), DELETED_PATH) == 0)
	{
		return PathStatus::Deleted;
	}

	return PathStatus::Reachable;
}

#endif
