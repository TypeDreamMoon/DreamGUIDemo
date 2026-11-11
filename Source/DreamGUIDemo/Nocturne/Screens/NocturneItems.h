// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What the NOCTURNE//OS archive holds: one table for the archive's grid, its detail panel, the item wheel and the
 * tooltip. Every item is drawn as a glyph in a frame, coloured by its rarity -- no pictures.
 */
enum class ENocturneItemKind : uint8 { Gear, Material, Record };

struct FNocturneItemDef
{
	const TCHAR* Glyph;
	const TCHAR* Name;
	const TCHAR* En;
	ENocturneItemKind Kind;
	/** 0 common, 1 rare, 2 legendary. */
	int32 Rarity;
	int32 Count;
	const TCHAR* Description;
	/** Where it came from, shown in the detail. */
	const TCHAR* Source;
};

namespace NocturneItems
{
	inline const FNocturneItemDef Items[] = {
		{ TEXT("镜"), TEXT("棱镜碎片"), TEXT("PRISM SHARD"), ENocturneItemKind::Material, 1, 3, TEXT("折射过星光的碎片，握在手里仍有一点余温。"), TEXT("第六象限 · 漂流物") },
		{ TEXT("钥"), TEXT("旧港钥匙"), TEXT("HARBOR KEY"), ENocturneItemKind::Record, 0, 1, TEXT("一把铜钥匙，齿纹磨得很浅。港口早已没有门了。"), TEXT("旧港 · 灯塔下") },
		{ TEXT("叉"), TEXT("调音音叉"), TEXT("TUNING FORK"), ENocturneItemKind::Gear, 0, 1, TEXT("敲响后能让附近的信标对齐频率。"), TEXT("弦 · 维修舱") },
		{ TEXT("灯"), TEXT("航标灯芯"), TEXT("BEACON CORE"), ENocturneItemKind::Gear, 1, 1, TEXT("放进任何一座熄灭的航标，它会重新亮一夜。"), TEXT("启明 · 补给") },
		{ TEXT("卷"), TEXT("星图残卷"), TEXT("CHART FRAGMENT"), ENocturneItemKind::Record, 1, 1, TEXT("一角残缺的星图，标着一条没有名字的航线。"), TEXT("匣 · 第三层") },
		{ TEXT("晶"), TEXT("夜晶"), TEXT("NIGHT CRYSTAL"), ENocturneItemKind::Material, 0, 12, TEXT("只在夜里结晶的矿物，天亮前要收好。"), TEXT("各处 · 采集") },
		{ TEXT("羽"), TEXT("白羽信标"), TEXT("FEATHER BEACON"), ENocturneItemKind::Gear, 2, 1, TEXT("轻得像羽毛的信标，能把位置告诉很远的人。"), TEXT("未知") },
		{ TEXT("匣"), TEXT("八音盒机芯"), TEXT("MUSIC MOVEMENT"), ENocturneItemKind::Record, 0, 1, TEXT("上满发条能放一支夜曲，最后一个音总是走调。"), TEXT("吟 · 音乐盒") },
		{ TEXT("砂"), TEXT("陨砂"), TEXT("METEOR SAND"), ENocturneItemKind::Material, 0, 30, TEXT("细得能从指缝漏下去的黑砂，带一点磁性。"), TEXT("各处 · 采集") },
		{ TEXT("环"), TEXT("共振环"), TEXT("RESONANCE RING"), ENocturneItemKind::Gear, 1, 1, TEXT("戴上后能听见附近星体低沉的振动。"), TEXT("轮 · 交换") },
		{ TEXT("信"), TEXT("未寄出的信"), TEXT("UNSENT LETTER"), ENocturneItemKind::Record, 1, 1, TEXT("信封上只写了收件人的星位，没有名字。"), TEXT("笺 · 抽屉") },
		{ TEXT("针"), TEXT("罗盘针"), TEXT("COMPASS NEEDLE"), ENocturneItemKind::Gear, 0, 1, TEXT("永远指向最近的一颗主星。"), TEXT("旧港 · 集市") },
		{ TEXT("露"), TEXT("冷凝露"), TEXT("CONDENSATE"), ENocturneItemKind::Material, 0, 6, TEXT("舷窗上收集的露水，可以补充冷却液。"), TEXT("各处 · 采集") },
		{ TEXT("印"), TEXT("管理员印章"), TEXT("ADMIN SEAL"), ENocturneItemKind::Record, 2, 1, TEXT("刻着 0721 的印章。系统里的每一扇门都认得它。"), TEXT("管理员") },
		{ TEXT("丝"), TEXT("银丝"), TEXT("SILVER THREAD"), ENocturneItemKind::Material, 0, 20, TEXT("修补帆片用的细丝。"), TEXT("各处 · 采集") },
		{ TEXT("鳞"), TEXT("星鳞"), TEXT("STAR SCALE"), ENocturneItemKind::Material, 1, 4, TEXT("从流星尾迹里拾来的鳞片，背面有细小的纹路。"), TEXT("第二象限 · 流星雨") },
		{ TEXT("帆"), TEXT("太阳帆片"), TEXT("SOLAR SAIL"), ENocturneItemKind::Material, 0, 2, TEXT("薄得透光的帆片，展开足有一个房间大。"), TEXT("启明 · 补给") },
		{ TEXT("钟"), TEXT("怀表"), TEXT("POCKET WATCH"), ENocturneItemKind::Gear, 1, 1, TEXT("停在 23:41。表盖里刻着一行字，已经看不清了。"), TEXT("未知") },
	};
	constexpr int32 Count = UE_ARRAY_COUNT(Items);

	inline const TCHAR* KindName(ENocturneItemKind Kind)
	{
		switch (Kind)
		{
		case ENocturneItemKind::Gear: return TEXT("装备");
		case ENocturneItemKind::Material: return TEXT("素材");
		default: return TEXT("记录");
		}
	}

	inline const TCHAR* KindTag(ENocturneItemKind Kind)
	{
		switch (Kind)
		{
		case ENocturneItemKind::Gear: return TEXT("GEAR");
		case ENocturneItemKind::Material: return TEXT("MATERIAL");
		default: return TEXT("RECORD");
		}
	}

	inline const TCHAR* RarityName(int32 Rarity)
	{
		return Rarity >= 2 ? TEXT("传说") : (Rarity == 1 ? TEXT("稀有") : TEXT("普通"));
	}

	inline FColor RarityColor(int32 Rarity)
	{
		return Rarity >= 2 ? FColor(0xFF, 0xB4, 0x54) : (Rarity == 1 ? FColor(0x8F, 0xE3, 0xFF) : FColor(0x8A, 0x98, 0xA6));
	}
}
