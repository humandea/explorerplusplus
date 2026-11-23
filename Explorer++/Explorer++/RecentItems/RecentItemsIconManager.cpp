// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RecentItemsIconManager.h"
#include "IconFetcher.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"

RecentItemsIconManager::RecentItemsIconManager(IconFetcher *iconFetcher, int iconWidth,
	int iconHeight) :
	m_iconFetcher(iconFetcher),
	m_weakPtrFactory(this)
{
	m_imageList.reset(ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 0, 1));
	CHECK(m_imageList);

	SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList));

	// No path is supplied to the calls below, so the default icon for each type will be returned.
	m_defaultFolderIconSystemImageListIndex =
		m_iconFetcher->GetCachedIconIndexOrDefault(L"", DefaultIconType::Folder);
	m_defaultFileIconSystemImageListIndex =
		m_iconFetcher->GetCachedIconIndexOrDefault(L"", DefaultIconType::File);

	m_defaultFolderIconIndex = ImageHelper::CopyImageListIcon(m_imageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()),
		m_defaultFolderIconSystemImageListIndex);
	m_defaultFileIconIndex = ImageHelper::CopyImageListIcon(m_imageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()),
		m_defaultFileIconSystemImageListIndex);

	m_imageListHandle = m_imageList.get();
}

wil::unique_himagelist RecentItemsIconManager::TakeImageList()
{
	auto imageList = std::move(m_imageList);
	return imageList;
}

int RecentItemsIconManager::GetIconIndexForPath(const std::wstring &path, RecentItem::Kind kind,
	const IconAvailableCallback &callback)
{
	auto cachedIconIndex = m_iconFetcher->GetCachedIconIndex(path);

	if (cachedIconIndex)
	{
		return AddSystemIconToImageList(*cachedIconIndex, kind);
	}

	m_iconFetcher->QueueIconTask(path,
		[self = m_weakPtrFactory.GetWeakPtr(), kind, callback](int iconIndex, int overlayIndex)
		{
			UNREFERENCED_PARAMETER(overlayIndex);

			if (!self || !callback)
			{
				return;
			}

			if (iconIndex == self->GetDefaultSystemIconIndex(kind))
			{
				// The item is using the default icon for its type, so nothing else needs to
				// happen.
				return;
			}

			int copiedIconIndex = self->AddSystemIconToImageList(iconIndex, kind);
			callback(copiedIconIndex);
		});

	return GetDefaultIconIndex(kind);
}

int RecentItemsIconManager::GetDefaultIconIndex(RecentItem::Kind kind) const
{
	return kind == RecentItem::Kind::Folder ? m_defaultFolderIconIndex : m_defaultFileIconIndex;
}

std::optional<int> RecentItemsIconManager::MaybeGetCachedIconIndex(const std::wstring &path,
	RecentItem::Kind kind) const
{
	auto cachedIconIndex = m_iconFetcher->GetCachedIconIndex(path);

	if (!cachedIconIndex)
	{
		return std::nullopt;
	}

	return AddSystemIconToImageList(*cachedIconIndex, kind);
}

int RecentItemsIconManager::GetDefaultSystemIconIndex(RecentItem::Kind kind) const
{
	return kind == RecentItem::Kind::Folder ? m_defaultFolderIconSystemImageListIndex
											: m_defaultFileIconSystemImageListIndex;
}

int RecentItemsIconManager::AddSystemIconToImageList(int systemIconIndex,
	RecentItem::Kind kind) const
{
	if (systemIconIndex == GetDefaultSystemIconIndex(kind))
	{
		return GetDefaultIconIndex(kind);
	}

	return ImageHelper::CopyImageListIcon(m_imageListHandle,
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()), systemIconIndex);
}
