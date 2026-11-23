// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PathStatusChecker.h"
#include "RecentItem.h"
#include "RecentTreeNode.h"
#include <memory>
#include <vector>

// Builds the tree displayed in the recent items pane from a set of tracked items. The tree is
// always rebuilt from the full set of items (rather than being updated incrementally), so that a
// late-arriving parent item correctly adopts items that were added earlier.
//
// Each item is nested under its nearest tracked ancestor (the longest tracked path prefix). When
// untracked folders lie in between, a display-only chain node is inserted between the tracked
// ancestor and the tracked item. Nodes are ordered by rank (their position in the source list),
// with a parent always appearing above its children. Virtual (non-filesystem) items are always
// root nodes.
class RecentTreeBuilder
{
public:
	// Builds the tree from the given items, which are expected to be in rank order. Duplicate
	// items (matching case-insensitively) are ignored; the occurrence with the best rank is kept.
	//
	// When pathStatusChecker is provided, items whose path is reported as deleted are excluded
	// from the tree; unreachable items are kept unchanged. The checker is only used for lookups
	// here, so it's expected to return cached results (see PathStatusChecker).
	static std::vector<std::unique_ptr<RecentTreeNode>> Build(const std::vector<RecentItem> &items,
		const PathStatusChecker *pathStatusChecker = nullptr);
};
