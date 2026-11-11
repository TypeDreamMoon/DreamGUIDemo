// Detroit: Become Human-style UI test for DreamGUI: a loading screen, a small crime scene with an android to walk, and
// the investigation UI in 3D space -- clue tags, the HIT MARKS analysis panel and the objectives float in the room,
// drawn by DreamGUI's world-space canvases -- over an analysis mode made in post-process. The materials are DreamShader
// sources (DShader/UITests/Detroit/Materials), the widgets .dui files (DUI/UITests/Detroit); this is what they share.

#pragma once

#include "CoreMinimal.h"

namespace DetroitUI
{
	/** The palette, sRGB, measured from reference screenshots. */
	/** The analysis UI's blue: titles, completed bars. */
	inline const FColor Cyan(58, 168, 240);
	/** "x/5 CLUES TO ANALYZE", the analysis bar and its first stage. */
	inline const FColor Orange(255, 74, 16);
	/** Text on the dark plates. */
	inline const FColor White(242, 245, 248);
	inline const FColor Dim(150, 160, 172);
	/** The clue markers and the panels' corner brackets. */
	inline const FColor Yellow(242, 180, 0);

	/** Where the generated classes and materials live (DUI/UITests/Detroit/_Tools/make_detroit_assets.py makes them). */
	inline const TCHAR* LoadingClass = TEXT("/Game/UITests/Detroit/UI/Detroit_Loading.Detroit_Loading_C");
	inline const TCHAR* HUDClass = TEXT("/Game/UITests/Detroit/UI/Detroit_HUD.Detroit_HUD_C");
	inline const TCHAR* ClueTagClass = TEXT("/Game/UITests/Detroit/UI/Components/Detroit_ClueTag.Detroit_ClueTag_C");
	inline const TCHAR* AnalysisPanelClass = TEXT("/Game/UITests/Detroit/UI/Components/Detroit_HitMarks.Detroit_HitMarks_C");
	inline const TCHAR* ObjectivesClass = TEXT("/Game/UITests/Detroit/UI/Components/Detroit_Objectives.Detroit_Objectives_C");
	inline const TCHAR* JacketClass = TEXT("/Game/UITests/Detroit/UI/Components/Detroit_Jacket.Detroit_Jacket_C");
	inline const TCHAR* WallWritingClass = TEXT("/Game/UITests/Detroit/UI/Components/Detroit_WallWriting.Detroit_WallWriting_C");

	inline const TCHAR* LitMaterial = TEXT("/Game/UITests/Detroit/Materials/M_DBH_Lit.M_DBH_Lit");
	inline const TCHAR* TilesMaterial = TEXT("/Game/UITests/Detroit/Materials/M_DBH_Tiles.M_DBH_Tiles");
	inline const TCHAR* HoloMeshMaterial = TEXT("/Game/UITests/Detroit/Materials/M_DBH_HoloMesh.M_DBH_HoloMesh");
	inline const TCHAR* ThiriumMaterial = TEXT("/Game/UITests/Detroit/Materials/M_DBH_Thirium.M_DBH_Thirium");
	inline const TCHAR* AnalysisMaterial = TEXT("/Game/UITests/Detroit/Materials/PP_DBH_Analysis.PP_DBH_Analysis");

	inline const TCHAR* CubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");
	inline const TCHAR* CylinderMesh = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	inline const TCHAR* SphereMesh = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	inline const TCHAR* ConeMesh = TEXT("/Engine/BasicShapes/Cone.Cone");
	inline const TCHAR* PlaneMesh = TEXT("/Engine/BasicShapes/Plane.Plane");

	/** An sRGB colour as the linear colour a material parameter takes. */
	inline FLinearColor Linear(const FColor& Color, float Alpha = 1.0f)
	{
		FLinearColor Result(Color);
		Result.A = Alpha;
		return Result;
	}
}
