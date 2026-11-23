// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PathStatusChecker.h"
#include "../Helper/WindowSubclass.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/signals2.hpp>
#include <chrono>
#include <future>
#include <map>
#include <optional>
#include <set>

// Performs existence checks for filesystem paths, backing the PathStatusChecker interface.
//
// Checks run on a background thread, since they can be slow (for example, when a path is on a
// disconnected network share). Results are cached; GetStatus() returns the cached status for a
// path that has been checked and a provisional status of Reachable for a path that hasn't been
// checked yet (or that is currently being checked). That way, callers never block.
//
// When a background check completes, observers are notified (on the thread the window passed to
// the constructor was created on, i.e. the UI thread), so that the tree can be rebuilt with the
// updated statuses. Observers are only notified when the effective status of a path changes.
class FileSystemPathStatusChecker : public PathStatusChecker
{
public:
	using StatusChangedSignal = boost::signals2::signal<void()>;

	// The checker subclasses the given window in order to marshal the results of background
	// checks back to the thread the window was created on. The caller is expected to ensure that
	// the window outlives this instance.
	explicit FileSystemPathStatusChecker(HWND hwnd);
	~FileSystemPathStatusChecker();

	PathStatus GetStatus(const std::wstring &path) const override;

	boost::signals2::connection AddStatusChangedObserver(
		const StatusChangedSignal::slot_type &observer);

	// Checks the status of a path synchronously. This blocks, so it should only be called when
	// that's acceptable (e.g. in tests). Exposed here so that the classification logic can be
	// tested independently of the background/caching behavior.
	static PathStatus CheckPathStatus(const std::wstring &path);

private:
	// This is adjacent to the end of the WM_APP range used by IconFetcherImpl (which subclasses a
	// different window). The value only has to be unique for the window that's subclassed here.
	static const UINT WM_APP_PATH_STATUS_READY = 0xBFFE;

	// Cached statuses are re-checked once they become stale, so that a path that comes back (e.g.
	// a deleted folder that's recreated) is picked up. Unreachable statuses are cached for the
	// lifetime of the instance instead, since re-checking an unreachable volume can be slow and
	// unreachable items are kept as-is in any case.
	static constexpr auto CACHE_TTL = std::chrono::seconds(30);

	struct CacheEntry
	{
		PathStatus status;
		std::chrono::steady_clock::time_point checkTime;
	};

	struct PendingResult
	{
		std::wstring path;
		std::future<PathStatus> future;
	};

	// Provides case-insensitive ordering of paths, for use with associative containers.
	struct CaseInsensitiveLess
	{
		bool operator()(const std::wstring &first, const std::wstring &second) const
		{
			return _wcsicmp(first.c_str(), second.c_str()) < 0;
		}
	};

	LRESULT OwnerWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void QueueCheck(const std::wstring &path) const;
	void ProcessCheckResult(int resultId);

	static std::optional<std::wstring> MaybeGetPathRoot(const std::wstring &path);

	HWND m_hwnd;
	mutable ctpl::thread_pool m_threadPool;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	mutable std::map<std::wstring, CacheEntry, CaseInsensitiveLess> m_cache;
	mutable std::set<std::wstring, CaseInsensitiveLess> m_pendingPaths;
	mutable std::map<int, PendingResult> m_pendingResults;
	mutable int m_resultIdCounter = 0;

	StatusChangedSignal m_statusChangedSignal;
};
