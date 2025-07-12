#pragma once
#include <Siv3D.hpp>
#include "SceneBase.hpp"
#include "PlayerColor.hpp"
#include "Player.hpp"
#include "Stage.hpp"
#include "EnemyBase.hpp"
#include "NormalSlime.hpp"
#include "HUDSystem.hpp"
#include "CoinSystem.hpp"
#include "StarSystem.hpp"

class GameScene final : public SceneBase
{
private:
	Texture m_backgroundTexture;
	Font m_gameFont;
	Optional<SceneType> m_nextScene;

	// ゲームの状態
	double m_gameTime;
	std::unique_ptr<Player> m_player;
	std::unique_ptr<Stage> m_stage;
	StageNumber m_currentStageNumber;

	// 敵システム
	Array<std::unique_ptr<EnemyBase>> m_enemies;

	// ゴール関連
	bool m_goalReached;
	double m_goalTimer;

	// UI・収集システム
	std::unique_ptr<HUDSystem> m_hudSystem;
	std::unique_ptr<CoinSystem> m_coinSystem;
	std::unique_ptr<StarSystem> m_starSystem;

	// リザルト関連
	bool m_isLastStage;
	bool m_fromResultScene;
	static StageNumber s_nextStageNumber;
	static int s_resultStars;
	static int s_resultCoins;
	static double s_resultTime;
	static PlayerColor s_resultPlayerColor;
	static bool s_shouldLoadNextStage;
	static bool s_shouldRetryStage;

public:
	GameScene();
	~GameScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

	// 静的メソッド - リザルトデータ管理用
	static StageNumber getNextStageNumber() { return s_nextStageNumber; }
	static int getResultStars() { return s_resultStars; }
	static int getResultCoins() { return s_resultCoins; }
	static double getResultTime() { return s_resultTime; }
	static PlayerColor getResultPlayerColor() { return s_resultPlayerColor; }
	static void clearResultData() {
		s_nextStageNumber = StageNumber::Stage1;
		s_resultStars = 0;
		s_resultCoins = 0;
		s_resultTime = 0.0;
		s_shouldLoadNextStage = false;
		s_shouldRetryStage = false;
	}
	static void setNextStageMode() { s_shouldLoadNextStage = true; s_shouldRetryStage = false; }
	static void setRetryMode() { s_shouldRetryStage = true; s_shouldLoadNextStage = false; }

private:
	// ステージ関連
	void loadStage(StageNumber stageNumber);
	void updatePlayerStageCollision();

	// 敵システム関連
	void initEnemies();
	void updateEnemies();
	void drawEnemies() const;
	void updatePlayerEnemyCollision();
	void updateEnemyStageCollision();
	void addEnemy(std::unique_ptr<EnemyBase> enemy);

	// ゴール関連
	void updateGoalCheck();
	void handleGoalReached();

	// 収集システム関連
	void updateCollectionSystems();
	void updateHUDWithCollectedItems();

	// 衝突判定ヘルパー
	bool isPlayerStompingEnemy(const RectF& playerRect, const RectF& enemyRect) const;
	void handlePlayerStompEnemy(EnemyBase* enemy);
	void handlePlayerHitByEnemy(EnemyBase* enemy);

	// エフェクト描画ヘルパー
	void drawEnemyFlattenedEffect(const Vec2& screenPos, const EnemyBase* enemy) const;
	void drawEnemyHitEffect(const Vec2& screenPos, const EnemyBase* enemy) const;
};
