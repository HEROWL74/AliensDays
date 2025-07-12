#pragma once
#include <Siv3D.hpp>
#include "Player.hpp"
#include "Stage.hpp"
class CoinSystem
{
public:
	// コインの状態
	enum class CoinState
	{
		Idle,        // 通常状態
		Attracting,  // 引き寄せ中
		Collected    // 収集済み
	};

	// 個別のコイン
	struct Coin
	{
		Vec2 position;          // 位置
		Vec2 originalPosition;  // 元の位置
		Vec2 velocity;          // 移動速度
		CoinState state;        // 状態
		double animationTimer;  // アニメーションタイマー
		double bobPhase;        // 浮遊効果のフェーズ
		double attractTimer;    // 引き寄せタイマー
		double scale;           // スケール
		double alpha;           // 透明度
		bool active;            // アクティブフラグ

		Coin(const Vec2& pos)
			: position(pos), originalPosition(pos), velocity(Vec2::Zero())
			, state(CoinState::Idle), animationTimer(0.0), bobPhase(0.0)
			, attractTimer(0.0), scale(1.0), alpha(1.0), active(true) {
		}
	};

	CoinSystem();
	~CoinSystem() = default;

	void init();
	void update(const Player* player, const Vec2& hudCoinPosition);
	void draw(const Vec2& cameraOffset) const;

	// コイン管理
	void addCoin(const Vec2& position);
	void clearAllCoins();
	int getCollectedCoinsCount() const { return m_collectedCoinsCount; }
	void resetCollectedCount() { m_collectedCoinsCount = 0; }

	// ステージ特化のコイン配置メソッド
	void generateCoinsForStage(StageNumber stageNumber);

private:
	// テクスチャ
	Texture m_coinTexture;
	Texture m_sparkleTexture; // きらめき効果用（オプション）

	// コイン管理
	Array<std::unique_ptr<Coin>> m_coins;
	int m_collectedCoinsCount;

	// 物理・演出定数
	static constexpr double COIN_SIZE = 64.0;           // コインサイズ（48→64に拡大）
	static constexpr double COLLECTION_DISTANCE = 80.0; // 収集開始距離
	static constexpr double ATTRACT_DISTANCE = 120.0;   // 引き寄せ開始距離
	static constexpr double ATTRACT_SPEED = 4.0;        // 引き寄せ速度（8.0→4.0に減速）
	static constexpr double BOB_SPEED = 0.04;           // 浮遊速度
	static constexpr double BOB_AMPLITUDE = 8.0;        // 浮遊振幅
	static constexpr double ROTATE_SPEED = 0.02;        // 回転速度（アニメーション用）

	// ヘルパー関数
	void updateCoin(Coin& coin, const Vec2& playerPosition, const Vec2& hudCoinPosition);
	void updateCoinPhysics(Coin& coin, const Vec2& hudCoinPosition);
	void updateCoinAnimation(Coin& coin);
	void drawCoin(const Coin& coin, const Vec2& cameraOffset) const;
	void drawCollectionEffect(const Coin& coin, const Vec2& cameraOffset) const;

	// ステージ別生成メソッド
	void generateCoinsForGrassStage();
	void generateCoinsForSandStage();
	void generateCoinsForPurpleStage();
	void generateCoinsForSnowStage();
	void generateCoinsForStoneStage();
	void generateCoinsForDirtStage();

	void loadTextures();
};
