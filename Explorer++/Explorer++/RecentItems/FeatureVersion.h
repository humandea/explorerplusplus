// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace FeatureVersion
{

// FORK-FEATURE-VERSION: BEGIN
//
// Fork-only version marker for the "recent items" pane feature, developed on the
// recent-items-pane branch. This version is tracked completely separately from the application
// version (which comes from VersionNumber.props) and is appended to the version line in the
// About dialog, so that local builds of the feature can be told apart.
//
// *** REMOVE BEFORE PUSHING TO UPSTREAM MASTER ***
//
// When the feature is ready to be pushed to the online master branch:
// 1. Delete this file.
// 2. Remove the append in AboutDialog::OnInitDialog() (marked with FORK-FEATURE-VERSION).
// 3. Remove the "Feature versioning" section in plan-dev\PROGRESS.md.
//
// Versioning policy: the major version starts at 2 (to keep it clearly distinct from the
// application version); the minor version is bumped on notable feature milestones (e.g. a
// completed phase) and the patch version on small fixes.
constexpr wchar_t DISPLAY_STRING[] = L"recent-items-pane v2.5.1";
// FORK-FEATURE-VERSION: END

}
