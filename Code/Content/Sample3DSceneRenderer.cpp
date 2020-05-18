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
	// Load shaders
	m_shader = std::shared_ptr<ShaderProgramHLSL>(new ShaderProgramHLSL(m_deviceResources));
	auto createVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso").then([this](std::vector<byte>& fileData) {m_shader->SetVertexShader(fileData); });
	auto createPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso").then([this](std::vector<byte>& fileData) {m_shader->SetPixelShader(fileData); });

	// Setup shader programs
	auto createPipelineStateTask = (createPSTask && createVSTask).then([this]()
	{
		m_shader->Setup();
		m_shader->ClearShaders(); // no longer need the data
	});

	// Create and upload cube geometry resources to the GPU.
	auto createAssetsTask = createPipelineStateTask.then([this]()
	{
		std::shared_ptr<VertexPositionColor> pCubeVertices = Primitives::Cube::SetupCubeVertices(0.5f, 0.5f, 0.5f, Vec3f(1.0f, 0.0f, 0.0f));
		const UINT vertexBufferSize = Primitives::Cube::CubeVerticesArraySize * sizeof(VertexPositionColor);

		std::shared_ptr<unsigned short> pCubeIndices = Primitives::Cube::SetupCubeIndices();
		const UINT indexBufferSize = Primitives::Cube::CubeIndicesArraySize * sizeof(unsigned short);

		m_cube = std::shared_ptr<Model>(new Model(m_deviceResources, pCubeVertices, vertexBufferSize, pCubeIndices, indexBufferSize, m_shader));
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
			Mat4f newModelMatrix = m_cube->GetModelMatrix() * RotateY(degrees);
			m_cube->SetModelMatrix(newModelMatrix);
		}

		// Prepare to pass the updated model matrix to the shader.
		XMStoreFloat4x4(&m_constantBufferData.model, m_cube->GetModelMatrixDirectX());

		// Update view matrix
		XMStoreFloat4x4(&m_constantBufferData.view, m_camera->GetViewMatrixDirectX());

		// Update the constant buffer resource.
		UINT8* destination = m_cube->GetMappedConstantBuffer() + (m_deviceResources->GetCurrentFrameIndex() * c_alignedConstantBufferSize);
		memcpy(destination, &m_constantBufferData, sizeof(m_constantBufferData));
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

	m_cube->Render(&m_scissorRect);

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_cube->GetCommandList() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return true;
}
