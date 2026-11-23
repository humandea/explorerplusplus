// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

class FrequentLocationsModel;

namespace FrequentLocationsXmlStorage
{

void Load(IXMLDOMNode *rootNode, FrequentLocationsModel *model);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const FrequentLocationsModel *model);

// The file variants use the same layout as the functions above, but store their data under
// separate nodes, so that file entries are kept apart from folder entries.
void LoadFiles(IXMLDOMNode *rootNode, FrequentLocationsModel *model);
void SaveFiles(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode,
	const FrequentLocationsModel *model);

}
