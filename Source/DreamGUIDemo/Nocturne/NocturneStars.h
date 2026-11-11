// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The six stars of the NOCTURNE//OS constellation -- the main menu -- in the order the menu lists them. One table for the
 * space (where they are), the labels (what they say) and the menu (what they open).
 *
 * The places are chosen from the overview camera (NocturneStars::OverviewCamera, looking along +X with a 46 degree field
 * of view at 1920 x 1080): each star lands on a planned spot of the screen, its label to its right, so that no two labels
 * overlap -- folded or unfolded, plates included -- and none runs under the top bar, the key bar or the route list in the
 * lower left. A label measures about 1 px a design unit from here: the plate runs from 56 to ~300 right of the star and
 * from 66 above to 42 below it, 102 below while the description shows. The spot each was placed for is in the comment;
 * move a star and check the labels again.
 */
struct FNocturneStarDef
{
	FVector Location;
	const TCHAR* Glyph;
	const TCHAR* Name;
	const TCHAR* En;
	const TCHAR* Description;
	/** Whether the star opens a screen yet; the others say so instead of flying anywhere. */
	bool bOpen;
	/** The widget Blueprint of the star's screen, under /Game/Nocturne/UI. */
	const TCHAR* Screen;
	/** Seen close up from straight on, the star in the middle of the frame (a screen built round it); else low on the left. */
	bool bVisitCentered;
};

namespace NocturneStars
{
	constexpr int32 Count = 6;

	inline const FNocturneStarDef Stars[Count] = {
		/* screen ( 520, 274) */ { FVector(-50.0, -330.0, 200.0), TEXT("启明"), TEXT("出发"), TEXT("DEPART"), TEXT("继续上一次的航程，或开始新的旅程。"), true, TEXT("WBP_Nocturne_Depart"), false },
		/* screen ( 768, 473) */ { FVector(20.0, -150.0, 52.0), TEXT("弦"), TEXT("设置"), TEXT("SETTINGS"), TEXT("画面、声音、操作与语言。"), true, TEXT("WBP_Nocturne_Settings"), false },
		/* screen (1240, 330) */ { FVector(200.0, 241.0, 181.0), TEXT("匣"), TEXT("档案"), TEXT("ARCHIVE"), TEXT("收集到的物品与记录。"), true, TEXT("WBP_Nocturne_Archive"), false },
		/* screen (1450, 610) */ { FVector(100.0, 401.0, -57.0), TEXT("轮"), TEXT("道具轮盘"), TEXT("ITEM WHEEL"), TEXT("快捷道具的排列与切换。"), true, TEXT("WBP_Nocturne_Wheel"), true },
		/* screen (1120, 780) */ { FVector(300.0, 145.0, -217.0), TEXT("吟"), TEXT("音乐盒"), TEXT("MUSIC BOX"), TEXT("收集到的曲目与歌词。"), true, TEXT("WBP_Nocturne_MusicBox"), true },
		/* screen ( 700, 720) */ { FVector(-50.0, -195.0, -135.0), TEXT("笺"), TEXT("日志"), TEXT("LOG"), TEXT("任务、事件与留言。"), true, TEXT("WBP_Nocturne_Log"), false },
	};

	/** The constellation's lines, as pairs of star indices. */
	inline const int32 Lines[][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 1, 5 }, { 5, 4 }, { 4, 3 } };
	constexpr int32 LineCount = UE_ARRAY_COUNT(Lines);

	/** Where the overview camera stands; it looks along +X. */
	inline const FVector OverviewCamera(-1750.0, 0.0, 0.0);

	/** The stars by what they open. */
	constexpr int32 DepartStar = 0;
	constexpr int32 SettingsStar = 1;
	constexpr int32 ArchiveStar = 2;
	constexpr int32 WheelStar = 3;
	constexpr int32 MusicStar = 4;
	constexpr int32 LogStar = 5;
}
