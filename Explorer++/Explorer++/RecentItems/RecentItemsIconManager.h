// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RecentItem.h"
#include "../Helper/WeakPtrFactory.h"
#include <wil/com.h>
#include <wil/resource.h>
#include <commoncontrols.h>
#include <functional>

class IconFetcher;

// Manages the image list used by the recent items tree. Icons are retrieved for individual items
// on a background thread and added to the image list as they become available.
class RecentItemsIconManager
{
public:
	using IconAvailableCallback = std::function<void(int iconIndex)>;

	RecentItemsIconManager(IconFetcher *iconFetcher, int iconWidth, int iconHeight);

	// Transfers ownership of the image list to the caller. The manager keeps a non-owning copy of
	// the handle, so that icons can still be added as they're fetched asynchronously. Because of
	// that, the caller is expected to keep the image list alive for as long as this manager is
	// alive.
	[[nodiscard]] wil::unique_himagelist TakeImageList();

	// Returns the index of the default icon for the specified type.
	int GetDefaultIconIndex(RecentItem::Kind kind) const;

	// Returns the index of the cached icon for the specified path, or nullopt if there is no
	// cached icon.
	std::optional<int> MaybeGetCachedIconIndex(const std::wstring &path,
		RecentItem::Kind kind) const;

	// Returns an icon index for the specified path. If the icon is cached, the returned index is
	// final. Otherwise, the index returned here is a default icon and the callback will be
	// invoked once the actual icon is available.
	int GetIconIndexForPath(const std::wstring &path, RecentItem::Kind kind,
		const IconAvailableCallback &callback = nullptr);

private:
	int GetDefaultSystemIconIndex(RecentItem::Kind kind) const;
	int AddSystemIconToImageList(int systemIconIndex, RecentItem::Kind kind) const;

	IconFetcher *const m_iconFetcher;

	wil::unique_himagelist m_imageList;
	HIMAGELIST m_imageListHandle = nullptr;

	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	int m_defaultFolderIconSystemImageListIndex;
	int m_defaultFileIconSystemImageListIndex;
	int m_defaultFolderIconIndex;
	int m_defaultFileIconIndex;

	WeakPtrFactory<RecentItemsIconManager> m_weakPtrFactory{ this };
};
