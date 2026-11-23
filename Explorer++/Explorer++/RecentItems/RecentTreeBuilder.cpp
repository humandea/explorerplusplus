// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentTreeBuilder.h"
#include <algorithm>
#include <map>
#include <set>

namespace
{

// Provides case-insensitive ordering of paths, for use with associative containers.
struct CaseInsensitiveLess
{
	bool operator()(const std::wstring &first, const std::wstring &second) const
	{
		return _wcsicmp(first.c_str(), second.c_str()) < 0;
	}
};

// A tracked item, along with its path in a comparable form.
struct TrackedItem
{
	RecentItem item;
	std::wstring comparablePath;
};

// Returns a comparable form of the path: forward slashes are converted to backslashes and
// trailing separators are removed. Case is preserved; path comparisons themselves are performed
// case-insensitively.
std::wstring GetComparablePath(const std::wstring &path)
{
	std::wstring comparablePath = path;
	std::replace(comparablePath.begin(), comparablePath.end(), L'/', L'\\');

	while (comparablePath.size() > 1 && comparablePath.back() == L'\\')
	{
		comparablePath.pop_back();
	}

	return comparablePath;
}

// Virtual (non-filesystem) items are identified by a GUID-style parsing path (e.g. "::{...}").
// See IsPathGUID() in Helper\ShellHelper.cpp, which uses the same check.
bool IsVirtualPath(const std::wstring &comparablePath)
{
	return comparablePath.starts_with(L"::");
}

// e.g. "C:".
bool IsDriveRoot(const std::wstring &comparablePath)
{
	return comparablePath.size() == 2 && comparablePath[1] == L':';
}

// e.g. "\\server" or "\\server\share".
bool IsUncRoot(const std::wstring &comparablePath)
{
	if (comparablePath.size() < 3 || comparablePath[0] != L'\\' || comparablePath[1] != L'\\')
	{
		return false;
	}

	auto numBackslashes = std::count(comparablePath.begin(), comparablePath.end(), L'\\');
	return numBackslashes == 2 || numBackslashes == 3;
}

bool IsPathRoot(const std::wstring &comparablePath)
{
	return IsDriveRoot(comparablePath) || IsUncRoot(comparablePath);
}

// Indicates whether prefix is a proper prefix of path, ending on a path separator boundary. Root
// paths (drive roots and UNC roots) are always on a boundary.
bool IsBoundaryPrefix(const std::wstring &prefix, const std::wstring &path)
{
	if (prefix.size() >= path.size())
	{
		return false;
	}

	if (_wcsnicmp(path.c_str(), prefix.c_str(), prefix.size()) != 0)
	{
		return false;
	}

	return IsPathRoot(prefix) || path[prefix.size()] == L'\\';
}

// Returns the final segment of the path, which is the name displayed for it. Root paths are
// returned unchanged (with a trailing separator added back for drive roots).
std::wstring GetNameFromComparablePath(const std::wstring &comparablePath)
{
	if (IsDriveRoot(comparablePath))
	{
		return comparablePath + L"\\";
	}

	if (IsUncRoot(comparablePath) || IsVirtualPath(comparablePath))
	{
		return comparablePath;
	}

	size_t lastSeparatorIndex = comparablePath.find_last_of(L'\\');

	if (lastSeparatorIndex == std::wstring::npos)
	{
		return comparablePath;
	}

	return comparablePath.substr(lastSeparatorIndex + 1);
}

// Joins a parent path and a relative path. The parent path may end with a separator (e.g. when
// it's a drive root like "C:\").
std::wstring JoinPath(const std::wstring &parent, const std::wstring &relativePath)
{
	if (!parent.empty() && parent.back() == L'\\')
	{
		return parent + relativePath;
	}

	return parent + L"\\" + relativePath;
}

// Returns the parent path of the given comparable path, or an empty string if the path is a root
// path (or otherwise has no parent).
std::wstring GetParentPath(const std::wstring &comparablePath)
{
	if (IsPathRoot(comparablePath))
	{
		return {};
	}

	size_t lastSeparatorIndex = comparablePath.find_last_of(L'\\');

	if (lastSeparatorIndex == std::wstring::npos)
	{
		return {};
	}

	return comparablePath.substr(0, lastSeparatorIndex);
}

}

std::vector<std::unique_ptr<RecentTreeNode>> RecentTreeBuilder::Build(
	const std::vector<RecentItem> &items, const PathStatusChecker *pathStatusChecker)
{
	// The items are expected to be in rank order, but they're sorted here as well, so that the
	// first occurrence of a duplicate is always the occurrence with the best rank.
	std::vector<RecentItem> sortedItems = items;
	std::stable_sort(sortedItems.begin(), sortedItems.end(),
		[](const RecentItem &first, const RecentItem &second)
		{ return first.GetRank() < second.GetRank(); });

	std::vector<TrackedItem> trackedItems;
	std::set<std::wstring, CaseInsensitiveLess> seenPaths;

	for (const auto &item : sortedItems)
	{
		if (item.GetPath().empty())
		{
			continue;
		}

		auto comparablePath = GetComparablePath(item.GetPath());

		// Duplicate items are added once; later duplicates are ignored.
		if (!seenPaths.insert(comparablePath).second)
		{
			continue;
		}

		// Deleted items are excluded from the tree. Their children are reattached to the next
		// tracked ancestor (or become root nodes), since the ancestor lookup below only considers
		// the items that remain.
		if (pathStatusChecker
			&& pathStatusChecker->GetStatus(item.GetPath()) == PathStatus::Deleted)
		{
			continue;
		}

		trackedItems.push_back({ item, comparablePath });
	}

	// Nodes for all tracked items are created up front, so that an item can be nested underneath
	// an ancestor that has a worse rank (i.e. that appears later in the list).
	std::vector<std::unique_ptr<RecentTreeNode>> ownedNodes;
	std::vector<RecentTreeNode *> itemNodes;
	itemNodes.reserve(trackedItems.size());

	for (const auto &trackedItem : trackedItems)
	{
		auto node = std::make_unique<RecentTreeNode>(RecentTreeNode::Type::Tracked,
			GetNameFromComparablePath(trackedItem.comparablePath), trackedItem.item.GetPath(),
			trackedItem.item.GetRank(), trackedItem.item);
		itemNodes.push_back(node.get());
		ownedNodes.push_back(std::move(node));
	}

	// Keyed on (ancestor path, gap), so that tracked children that share the same tracked
	// ancestor and an identical gap share a single chain node.
	std::map<std::pair<std::wstring, std::wstring>, RecentTreeNode *> chainNodes;
	std::vector<std::unique_ptr<RecentTreeNode>> roots;

	// Items are processed in rank order. That way, children are added to their parent in rank
	// order and a chain node is created at the rank of its best-ranked child (i.e. the first
	// child that needs it).
	for (size_t index = 0; index < trackedItems.size(); index++)
	{
		const TrackedItem &trackedItem = trackedItems[index];

		// The nearest tracked ancestor is the longest tracked path prefix. Virtual items are
		// always root nodes and are never treated as ancestors of other items.
		const TrackedItem *ancestor = nullptr;

		if (!IsVirtualPath(trackedItem.comparablePath))
		{
			for (const auto &candidate : trackedItems)
			{
				if (&candidate == &trackedItem
					|| candidate.item.GetKind() != RecentItem::Kind::Folder
					|| IsVirtualPath(candidate.comparablePath))
				{
					continue;
				}

				if (!IsBoundaryPrefix(candidate.comparablePath, trackedItem.comparablePath))
				{
					continue;
				}

				if (!ancestor || candidate.comparablePath.size() > ancestor->comparablePath.size())
				{
					ancestor = &candidate;
				}
			}
		}

		if (!ancestor)
		{
			roots.push_back(std::move(ownedNodes[index]));
			continue;
		}

		RecentTreeNode *ancestorNode =
			itemNodes[static_cast<size_t>(ancestor - trackedItems.data())];

		// The gap is the path in between the tracked ancestor and the tracked item (i.e. the
		// relative path of the item's parent folder). When the item is a direct child of its
		// ancestor, the gap is empty.
		std::wstring parentPath = GetParentPath(trackedItem.comparablePath);

		if (parentPath.empty())
		{
			// The item doesn't have a parent path (e.g. because it's a malformed path), so it
			// can't be nested underneath another item.
			roots.push_back(std::move(ownedNodes[index]));
			continue;
		}

		if (_wcsicmp(parentPath.c_str(), ancestor->comparablePath.c_str()) == 0)
		{
			ancestorNode->AddChild(std::move(ownedNodes[index]));
			continue;
		}

		size_t gapStartIndex = ancestor->comparablePath.size();

		if (IsPathRoot(ancestor->comparablePath))
		{
			// The separator in between a root path and the rest of the path is part of the root
			// path itself (e.g. "D:\"), so it has to be skipped explicitly here.
			gapStartIndex++;
		}

		std::wstring gap = parentPath.substr(gapStartIndex);

		while (!gap.empty() && gap.front() == L'\\')
		{
			gap.erase(gap.begin());
		}

		auto [iterator, inserted] =
			chainNodes.try_emplace(std::make_pair(ancestor->comparablePath, gap), nullptr);

		if (inserted)
		{
			// The chain node is created the first time a child needs it. Since items are
			// processed in rank order, the rank of the first child is the best rank of any child
			// of the chain node.
			auto chainNode = std::make_unique<RecentTreeNode>(RecentTreeNode::Type::Chain, gap,
				JoinPath(ancestor->item.GetPath(), gap), trackedItem.item.GetRank(), std::nullopt);
			iterator->second = chainNode.get();
			ancestorNode->AddChild(std::move(chainNode));
		}

		iterator->second->AddChild(std::move(ownedNodes[index]));
	}

	return roots;
}
