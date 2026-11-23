// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryModel.h"

// Tracks files that have been opened in Explorer++, mirroring the way HistoryModel tracks
// folders that have been navigated to. This is a distinct type (rather than a plain HistoryModel
// instance) so that file entries are stored and exposed separately from folder entries.
class FileHistoryModel : public HistoryModel
{
};
