#pragma once
#include "Common/DeviceResources.h"
#include "Content/ShaderStructures.h"
#include<vector>
#include<rpcndr.h>
#include<string>

//Helper class to allow easy creation and use of HLSL shaders
class ShaderProgramHLSL
{
public:
	enum ShaderPipelineType
	{
		SHADER_PIPELINE_TYPE_RASTER,
		SHADER_PIPELINE_TYPE_RAYTRACE
	};

	ShaderProgramHLSL(const std::shared_ptr<DX::DeviceResources>& deviceResources, ShaderPipelineType shaderPipelineType); //Constructor
	
	void Setup();

	ID3D12RootSignature* GetRootSignature() const
	{
		return m_rootSignature.Get();
	}

	ID3D12PipelineState* GetPipelineState() const
	{
		return m_pipelineState.Get();
	}

	ShaderPipelineType GetShaderPipelineType()
	{
		return m_shaderPipelineType;
	}

	void SetVertexShader(std::vector<byte>& vertexShader)
	{
		m_vertexShader = vertexShader;
	}

	void SetPixelShader(std::vector<byte>& pixelShader)
	{
		m_pixelShader = pixelShader;
	}

	void ClearShaders()
	{
		m_vertexShader.clear();
		m_pixelShader.clear();
	}

private:
	std::string m_vertexShaderName;
	std::string m_pixelShaderName;
	std::vector<byte> m_vertexShader;
	std::vector<byte> m_pixelShader;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	ShaderPipelineType m_shaderPipelineType;
};
