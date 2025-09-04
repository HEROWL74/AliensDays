#pragma once
#include <Siv3D.hpp>

class ShaderEffects
{
private:
	RenderTexture m_renderTexture;
	PixelShader m_glowShader;
	PixelShader m_waveShader;
	PixelShader m_chromaticShader;
	PixelShader m_shockwaveShader;

	struct WaveParams
	{
		float time = 0.0f;
		float amplitude = 10.0f;
		float frequency = 20.0f;
		float speed = 3.0f;
	};

	struct ChromaticParams
	{
		float intensity = 0.5f;
		float radialStrength = 0.5f;
		float padding[2] = {};
	};

	struct ShockwaveParams
	{
		Float2 center = { 0.5f, 0.5f };
		float radius = 0.0f;
		float thickness = 0.05f;
		float force = 0.1f;
		float padding[3] = {};
	};

	ConstantBuffer<WaveParams> m_waveBuffer;
	ConstantBuffer<ChromaticParams> m_chromaticBuffer;
	ConstantBuffer<ShockwaveParams> m_shockwaveBuffer;

	bool m_glowActive = false;
	bool m_waveActive = false;
	bool m_chromaticActive = false;
	bool m_shockwaveActive = false;
	bool m_isCapturing = false;

	double m_shockwaveTime = 0.0;
	static constexpr double SHOCKWAVE_DURATION = 1.0;

public:
	void init()
	{
		m_renderTexture = RenderTexture(Scene::Size());
		m_glowShader = HLSL(U"Shaders/Glow.hlsl", U"PS");
		m_waveShader = HLSL(U"Shaders/Wave.hlsl", U"PS");
		m_chromaticShader = HLSL(U"Shaders/ChromaticAberration.hlsl", U"PS");
		m_shockwaveShader = HLSL(U"Shaders/Shockwave.hlsl", U"PS");

		if (!m_glowShader) Print << U"Failed to load Glow shader";
		if (!m_waveShader) Print << U"Failed to load Wave shader";
		if (!m_chromaticShader) Print << U"Failed to load Chromatic shader";
		if (!m_shockwaveShader) Print << U"Failed to load Shockwave shader";
	}

	void beginCapture()
	{
		m_renderTexture.clear(Scene::GetBackground());
		m_isCapturing = true;
	}

	RenderTexture& getRenderTarget()
	{
		return m_renderTexture;
	}

	void endCaptureAndDraw()
	{
		m_isCapturing = false;

		if (m_shockwaveActive && m_shockwaveShader)
		{
			applyShockwave();
		}
		else if (m_glowActive && m_glowShader)
		{
			applyGlow();
		}
		else if (m_chromaticActive && m_chromaticShader)
		{
			applyChromaticAberration();
		}
		else if (m_waveActive && m_waveShader)
		{
			applyWave();
		}
		else
		{
			m_renderTexture.draw();
		}
	}

	void enableGlow(bool enable) { m_glowActive = enable; }
	void enableWave(bool enable) { m_waveActive = enable; }
	void enableChromatic(bool enable) { m_chromaticActive = enable; }

	void triggerShockwave(const Vec2& worldPos, const Vec2& cameraOffset)
	{
		if (!m_shockwaveShader) return;

		const Vec2 screenPos = worldPos - cameraOffset;
		m_shockwaveActive = true;
		m_shockwaveTime = 0.0;

		m_shockwaveBuffer->center = Float2{
	static_cast<float>(screenPos.x / Scene::Width()),
	static_cast<float>(screenPos.y / Scene::Height())
		};
		m_shockwaveBuffer->radius = 0.0f;
		m_shockwaveBuffer->thickness = 0.1f;
		m_shockwaveBuffer->force = 0.3f;
	}


	void update(double deltaTime)
	{
		if (m_waveActive)
		{
			m_waveBuffer->time = static_cast<float>(Scene::Time());
			m_waveBuffer->amplitude = 10.0f;
			m_waveBuffer->frequency = 20.0f;
			m_waveBuffer->speed = 3.0f;
		}

		if (m_shockwaveActive)
		{
			m_shockwaveTime += deltaTime;
			float progress = static_cast<float>(m_shockwaveTime / SHOCKWAVE_DURATION);

			if (progress >= 1.0f)
			{
				m_shockwaveActive = false;
			}
			else
			{
				m_shockwaveBuffer->radius = progress * 0.5f;
				m_shockwaveBuffer->thickness = 0.05f * (1.0f - progress * 0.5f);
				m_shockwaveBuffer->force = 0.15f * (1.0f - progress);
			}
		}
	}

	void setChromaticIntensity(float intensity)
	{
		m_chromaticBuffer->intensity = intensity;
		m_chromaticBuffer->radialStrength = intensity * 0.5f;
	}

private:
	void applyGlow()
	{
		const ScopedCustomShader2D shader(m_glowShader);
		m_renderTexture.draw();
	}

	void applyWave()
	{
		Graphics2D::SetConstantBuffer(ShaderStage::Pixel, 1, m_waveBuffer); 
		const ScopedCustomShader2D shader(m_waveShader);
		m_renderTexture.draw();
	}

	void applyChromaticAberration()
	{
		Graphics2D::SetConstantBuffer(ShaderStage::Pixel, 1, m_chromaticBuffer);
		const ScopedCustomShader2D shader(m_chromaticShader);
		m_renderTexture.draw();
	}

	void applyShockwave()
	{
		Graphics2D::SetConstantBuffer(ShaderStage::Pixel, 1, m_shockwaveBuffer);
		const ScopedCustomShader2D shader(m_shockwaveShader);
		m_renderTexture.draw();
	}
};
