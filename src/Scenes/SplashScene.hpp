#pragma once
#include <Siv3D.hpp>
#include "../Core/SceneBase.hpp"

class SplashScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_bombTexture;
	Texture m_bombActiveTexture;

	// フォント
	Font m_companyFont;
	Font m_poweredByFont;

	// アニメーション制御
	double m_timer;
	double m_totalDuration;

	// フェーズ管理
	enum class Phase
	{
		FadeIn,      // フェードイン
		BombIdle,    // 爆弾がアイドル状態
		BombActive,  // 爆弾が点滅
		Explosion,   // 爆発エフェクト
		TextReveal,  // テキスト表示
		Hold,        // 表示維持
		FadeOut      // フェードアウト
	};
	Phase m_currentPhase;

	// 強化されたパーティクルシステム
	Array<Vec2> m_particles;
	Array<ColorF> m_particleColors;
	Array<double> m_particleLifetime;
	Array<Vec2> m_particleVelocities;  // 速度ベクトル
	Array<double> m_particleSizes;     // パーティクルサイズ

	// 衝撃波エフェクト
	Array<Vec2> m_shockwaves;
	Array<double> m_shockwaveTimers;

	double m_explosionTimer;
	double m_textRevealTimer;
	bool m_skipRequested;

	Optional<SceneType> m_nextScene;

public:
	SplashScene();
	~SplashScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

private:
	// フェーズ更新メソッド
	void updateFadeIn();
	void updateBombIdle();
	void updateBombActive();
	void updateExplosion();
	void updateTextReveal();
	void updateHold();
	void updateFadeOut();

	// エフェクト関連
	void createExplosionParticles();
	void createShockwaves();
	void updateParticles();
	void updateShockwaves();
	void drawParticles() const;
	void drawShockwaves() const;

	// 描画メソッド
	void drawBomb(const Vec2& pos, double scale, double rotation = 0.0) const;
	void drawBombActive(const Vec2& pos, double scale, double rotation = 0.0) const;
	void drawCompanyText(double alpha) const;
	void drawPoweredByText(double alpha) const;

	// ユーティリティ
	double getPhaseProgress() const;
	Vec2 getBombPosition() const;
	double getBombScale() const;
	ColorF getBackgroundColor() const;
};
