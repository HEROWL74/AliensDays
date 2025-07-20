#pragma once
#include "EnemyBase.hpp"

class Bee final : public EnemyBase
{
private:
	// Bee固有のパラメータ
	static constexpr double FLY_SPEED = 150.0;           // 飛行速度
	static constexpr double HOVER_AMPLITUDE = 15.0;      // ホバリング振幅
	static constexpr double HOVER_SPEED = 3.0;           // ホバリング速度
	static constexpr double FLY_ANIMATION_SPEED = 0.15;  // 羽ばたきアニメーション速度
	static constexpr double FLATTENED_DURATION = 1.0;    // 踏まれた状態の持続時間
	static constexpr double HIT_DURATION = 0.4;          // ヒット状態の持続時間
	static constexpr double CHASE_DISTANCE = 200.0;      // 追跡開始距離
	static constexpr double PATROL_RADIUS = 120.0;       // パトロール半径

	// 移動関連
	double m_hoverTimer;
	double m_patrolAngle;
	Vec2 m_startPosition;
	Vec2 m_targetPosition;
	bool m_isChasingPlayer;
	bool m_hasHitWall;

	// エフェクト関連
	bool m_isFlattened;
	double m_flattenedTimer;
	bool m_isFlying;

public:
	Bee(const Vec2& startPosition);
	~Bee() override = default;

	// EnemyBaseの純粋仮想関数の実装
	void init() override;
	void update() override;
	void draw() const override;
	void loadTextures() override;

	// 状態管理のオーバーライド
	void setState(EnemyState newState) override;
	void onHit() override;
	void onStomp() override;

	// Bee固有のメソッド
	void startFlying();
	void updateMovement();
	void updatePatrol();
	void updateChase(const Vec2& playerPosition);
	bool isFlying() const { return m_isFlying; }

protected:
	// 描画・アニメーション関連
	Texture getCurrentTexture() const override;
	void drawFlattenedEffect() const;
	void drawHitEffect() const;
	void drawFlyTrail() const;
	void drawWingEffect() const;

private:
	// 内部ヘルパーメソッド
	void updateFlyBehavior();
	void updateFlattenedBehavior();
	void updateHitBehavior();
	void calculatePatrolTarget();
};
