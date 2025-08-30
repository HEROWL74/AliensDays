#pragma once
#include <Siv3D.hpp>
#include <memory>
#include "SceneBase.hpp"
#include "SceneFactory.hpp"

// 宇宙エフェクト用の星パーティクル
struct SpaceParticle
{
	Vec2 position;
	Vec2 velocity;
	double life;
	double maxLife;
	double size;
	ColorF color;
	double brightness;

	SpaceParticle()
		: position(Vec2::Zero()), velocity(Vec2::Zero())
		, life(1.0), maxLife(1.0), size(2.0)
		, color(Palette::White), brightness(1.0)
	{
	}
};

class SceneManagers
{
private:
	enum class FadeState
	{
		None,
		SpaceTransitionOut,  // 宇宙エフェクト開始
		SpaceTransitionIn,   // 宇宙エフェクト終了
		NormalFadeOut,       // 通常のフェード開始
		NormalFadeIn         // 通常のフェード終了
	};

	double m_fadeTimer = 0.0;
	FadeState m_fadeState = FadeState::None;
	SceneType m_nextSceneType{};

	std::unique_ptr<SceneBase> m_currentScene;
	SceneType m_currentSceneType;

	// 宇宙エフェクト用メンバー
	Array<SpaceParticle> m_spaceParticles;
	double m_warpIntensity = 0.0;
	Vec2 m_warpCenter;
	static constexpr double TRANSITION_DURATION = 1.5; // 遷移時間
	static constexpr int PARTICLE_COUNT = 120;         // パーティクル数（画面全体用に増加）

public:
	SceneManagers();
	~SceneManagers();

	// シーンの初期化
	void init(SceneType initialScene);

	// シーンの更新
	void update();

	// シーンの描画
	void draw() const;

	// シーンの切り替え
	void changeScene(SceneType newScene);

	// 現在のシーンタイプを取得
	SceneType getCurrentSceneType() const;

	// 現在のシーンを取得
	SceneBase* getCurrentScene() const { return m_currentScene.get(); }

private:
	// シーンのファクトリーメソッド
	//std::unique_ptr<SceneBase> createScene(SceneType sceneType);

	// 宇宙エフェクト関連メソッド
	void initializeSpaceTransition();
	void updateSpaceTransition();
	void drawSpaceTransition() const;
	void createSpaceParticles();
	void updateSpaceParticles();
	void drawWarpEffect() const;
	void drawStarField() const;
	void drawFullScreenCover() const;

	// シーン遷移判定
	bool shouldUseSpaceTransition(SceneType from, SceneType to) const;
};
