/*
Copyright(c) 2016-2018 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES ===========================
#include <memory>
#include <vector>
#include "../RHI/IRHI_Definition.h"
#include "../Math/Matrix.h"
#include "../Resource/ResourceManager.h"
//======================================

namespace Directus
{
	class Actor;
	class Camera;
	class Skybox;
	class LineRenderer;
	class Light;
	class GBuffer;
	class Rectangle;
	class LightShader;
	class ResourceManager;
	class Font;
	class Grid;
	class Variant;	

	namespace Math
	{
		class Frustum;
	}

	enum RenderMode : unsigned long
	{
		Render_Albedo				= 1UL << 0,
		Render_Normal				= 1UL << 1,
		Render_Specular				= 1UL << 2,
		Render_Depth				= 1UL << 3,	
		Render_Physics				= 1UL << 4,
		Render_AABB					= 1UL << 5,
		Render_PickingRay			= 1UL << 6,
		Render_SceneGrid			= 1UL << 7,
		Render_PerformanceMetrics	= 1UL << 8,
		Render_Light				= 1UL << 9,
		Render_Bloom				= 1UL << 10,
		Render_FXAA					= 1UL << 11,
		Render_Sharpening			= 1UL << 12,
		Render_ChromaticAberration	= 1UL << 13,
		Render_Correction			= 1UL << 14, // Tone-mapping & Gamma correction
	};

	class ENGINE_CLASS Renderer : public Subsystem
	{
	public:
		Renderer(Context* context);
		~Renderer();

		//= Subsystem ============
		bool Initialize() override;
		//========================

		// Rendering
		void SetRenderTarget(std::shared_ptr<RHI_RenderTexture>& renderTarget, bool clear = true);
		void SetRenderTarget(bool clear = true);
		void* GetFrame();
		void Present();
		void Render();

		// The back-buffer is the final output (should match the display size)
		void SetBackBufferSize(int width, int height);
		const IRHI_Viewport& GetViewportBackBuffer();

		// The actual frame that all rendering takes place (or the viewport window in the editor)
		void SetResolution(int width, int height);
		const Math::Vector2& GetViewportInternal();

		//= RENDER MODE ======================================================================
		// Returns all render mode flags
		static unsigned long RenderFlags_GetAll()					{ return m_flags; }
		// Set's all render mode flags
		static void RenderFlags_SetAll(unsigned long renderFlags)	{ m_flags = renderFlags; }
		// Enables an render mode flag
		static void RenderFlags_Enable(RenderMode flag)				{ m_flags |= flag; }
		// Removes an render mode flag
		static void RenderFlags_Disable(RenderMode flag)			{ m_flags &= ~flag; }
		// Returns whether render mode flag is set
		static bool RenderFlags_IsSet(RenderMode flag)				{ return m_flags & flag; }
		//====================================================================================

		void Clear();
		const std::vector<Actor*>& GetRenderables() { return m_renderables; }

	private:
		void RenderTargets_Create(int width, int height);

		void Renderables_Acquire(const Variant& renderables);
		void Renderables_Sort(std::vector<Actor*>* renderables);

		void Pass_DepthDirectionalLight(Light* directionalLight);
		void Pass_GBuffer();

		void Pass_PreLight(
			void* inTextureNormal,
			void* inTextureDepth,
			std::shared_ptr<RHI_Texture>& inTextureNormalNoise,
			std::shared_ptr<RHI_RenderTexture>& inRenderTexure,
			std::shared_ptr<RHI_RenderTexture>& outRenderTextureShadowing
		);

		void Pass_Light(
			std::shared_ptr<RHI_RenderTexture>& inTextureShadowing,
			std::shared_ptr<RHI_RenderTexture>& outRenderTexture
		);	

		void Pass_PostLight(
			std::shared_ptr<RHI_RenderTexture>& inRenderTexture1,
			std::shared_ptr<RHI_RenderTexture>& inRenderTexture2,
			std::shared_ptr<RHI_RenderTexture>& outRenderTexture
		);

		bool Pass_DebugGBuffer();
		void Pass_Debug();

		void Pass_Correction(
			std::shared_ptr<RHI_RenderTexture>& inTexture,
			std::shared_ptr<RHI_RenderTexture>& outTexture
		);

		void Pass_FXAA(
			std::shared_ptr<RHI_RenderTexture>& inTexture,
			std::shared_ptr<RHI_RenderTexture>& outTexture
		);

		void Pass_Sharpening(
			std::shared_ptr<RHI_RenderTexture>& inTexture,
			std::shared_ptr<RHI_RenderTexture>& ouTexture
		);

		void Pass_ChromaticAberration(
			std::shared_ptr<RHI_RenderTexture>& inTexture,
			std::shared_ptr<RHI_RenderTexture>& outTexture
		);

		void Pass_Bloom(
			std::shared_ptr<RHI_RenderTexture>& inRenderTexture1,
			std::shared_ptr<RHI_RenderTexture>& inRenderTexture2,
			std::shared_ptr<RHI_RenderTexture>& outRenderTexture
		);

		void Pass_Blur(
			std::shared_ptr<RHI_RenderTexture>& texture,
			std::shared_ptr<RHI_RenderTexture>& renderTarget,
			const Math::Vector2& blurScale
		);

		void Pass_Shadowing(
			void* inTextureNormal_shaderResource,
			void* inTextureDepth_shaderResource,
			std::shared_ptr<RHI_Texture>& inTextureNormalNoise,
			Light* inDirectionalLight,
			std::shared_ptr<RHI_RenderTexture>& outRenderTexture
		);

		const Math::Vector4& GetClearColor();

		std::unique_ptr<GBuffer> m_gbuffer;

		// actorS ========================
		std::vector<Actor*> m_renderables;
		std::vector<Light*> m_lights;
		Light* m_directionalLight{};
		//=====================================

		//= RENDER TEXTURES ====================================
		std::shared_ptr<RHI_RenderTexture> m_renderTexPing;
		std::shared_ptr<RHI_RenderTexture> m_renderTexPing2;
		std::shared_ptr<RHI_RenderTexture> m_renderTexShadowing;
		std::shared_ptr<RHI_RenderTexture> m_renderTexPong;
		//======================================================

		//= SHADERS ============================================
		std::shared_ptr<LightShader> m_shaderLight;
		std::shared_ptr<IRHI_Shader> m_shaderLightDepth;
		std::shared_ptr<IRHI_Shader> m_shaderLine;
		std::shared_ptr<IRHI_Shader> m_shaderGrid;
		std::shared_ptr<IRHI_Shader> m_shaderFont;
		std::shared_ptr<IRHI_Shader> m_shaderTexture;
		std::shared_ptr<IRHI_Shader> m_shaderFXAA;
		std::shared_ptr<IRHI_Shader> m_shaderShadowing;
		std::shared_ptr<IRHI_Shader> m_shaderSharpening;
		std::shared_ptr<IRHI_Shader> m_shaderChromaticAberration;
		std::shared_ptr<IRHI_Shader> m_shaderBlurBox;
		std::shared_ptr<IRHI_Shader> m_shaderBlurGaussianH;
		std::shared_ptr<IRHI_Shader> m_shaderBlurGaussianV;
		std::shared_ptr<IRHI_Shader> m_shaderBloom_Bright;
		std::shared_ptr<IRHI_Shader> m_shaderBloom_BlurBlend;
		std::shared_ptr<IRHI_Shader> m_shaderCorrection;
		std::shared_ptr<IRHI_Shader> m_shaderTransformationGizmo;
		//======================================================

		//= SAMPLERS ===============================================
		std::shared_ptr<RHI_Sampler> m_samplerPointClampAlways;
		std::shared_ptr<RHI_Sampler> m_samplerPointClampGreater;
		std::shared_ptr<RHI_Sampler> m_samplerLinearClampGreater;
		std::shared_ptr<RHI_Sampler> m_samplerLinearClampAlways;
		std::shared_ptr<RHI_Sampler> m_samplerBilinearClampAlways;
		std::shared_ptr<RHI_Sampler> m_samplerAnisotropicWrapAlways;
		//==========================================================

		//= DEBUG ==============================================
		std::unique_ptr<Font> m_font;
		std::unique_ptr<Grid> m_grid;
		std::unique_ptr<RHI_Texture> m_gizmoTexLightDirectional;
		std::unique_ptr<RHI_Texture> m_gizmoTexLightPoint;
		std::unique_ptr<RHI_Texture> m_gizmoTexLightSpot;
		std::unique_ptr<Rectangle> m_gizmoRectLight;
		static unsigned long m_flags;
		//======================================================

		//= MISC ==================================
		std::vector<void*> m_texArray;
		ID3D11ShaderResourceView* m_texEnvironment;
		std::shared_ptr<RHI_Texture> m_texNoiseMap;
		std::unique_ptr<Rectangle> m_quad;
		//=========================================
		
		//= PREREQUISITES ====================================
		Camera* m_camera;
		Skybox* m_skybox;
		LineRenderer* m_lineRenderer;
		Math::Matrix m_mV;
		Math::Matrix m_mP_perspective;
		Math::Matrix m_mP_orthographic;
		Math::Matrix m_mV_base;
		Math::Matrix m_wvp_perspective;
		Math::Matrix m_wvp_baseOrthographic;
		float m_nearPlane;
		float m_farPlane;
		RHI_Device* m_rhiDevice;
		std::shared_ptr<IRHI_PipelineState> m_rhiPipelineState;
		//====================================================

		//= PIPELINE STATE ===================
		unsigned int m_currentlyBoundGeometry;
		unsigned int m_currentlyBoundShader;
		unsigned int m_currentlyBoundMaterial;
		//====================================
	};
}