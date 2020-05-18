#pragma once
#include <memory>
#include <DirectXMath.h>
#include "Code/MathClass.h"
#include "Content/ShaderStructures.h"

namespace Primitives
{
	namespace Cube
	{
		static const int CubeVerticesArraySize = 8;
		static const int CubeIndicesArraySize = 36;

		static std::shared_ptr<DirectX_RayTracing_Demo::VertexPositionColor> SetupCubeVertices(float width, float height, float depth, Vec3f color)
		{
			DirectX_RayTracing_Demo::VertexPositionColor cubeVertices[CubeVerticesArraySize] =
			{
				{ DirectX::XMFLOAT3(-width, -height, -depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
				{ DirectX::XMFLOAT3(-width, -height,  depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
				{ DirectX::XMFLOAT3(-width,  height, -depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
				{ DirectX::XMFLOAT3(-width,  height,  depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
				{ DirectX::XMFLOAT3(width, -height, -depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
				{ DirectX::XMFLOAT3(width, -height,  depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
				{ DirectX::XMFLOAT3(width,  height, -depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
				{ DirectX::XMFLOAT3(width,  height,  depth), DirectX::XMFLOAT3(color.x, color.y, color.z) },
			};

			DirectX_RayTracing_Demo::VertexPositionColor* pCubeVertices = new DirectX_RayTracing_Demo::VertexPositionColor[CubeVerticesArraySize];

			for (int i = 0; i < CubeVerticesArraySize; i++)
			{
				pCubeVertices[i] = cubeVertices[i];
			}

			return std::shared_ptr<DirectX_RayTracing_Demo::VertexPositionColor>(pCubeVertices);
		}

		static std::shared_ptr<unsigned short> SetupCubeIndices()
		{
			unsigned short cubeIndices[CubeIndicesArraySize] =
			{
				0, 2, 1, // -x
				1, 2, 3,

				4, 5, 6, // +x
				5, 7, 6,

				0, 1, 5, // -y
				0, 5, 4,

				2, 6, 7, // +y
				2, 7, 3,

				0, 4, 6, // -z
				0, 6, 2,

				1, 3, 7, // +z
				1, 7, 5,
			};

			unsigned short* pCubeIndices = new unsigned short[CubeIndicesArraySize];

			for (int i = 0; i < CubeIndicesArraySize; i++)
			{
				pCubeIndices[i] = cubeIndices[i];
			}

			return std::shared_ptr<unsigned short>(pCubeIndices);
		}
	}
}
