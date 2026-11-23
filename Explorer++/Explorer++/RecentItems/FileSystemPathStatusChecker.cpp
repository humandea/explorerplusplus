// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileSystemPathStatusChecker.h"

FileSystemPathStatusChecker::FileSystemPathStatusChecker(HWND hwnd) : m_hwnd(hwnd), m_threadPool(1)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
		std::bind_front(&FileSystemPathStatusChecker::OwnerWindowSubclass, this)));
}

FileSystemPathStatusChecker::~FileSystemPathStatusChecker()
{
	m_threadPool.clear_queue();
}

LRESULT FileSystemPathStatusChecker::OwnerWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_APP_PATH_STATUS_READY:
		ProcessCheckResult(static_cast<int>(wParam));
		return 0;
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

PathStatus FileSystemPathStatusChecker::GetStatus(const std::wstring &path) const
{
	// Virtual (non-filesystem) items don't refer to anything on disk, so they're always treated
	// as reachable.
	if (path.starts_with(L"::"))
	{
		return PathStatus::Reachable;
	}

	auto cacheItr = m_cache.find(path);

	if (cacheItr != m_cache.end())
	{
		const auto &entry = cacheItr->second;

		if (entry.status == PathStatus::Unreachable
			|| (std::chrono::steady_clock::now() - entry.checkTime) < CACHE_TTL)
		{
			return entry.status;
		}

		// The cached status is stale. It's returned here, while a fresh check runs in the
		// background. That way, a path isn't removed from (or added back to) the tree until its
		// status has been re-confirmed.
		QueueCheck(path);
		return entry.status;
	}

	if (m_pendingPaths.contains(path))
	{
		// A check is already running for this path. Until it completes, the path is kept
		// unchanged.
		return PathStatus::Reachable;
	}

	QueueCheck(path);

	// The path hasn't been checked yet, so it's kept unchanged until its status is known.
	return PathStatus::Reachable;
}

void FileSystemPathStatusChecker::QueueCheck(const std::wstring &path) const
{
	// Only a single check is ever queued for a path.
	if (!m_pendingPaths.insert(path).second)
	{
		return;
	}

	int resultId = m_resultIdCounter++;

	auto future = m_threadPool.push(
		[this, resultId, copiedPath = path](int id) -> PathStatus
		{
			UNREFERENCED_PARAMETER(id);

			auto status = CheckPathStatus(copiedPath);

			PostMessage(m_hwnd, WM_APP_PATH_STATUS_READY, resultId, 0);

			return status;
		});

	PendingResult pendingResult;
	pendingResult.path = path;
	pendingResult.future = std::move(future);
	m_pendingResults.insert({ resultId, std::move(pendingResult) });
}

void FileSystemPathStatusChecker::ProcessCheckResult(int resultId)
{
	auto resultItr = m_pendingResults.find(resultId);

	if (resultItr == m_pendingResults.end())
	{
		return;
	}

	auto pendingResult = std::move(resultItr->second);
	m_pendingResults.erase(resultItr);
	m_pendingPaths.erase(pendingResult.path);

	auto status = pendingResult.future.get();

	auto effectiveStatus = PathStatus::Reachable;
	auto cacheItr = m_cache.find(pendingResult.path);

	if (cacheItr != m_cache.end())
	{
		effectiveStatus = cacheItr->second.status;
		cacheItr->second.status = status;
		cacheItr->second.checkTime = std::chrono::steady_clock::now();
	}
	else
	{
		m_cache.insert({ pendingResult.path, { status, std::chrono::steady_clock::now() } });
	}

	// Observers only need to be notified when the status they last saw for this path has changed
	// (e.g. when a path has been confirmed to be deleted).
	if (status != effectiveStatus)
	{
		m_statusChangedSignal();
	}
}

boost::signals2::connection FileSystemPathStatusChecker::AddStatusChangedObserver(
	const StatusChangedSignal::slot_type &observer)
{
	return m_statusChangedSignal.connect(observer);
}

PathStatus FileSystemPathStatusChecker::CheckPathStatus(const std::wstring &path)
{
	// Virtual (non-filesystem) paths don't refer to anything on disk, so they're always treated
	// as reachable.
	if (path.starts_with(L"::"))
	{
		return PathStatus::Reachable;
	}

	auto root = MaybeGetPathRoot(path);

	if (!root)
	{
		// The path doesn't have a recognizable root, so there's no way to determine whether it
		// has been deleted. It's kept unchanged.
		return PathStatus::Unreachable;
	}

	// If the root itself can't be accessed (e.g. because it's on a disconnected network share or
	// an absent drive), it's not known whether the path still exists, so it's kept unchanged.
	if (GetFileAttributes(root->c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		return PathStatus::Unreachable;
	}

	if (_wcsicmp(path.c_str(), root->c_str()) == 0)
	{
		return PathStatus::Reachable;
	}

	if (GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		// The root is accessible, so the path is known to no longer exist.
		return PathStatus::Deleted;
	}

	return PathStatus::Reachable;
}

// Returns the root of the path (i.e. the drive root or UNC share root), or nullopt if the path
// doesn't have a recognizable root.
std::optional<std::wstring> FileSystemPathStatusChecker::MaybeGetPathRoot(const std::wstring &path)
{
	if (path.size() >= 2 && path[1] == L':')
	{
		// e.g. "C:\" (a parsing path always includes the separator for a drive root).
		return path.substr(0, 3);
	}

	if (path.starts_with(L"\\\\"))
	{
		// e.g. "\\server\share". Anything shorter than a full share (i.e. just "\\server")
		// doesn't identify a volume that can be checked.
		size_t shareStartIndex = path.find(L'\\', 2);

		if (shareStartIndex == std::wstring::npos)
		{
			return std::nullopt;
		}

		size_t pathStartIndex = path.find(L'\\', shareStartIndex + 1);

		if (pathStartIndex == std::wstring::npos)
		{
			return path;
		}

		return path.substr(0, pathStartIndex + 1);
	}

	return std::nullopt;
}
