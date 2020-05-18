#pragma once
#include <memory>
#include <DirectXMath.h>
#include "Code/MathClass.h"
#include "Content/ShaderStructures.h"
#include "Common/DeviceResources.h"
#include "Code/ShaderProgramHLSL.h"
#include "Common/StepTimer.h"

class Model
{
public:
    Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, 
        std::shared_ptr<DirectX_RayTracing_Demo::VertexPositionColor> pVertices, 
        const int verticesArraySize,
        std::shared_ptr<unsigned short> pIndices, 
        const int indicesArraySize,
        std::shared_ptr<ShaderProgramHLSL> pShader);

    ~Model();

    void Render(D3D12_RECT* pScissorRect);

    void SetModelMatrix(Mat4f modelMatrix)
    {
        m_modelMatrix = modelMatrix;
    }

    Mat4f GetModelMatrix() const
    {
        return m_modelMatrix;
    }

    DirectX::XMMATRIX GetModelMatrixDirectX()
    {
        return XMMatrixTranspose(DirectX::XMMATRIX(m_modelMatrix.m));
    }

    ID3D12GraphicsCommandList* GetCommandList() const
    {
        return m_commandList.Get();
    }

    UINT8* GetMappedConstantBuffer()
    {
        return m_mappedConstantBuffer;
    }

private:
    // Constant buffers must be 256-byte aligned.
    static const UINT c_alignedConstantBufferSize = (sizeof(DirectX_RayTracing_Demo::ModelViewProjectionConstantBuffer) + 255) & ~255;

    std::shared_ptr<DX::DeviceResources> m_deviceResources;
    std::shared_ptr<ShaderProgramHLSL> m_shader;

    // Direct3D resources for geometry.
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_commandList;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_cbvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>				m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>				m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBuffer;
    UINT8* m_mappedConstantBuffer;
    UINT												m_cbvDescriptorSize;
    D3D12_VERTEX_BUFFER_VIEW							m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW								m_indexBufferView;

    Mat4f m_modelMatrix;
};

