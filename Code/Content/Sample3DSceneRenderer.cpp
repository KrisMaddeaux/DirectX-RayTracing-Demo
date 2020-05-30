#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"
#include <ppltasks.h>
#include <synchapi.h>

#include "Code/Primitives.h"
#include <stdlib.h>

using namespace DirectX_RayTracing_Demo;

using namespace Concurrency;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Storage;

// Indices into the application state map.
Platform::String^ AngleKey = "Angle";
Platform::String^ TrackingKey = "Tracking";

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources, std::shared_ptr<Camera> camera) :
	m_loadingComplete(false),
	m_radiansPerSecond(XM_PIDIV4),	// rotate 45 degrees per second
	m_deviceResources(deviceResources),
	m_camera(camera)
{
	LoadState();
	ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

Sample3DSceneRenderer::~Sample3DSceneRenderer()
{
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// Check Raytracing support
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
	{
		throw std::runtime_error("Raytracing not supported on device");
	}

	// Create a command list.
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_deviceResources->GetCommandAllocator(), NULL, IID_PPV_ARGS(&m_commandList)));
	NAME_D3D12_OBJECT(m_commandList);
	DX::ThrowIfFailed(m_commandList->Close());

	// Load shaders
	m_shaderRaster = std::shared_ptr<ShaderProgramHLSL>(new ShaderProgramHLSL(m_deviceResources, ShaderProgramHLSL::SHADER_PIPELINE_TYPE_RASTER));
	auto createVSTaskRaster = DX::ReadDataAsync(L"SampleVertexShader.cso").then([this](std::vector<byte>& fileData) {m_shaderRaster->SetVertexShader(fileData); });
	auto createPSTaskRaster = DX::ReadDataAsync(L"SamplePixelShader.cso").then([this](std::vector<byte>& fileData) {m_shaderRaster->SetPixelShader(fileData); });

	m_shaderRaytrace = std::shared_ptr<ShaderProgramHLSL>(new ShaderProgramHLSL(m_deviceResources, ShaderProgramHLSL::SHADER_PIPELINE_TYPE_RAYTRACE));
	auto createVSTaskRaytrace = DX::ReadDataAsync(L"SampleVertexShader.cso").then([this](std::vector<byte>& fileData) {m_shaderRaytrace->SetVertexShader(fileData); });
	auto createPSTaskRaytrace = DX::ReadDataAsync(L"SamplePixelShader.cso").then([this](std::vector<byte>& fileData) {m_shaderRaytrace->SetPixelShader(fileData); });

	// Setup shader programs
	auto createPipelineStateTask = (createVSTaskRaster && createPSTaskRaster && createVSTaskRaytrace && createPSTaskRaytrace).then([this]()
	{
		m_shaderRaster->Setup();
		m_shaderRaster->ClearShaders(); // no longer need the data

		m_shaderRaytrace->Setup();
		m_shaderRaytrace->ClearShaders(); // no longer need the data
	});

	// Create and upload cube geometry resources to the GPU.
	auto createAssetsTask = createPipelineStateTask.then([this]()
	{
		std::shared_ptr<VertexPositionColor> pCubeVertices = Primitives::Cube::SetupCubeVertices(0.25f, 0.25f, 0.25f, Vec3f(1.0f, 0.0f, 0.0f));
		const UINT vertexBufferSize = Primitives::Cube::CubeVerticesArraySize * sizeof(VertexPositionColor);

		std::shared_ptr<unsigned short> pCubeIndices = Primitives::Cube::SetupCubeIndices();
		const UINT indexBufferSize = Primitives::Cube::CubeIndicesArraySize * sizeof(unsigned short);

		m_cubeRaster = std::shared_ptr<Model>(new Model(m_deviceResources, pCubeVertices, vertexBufferSize, pCubeIndices, indexBufferSize, m_shaderRaster));
		m_cubeRaster->SetModelMatrix(m_cubeRaster->GetModelMatrix() * Translate(1.0f, 0.0f, 0.0f));

		m_cubeRayTrace = std::shared_ptr<Model>(new Model(m_deviceResources, pCubeVertices, vertexBufferSize, pCubeIndices, indexBufferSize, m_shaderRaster));
		m_cubeRayTrace->SetModelMatrix(m_cubeRayTrace->GetModelMatrix() * Translate(-1.0f, 0.0f, 0.0f));
	});

	createAssetsTask.then([this]()
	{
		m_loadingComplete = true;
	});
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
	m_scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height)};

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (m_loadingComplete)
	{
		// Rotate the cube a small amount.
		{
			float radians = static_cast<float>(timer.GetElapsedSeconds()) * m_radiansPerSecond;
			float degrees = RadiansToDegrees * radians;

			Mat4f newModelMatrixRaster = m_cubeRaster->GetModelMatrix() * RotateY(degrees);
			m_cubeRaster->SetModelMatrix(newModelMatrixRaster);

			Mat4f newModelMatrixRaytrace = m_cubeRayTrace->GetModelMatrix() * RotateY(degrees);
			m_cubeRayTrace->SetModelMatrix(newModelMatrixRaytrace);
		}

		// Update view matrix
		XMStoreFloat4x4(&m_constantBufferData.view, m_camera->GetViewMatrixDirectX());

		// Prepare to pass the updated model matrix to the shader.
		{
			XMStoreFloat4x4(&m_constantBufferData.model, m_cubeRaster->GetModelMatrixDirectX());

			// Update the constant buffer resource.
			UINT8* destination = m_cubeRaster->GetMappedConstantBuffer() + (m_deviceResources->GetCurrentFrameIndex() * c_alignedConstantBufferSize);
			memcpy(destination, &m_constantBufferData, sizeof(m_constantBufferData));
		}

		// Prepare to pass the updated model matrix to the shader.
		{
			XMStoreFloat4x4(&m_constantBufferData.model, m_cubeRayTrace->GetModelMatrixDirectX());

			// Update the constant buffer resource.
			UINT8* destination = m_cubeRayTrace->GetMappedConstantBuffer() + (m_deviceResources->GetCurrentFrameIndex() * c_alignedConstantBufferSize);
			memcpy(destination, &m_constantBufferData, sizeof(m_constantBufferData));
		}
	}
}

// Saves the current state of the renderer.
void Sample3DSceneRenderer::SaveState()
{
}

// Restores the previous state of the renderer.
void Sample3DSceneRenderer::LoadState()
{
}

// Renders one frame using the vertex and pixel shaders.
bool Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return false;
	}

	DX::ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());

	// Setup command list to clear render target and stencil buffer
	{
		DX::ThrowIfFailed(m_commandList->Reset(m_deviceResources->GetCommandAllocator(), NULL));

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_deviceResources->GetRenderTargetView();
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = m_deviceResources->GetDepthStencilView();
		m_commandList->ClearRenderTargetView(renderTargetView, DirectX::Colors::CornflowerBlue, 0, nullptr);
		m_commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		DX::ThrowIfFailed(m_commandList->Close());
	}

	// Render objects
	{
		m_cubeRaster->Render(&m_scissorRect);
		m_cubeRayTrace->Render(&m_scissorRect);
	}

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get(), m_cubeRaster->GetCommandList(), m_cubeRayTrace->GetCommandList() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return true;
}
