﻿#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"

#include "Camera.h"

// Renders Direct3D content on the screen.
namespace DirectX_RayTracing_Demo
{
	class DirectX_RayTracing_DemoMain
	{
	public:
		DirectX_RayTracing_DemoMain();
		void CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void Update();
		bool Render();

		void OnWindowSizeChanged();
		void OnSuspending();
		void OnResuming();
		void OnDeviceRemoved();

	private:
		// TODO: Replace with your own content renderers.
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
		std::shared_ptr<Camera> m_camera;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}