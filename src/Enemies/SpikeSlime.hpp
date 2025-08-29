#pragma once
#include "EnemyBase.hpp"

class SpikeSlime final : public EnemyBase
{
private:
	// SpikeSlime固有のパラメータ
	static constexpr double WALK_SPEED = 60.0;           // 歩行速度（NormalSlimeより遅い）
	static constexpr double WALK_ANIMATION_SPEED = 0.6;  // 歩行アニメーション速度
	static constexpr double FLATTENED_DURATION = 1.5;    // 踏まれた状態の持続時間（長め）
	static constexpr double HIT_DURATION = 0.5;          // ヒット状態の持続時間
	static constexpr double DIRECTION_CHANGE_TIME = 4.0; // 方向転換の間隔（長め）
	static constexpr double SPIKE_DAMAGE_RADIUS = 80.0;  // スパイクダメージ範囲

	// 移動関連
	double m_directionTimer;
	double m_patrolDistance;
	Vec2 m_startPosition;
	bool m_hasHitWall;

	// エフェクト関連
	bool m_isFlattened;
	double m_flattenedTimer;
	// スパイクは常に危険（削除: bool m_isSpikeDangerous）

public:
	SpikeSlime(const Vec2& startPosition);
	~SpikeSlime() override = default;

	//ファクトリーパターンのEnemyKey
	String typeKey() const noexcept override { return U"SpikeSlime"; }
	String currentVisualKey() const override;

	// EnemyBaseの純粋仮想関数の実装
	void init() override;
	void update() override;
	void draw() const override;
	void loadTextures() override;

	// 状態管理のオーバーライド
	void setState(EnemyState newState) override;
	void onHit() override;
	void onStomp() override;

	// SpikeSlime固有のメソッド
	void startWalking();
	void checkWallCollision();
	void updateMovement();
	void changeDirection();
	bool isDangerous() const override { return m_isActive && m_isAlive; }  // 常に危険
	bool hasSpikes() const override { return true; }  // 常にスパイクを持つ

protected:
	// 描画・アニメーション関連
	Texture getCurrentTexture() const override;
	void drawFlattenedEffect() const;
	void drawHitEffect() const;
	void drawSpikeWarning() const;

private:
	// 内部ヘルパーメソッド
	void updateWalkBehavior();
	void updateFlattenedBehavior();
	void updateHitBehavior();
};
