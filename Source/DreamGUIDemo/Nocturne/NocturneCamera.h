// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"

namespace NocturneCamera
{
	constexpr float FieldOfView = 46.0f;

	/** Every camera that looks at NOCTURNE//OS's space looks the same way: the lens, a fixed exposure, the bloom. */
	inline void Configure(UCameraComponent* Camera)
	{
		Camera->SetFieldOfView(FieldOfView);
		// A fixed exposure: nothing in this space should change brightness as the camera turns.
		FPostProcessSettings& Post = Camera->PostProcessSettings;
		Post.bOverride_AutoExposureMethod = true;
		Post.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		Post.bOverride_AutoExposureBias = true;
		Post.AutoExposureBias = 0.0f;
		Post.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		Post.AutoExposureApplyPhysicalCameraExposure = false;
		Post.bOverride_BloomIntensity = true;
		Post.BloomIntensity = 0.9f;
		Post.bOverride_MotionBlurAmount = true;
		Post.MotionBlurAmount = 0.0f;
		Post.bOverride_VignetteIntensity = true;
		Post.VignetteIntensity = 0.35f;
	}
}
