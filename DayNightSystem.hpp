#pragma once
#include <Siv3D.hpp>

class DayNightSystem
{
public:
	enum class TimePhase
	{
		Day,        // 昼（0.0～0.5）
		Sunset,     // 夕暮れ（0.5～0.65）
		Night,      // 夜（0.65～0.85）
		Dawn        // 夜明け（0.85～1.0）
	};

private:
	double m_currentTime;
	double m_dayDuration;
	double m_nightStartTime;
	double m_timeSpeed;
	int m_starsCollected;
	TimePhase m_currentPhase;
	bool m_isPaused;
	double m_phaseTransitionTimer;
	double m_enemyAggressionLevel;

	// シェーダー関連
	PixelShader m_dayNightShader;
	struct DayNightParams
	{
		float timeOfDay;
		float moonlightIntensity;
		float phaseBlend;
		float starsBonus;
	};
	ConstantBuffer<DayNightParams> m_shaderParams;

	// ★ 新規追加: UI関連メンバー
	Texture m_gaugeTexture;
	Font m_uiFont;
	Font m_smallFont;

	// 定数
	static constexpr double BASE_DAY_DURATION = 60.0;
	static constexpr double STAR_TIME_BONUS = 10.0;
	static constexpr double STAR_NIGHT_DELAY = 5.0;

public:
	DayNightSystem();

	void init();
	void update();
	void applyShader(const RenderTexture& source) const;

	// スター収集による時間制御
	void onStarCollected();
	void resetStars() { m_starsCollected = 0; updateDayDuration(); }

	// 時間制御
	void pauseTime(bool pause) { m_isPaused = pause; }
	void setTimeOfDay(double time) { m_currentTime = Math::Clamp(time, 0.0, m_dayDuration); }
	void setTimeSpeed(double speed) { m_timeSpeed = Math::Max(0.0, speed); }

	// 状態取得
	bool isNight() const { return m_currentPhase == TimePhase::Night; }
	bool isDangerous() const { return isNight() || m_currentPhase == TimePhase::Sunset; }
	double getTimeOfDay() const { return m_currentTime; }
	double getNormalizedTime() const { return m_currentTime / m_dayDuration; }
	TimePhase getCurrentPhase() const { return m_currentPhase; }
	int getStarsCollected() const { return m_starsCollected; }
	double getTimeUntilNight() const;

	// 敵への影響
	double getEnemySpeedMultiplier() const;
	double getEnemyAggressionMultiplier() const;
	ColorF getEnemyGlowColor() const;

	// 環境効果
	ColorF getAmbientLight() const;
	String getPhaseText() const;
	ColorF getPhaseColor() const;

	// ★ 新規追加: UI描画メソッド
	void drawTimeGaugeUI(const Vec2& position) const;
	void drawTimeInfoUI(const Vec2& position) const;

private:
	void updateDayDuration();
	void updatePhase();
	void updatePhaseTransition();
	void updateEnemyAggression();
	void updateShaderParams();
	double calculateMoonlight() const;
};
