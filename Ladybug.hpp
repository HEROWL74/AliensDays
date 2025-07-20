#pragma once
#include "EnemyBase.hpp"

class Ladybug final : public EnemyBase
{
private:
	// Ladybug固有のパラメータ
	static constexpr double WALK_SPEED = 100.0;          // 歩行速度
	static constexpr double FLY_SPEED = 120.0;           // 飛行速度
	static constexpr double WALK_ANIMATION_SPEED = 0.4;  // 歩行アニメーション速度
	static constexpr double FLY_ANIMATION_SPEED = 0.3;   // 飛行アニメーション速度
	static constexpr double FLATTENED_DURATION = 0.8;    // 踏まれた状態の持続時間
	static constexpr double HIT_DURATION = 0.4;          // ヒット状態の持続時間
	static constexpr double FLY_MODE_DURATION = 3.0;     // 飛行モードの持続時間
	static constexpr double WALK_MODE_DURATION = 4.0;    // 歩行モードの持続時間
	static constexpr double FLY_HEIGHT = 100.0;          // 飛行高度

	// 移動関連
	double m_modeTimer;
	double m_patrolDistance;
	Vec2 m_startPosition;
	Vec2 m_flyTargetPosition;
	bool m_isFlyMode;
	bool m_hasHitWall;

	// エフェクト関連
	bool m_isFlattened;
	double m_flattenedTimer;

public:
	Ladybug(const Vec2& startPosition);
	~Ladybug() override = default;

	// EnemyBaseの純粋仮想関数の実装
	void init() override;
	void update() override;
	void draw() const override;
	void loadTextures() override;

	// 状態管理のオーバーライド
	void setState(EnemyState newState) override;
	void onHit() override;
	void onStomp() override;

	// Ladybug固有のメソッド
	void startWalking();
	void startFlying();
	void updateMovement();
	void changeDirection();
	void switchMode();
	bool isFlyMode() const { return m_isFlyMode; }

protected:
	// 描画・アニメーション関連
	Texture getCurrentTexture() const override;
	void drawFlattenedEffect() const;
	void drawHitEffect() const;
	void drawFlyTrail() const;

private:
	// 内部ヘルパーメソッド
	void updateWalkBehavior();
	void updateFlyBehavior();
	void updateFlattenedBehavior();
	void updateHitBehavior();
	void calculateFlyTarget();
};
