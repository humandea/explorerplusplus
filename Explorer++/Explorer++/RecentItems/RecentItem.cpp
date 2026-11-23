// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentItem.h"

RecentItem::RecentItem(const std::wstring &path, Kind kind, int rank,
	std::optional<PidlAbsolute> pidl) :
	m_path(path),
	m_kind(kind),
	m_rank(rank),
	m_pidl(std::move(pidl))
{
}

const std::wstring &RecentItem::GetPath() const
{
	return m_path;
}

RecentItem::Kind RecentItem::GetKind() const
{
	return m_kind;
}

bool RecentItem::IsVirtualPath() const
{
	// See IsPathGUID() in Helper\ShellHelper.cpp, which uses the same check.
	return m_path.starts_with(L"::");
}

int RecentItem::GetRank() const
{
	return m_rank;
}

const std::optional<PidlAbsolute> &RecentItem::GetPidl() const
{
	return m_pidl;
}
