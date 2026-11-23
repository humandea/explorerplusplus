// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

enum class PathStatus
{
	// The path exists and is reachable.
	Reachable,

	// The path couldn't be checked (for example, because it's on a disconnected network share or
	// an absent drive). Unreachable items are kept as-is, since it's not known whether they
	// still exist.
	Unreachable,

	// The path was confirmed to no longer exist (i.e. the containing volume or parent folder is
	// reachable, but the path is missing).
	Deleted
};

// Checks the status of a path. Implementations are expected to perform any actual checks in the
// background and cache results, since checks can be slow (for example, on disconnected network
// drives). Callers should treat this interface as a cheap lookup and avoid invoking it on the UI
// thread for paths that haven't been checked yet.
class PathStatusChecker
{
public:
	virtual ~PathStatusChecker() = default;

	virtual PathStatus GetStatus(const std::wstring &path) const = 0;
};
