#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

#include "Code/Camera.h"
#include "Code/ShaderProgramHLSL.h"
#include "Code/Model.h"

namespace DirectX_RayTracing_Demo
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources, std::shared_ptr<Camera> camera);
		~Sample3DSceneRenderer();
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void Update(DX::StepTimer const& timer);
		bool Render();
		void SaveState();

	private:
		void LoadState();

	private:
		// Constant buffers must be 256-byte aligned.
		static const UINT c_alignedConstantBufferSize = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		std::shared_ptr<Camera> m_camera;

		std::shared_ptr<ShaderProgramHLSL> m_shader;

		std::shared_ptr<Model> m_cube;

		// Direct3D resources for cube geometry.
		ModelViewProjectionConstantBuffer					m_constantBufferData;
		D3D12_RECT											m_scissorRect;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_radiansPerSecond;
	};
}

