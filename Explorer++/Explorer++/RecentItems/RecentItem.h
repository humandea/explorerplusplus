// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Pidl.h"
#include <optional>
#include <string>

// An item tracked by a RecentItemsDataSource. The path is a shell parsing path (i.e. the path
// returned by GetDisplayName() with SHGDN_FORPARSING). For filesystem items, this is a normal
// path; for virtual items, it's a GUID-style path (e.g. "::{...}").
class RecentItem
{
public:
	enum class Kind
	{
		Folder,
		File
	};

	RecentItem(const std::wstring &path, Kind kind, int rank,
		std::optional<PidlAbsolute> pidl = std::nullopt);

	const std::wstring &GetPath() const;
	Kind GetKind() const;

	// Indicates whether the item is virtual (i.e. non-filesystem). Virtual items have a GUID-style
	// parsing path (e.g. "::{...}") and don't refer to anything on disk.
	bool IsVirtualPath() const;

	// Indicates the position of this item in the source list, with lower values indicating more
	// important items (e.g. items that were visited more recently or more often).
	int GetRank() const;
	const std::optional<PidlAbsolute> &GetPidl() const;

private:
	std::wstring m_path;
	Kind m_kind;
	int m_rank;
	std::optional<PidlAbsolute> m_pidl;
};
