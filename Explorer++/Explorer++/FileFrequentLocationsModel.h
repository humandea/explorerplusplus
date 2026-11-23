// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FrequentLocationsModel.h"

// Tracks how often files have been opened in Explorer++, mirroring the way FrequentLocationsModel
// tracks how often folders have been navigated to. This is a distinct type (rather than a plain
// FrequentLocationsModel instance) so that file entries are stored and exposed separately from
// folder entries.
class FileFrequentLocationsModel : public FrequentLocationsModel
{
public:
	FileFrequentLocationsModel(SystemClock *systemClock) : FrequentLocationsModel(systemClock)
	{
	}
};
