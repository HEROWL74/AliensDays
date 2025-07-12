#pragma once
#include <Siv3D.hpp>
#include "Player.hpp"
#include "Stage.hpp"

class StarSystem
{
public:
	// 星の状態
	enum class StarState
	{
		Idle,        // 通常状態
		Attracting,  // 引き寄せ中
		Collected    // 収集済み
	};

	// 個別の星
	struct Star
	{
		Vec2 position;          // 位置
		Vec2 originalPosition;  // 元の位置
		StarState state;        // 状態
		double animationTimer;  // アニメーションタイマー
		double bobPhase;        // 浮遊効果のフェーズ
		double attractTimer;    // 引き寄せタイマー
		double scale;           // スケール
		double rotation;        // 回転角度
		double alpha;           // 透明度
		bool active;            // アクティブフラグ
		bool collected;         // 収集済みフラグ

		// 収集アニメーション用の変数
		double collectionPhase; // 収集アニメーションの進行度
		double burstIntensity;  // バースト効果の強度
		int sparkleCount;       // きらきらの数

		Star(const Vec2& pos)
			: position(pos), originalPosition(pos), state(StarState::Idle)
			, animationTimer(0.0), bobPhase(0.0), attractTimer(0.0)
			, scale(1.0), rotation(0.0), alpha(1.0), active(true), collected(false)
			, collectionPhase(0.0), burstIntensity(0.0), sparkleCount(0) {
		}
	};

	StarSystem();
	~StarSystem() = default;

	void init();
	void update(const Player* player);
	void draw(const Vec2& cameraOffset) const;

	// 星管理
	void addStar(const Vec2& position);
	void clearAllStars();
	int getCollectedStarsCount() const { return m_collectedStarsCount; }
	void resetCollectedCount() { m_collectedStarsCount = 0; }

	// ステージ特化の星配置メソッド
	void generateStarsForStage(StageNumber stageNumber);

	// デバッグ用メソッド
	int getTotalStarsCount() const { return static_cast<int>(m_stars.size()); }
	int getActiveStarsCount() const {
		int count = 0;
		for (const auto& star : m_stars) {
			if (star->active) count++;
		}
		return count;
	}

	// スター配置情報を出力（範囲チェック付き）
	void printStarPositions() const {
		const double STAGE_MAX_X = 80 * 64; // 5120px
		const double STAGE_MAX_Y = 17 * 64; // 1088px

		Print << U"=== Star Position Validation ===";
		Print << U"Stage bounds: X(0-" << STAGE_MAX_X << U"), Y(0-" << STAGE_MAX_Y << U")";
		Print << U"Total stars: " << m_stars.size();

		for (size_t i = 0; i < m_stars.size(); ++i) {
			const auto& star = m_stars[i];
			const bool validX = star->position.x >= 0 && star->position.x <= STAGE_MAX_X;
			const bool validY = star->position.y >= 0 && star->position.y <= STAGE_MAX_Y;
			const String status = (validX && validY) ? U"✓ VALID" : U"✗ OUT OF BOUNDS";

			Print << U"Star " << (i + 1) << U": (" << star->position.x << U", " << star->position.y
				<< U") Active: " << (star->active ? U"Yes" : U"No") << U" " << status;
		}
		Print << U"================================";
	}

private:
	// テクスチャ
	Texture m_starTexture;
	Texture m_sparkleTexture;

	// 星管理
	Array<std::unique_ptr<Star>> m_stars;
	int m_collectedStarsCount;

	// 物理・演出定数
	static constexpr double STAR_SIZE = 64.0;           // 星サイズ
	static constexpr double COLLECTION_DISTANCE = 100.0; // 収集開始距離
	static constexpr double ATTRACT_DISTANCE = 150.0;   // 引き寄せ開始距離
	static constexpr double BOB_SPEED = 0.03;           // 浮遊速度
	static constexpr double BOB_AMPLITUDE = 12.0;       // 浮遊振幅
	static constexpr double ROTATE_SPEED = 0.04;        // 回転速度
	static constexpr double SPARKLE_DURATION = 2.0;     // 点滅+パーティクル持続時間（3回点滅用）

	// ヘルパー関数
	void updateStar(Star& star, const Vec2& playerPosition);
	void updateStarPhysics(Star& star);
	void updateStarAnimation(Star& star);
	void drawStar(const Star& star, const Vec2& cameraOffset) const;
	void drawCollectionEffect(const Star& star, const Vec2& cameraOffset) const;

	// 収集エフェクト関数
	void drawCollectionBurst(const Star& star, const Vec2& screenPos) const;

	// ステージ別生成メソッド
	void generateStarsForGrassStage();
	void generateStarsForSandStage();
	void generateStarsForPurpleStage();
	void generateStarsForSnowStage();
	void generateStarsForStoneStage();
	void generateStarsForDirtStage();

	void loadTextures();
};
