#pragma once
#include <Siv3D.hpp>
#include "../Core/SceneBase.hpp"
#include "../Player/PlayerColor.hpp"
#include "../Player/Player.hpp"
#include "../Stages/Stage.hpp"
#include "../Enemies/EnemyBase.hpp"
#include "../Enemies/NormalSlime.hpp"
#include "../Enemies/SpikeSlime.hpp"
#include "../Enemies/Ladybug.hpp"
#include "../Enemies/SlimeBlock.hpp"
#include "../Enemies/Saw.hpp"
#include "../Enemies/Bee.hpp"
#include "../Enemies/Fly.hpp"
#include "../Systems/HUDSystem.hpp"
#include "../Systems/CoinSystem.hpp"
#include "../Systems/StarSystem.hpp"
#include "../Systems/BlockSystem.hpp"
#include "../Systems/CollisionSystem.hpp"
#include "../Effects/ShaderEffects.hpp"
#include "../Systems/DayNightSystem.hpp"
// ファイアボール撃破エフェクト用の構造体
struct FireballParticle
{
	Vec2 position;
	Vec2 velocity;
	double life;
	double maxLife;
	double size;
	ColorF color;
	double rotation;
	double rotationSpeed;

	FireballParticle()
		: position(Vec2::Zero()), velocity(Vec2::Zero())
		, life(1.0), maxLife(1.0), size(5.0)
		, color(ColorF(1.0, 1.0, 1.0)), rotation(0.0), rotationSpeed(0.0)
	{
	}
};

struct FireballShockwave
{
	Vec2 position;
	double radius;
	double maxRadius;
	double life;
	double maxLife;
	double delay;

	FireballShockwave()
		: position(Vec2::Zero()), radius(0.0), maxRadius(100.0)
		, life(1.0), maxLife(1.0), delay(0.0)
	{
	}
};

struct FireballDestructionEffect
{
	Vec2 position;
	Vec2 fireballDirection;
	EnemyType enemyType;
	double timer;
	bool active;
	int particleCount;
	ColorF primaryColor;
	ColorF secondaryColor;
	Array<FireballParticle> particles;
	Array<FireballShockwave> shockwaves;

	FireballDestructionEffect()
		: position(Vec2::Zero()), fireballDirection(Vec2(1, 0))
		, enemyType(EnemyType::NormalSlime), timer(0.0), active(false)
		, particleCount(15), primaryColor(ColorF(1.0, 1.0, 1.0))
		, secondaryColor(ColorF(0.8, 0.8, 0.8))
	{
	}
};

class GameScene final : public SceneBase
{
private:
	// 基本メンバー変数
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
	std::unique_ptr<BlockSystem> m_blockSystem;

	// 新しい統一衝突判定システム
	std::unique_ptr<CollisionSystem> m_collisionSystem;

	// ファイアボール撃破エフェクト用メンバー変数
	Array<FireballDestructionEffect> m_fireballDestructionEffects;

	//シェーダーエフェクト
	std::unique_ptr<ShaderEffects> m_shaderEffects;

	std::unique_ptr<DayNightSystem> m_dayNightSystem;

	// リザルト関連
	bool m_isLastStage;
	bool m_fromResultScene;
	static StageNumber s_nextStageNumber;
	static StageNumber s_gameOverStage;
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
	static StageNumber getNextStageNumber() noexcept { return s_nextStageNumber; }
	static StageNumber getGameOverStage()noexcept { return s_gameOverStage; }
	static int getResultStars() noexcept{ return s_resultStars; }
	static int getResultCoins() noexcept{ return s_resultCoins; }
	static double getResultTime() noexcept{ return s_resultTime; }
	static PlayerColor getResultPlayerColor()noexcept { return s_resultPlayerColor; }
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

	// 新しい統一衝突判定メソッド
	void updatePlayerCollisionsUnified();
	void updateBlockSystemInteractions();

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
	void updateFireballEnemyCollision();
	void handleEnemyHitByFireball(EnemyBase* enemy, const Vec2& fireballPosition);
	void handlePlayerHitByEnemy(EnemyBase* enemy);

	// エフェクト描画ヘルパー
	void drawEnemyFlattenedEffect(const Vec2& screenPos, const EnemyBase* enemy) const;
	void drawEnemyHitEffect(const Vec2& screenPos, const EnemyBase* enemy) const;
	void drawPlayerFireballs() const;

	// 新敵用の特殊処理
	void handleSpecialEnemyCollision(EnemyBase* enemy);
	bool canStompEnemy(const EnemyBase* enemy) const;

	// BlockSystem関連のメソッド（簡素化）
	void updateTotalCoinsFromBlocks();

	// ファイアボール撃破エフェクト用メソッド
	void createFireballDestructionEffect(const Vec2& enemyPos, EnemyType enemyType, const Vec2& fireballPos);
	void updateFireballDestructionEffects();
	void drawFireballDestructionEffects() const;

	//昼夜実装
	void updateDayNight();
	void drawDayNightEffects() const;
	void drawDayNightUI() const;
	void drawNightWarningEffects() const;
	void drawSunsetCautionEffects() const;
};
