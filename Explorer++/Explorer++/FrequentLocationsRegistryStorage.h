// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class FrequentLocationsModel;

namespace FrequentLocationsRegistryStorage
{

void Load(HKEY applicationKey, FrequentLocationsModel *model);
void Save(HKEY applicationKey, const FrequentLocationsModel *model);

// The file variants use the same layout as the functions above, but store their data under a
// separate key, so that file entries are kept apart from folder entries.
void LoadFiles(HKEY applicationKey, FrequentLocationsModel *model);
void SaveFiles(HKEY applicationKey, const FrequentLocationsModel *model);

}
