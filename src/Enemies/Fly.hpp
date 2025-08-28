#pragma once
#include "EnemyBase.hpp"

class Fly final : public EnemyBase
{
private:
	// Fly固有のパラメータ
	static constexpr double FLY_SPEED = 100.0;           // 飛行速度
	static constexpr double ERRATIC_AMPLITUDE = 25.0;    // 不規則な動きの振幅
	static constexpr double ERRATIC_SPEED = 5.0;         // 不規則な動きの速度
	static constexpr double FLY_ANIMATION_SPEED = 0.1;   // 羽ばたきアニメーション速度
	static constexpr double FLATTENED_DURATION = 0.6;    // 踏まれた状態の持続時間
	static constexpr double HIT_DURATION = 0.3;          // ヒット状態の持続時間
	static constexpr double DIRECTION_CHANGE_TIME = 2.0; // 方向転換の間隔

	// 移動関連
	double m_erraticTimer;
	double m_directionTimer;
	double m_patrolDistance;
	Vec2 m_startPosition;
	Vec2 m_erraticOffset;
	bool m_hasHitWall;

	// エフェクト関連
	bool m_isFlattened;
	double m_flattenedTimer;
	bool m_isFlying;

public:
	Fly(const Vec2& startPosition);
	~Fly() override = default;

	//ファクトリーパターンのEnemyKey
	String typeKey() const noexcept override { return U"Fly"; }
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

	// Fly固有のメソッド
	void startFlying();
	void updateMovement();
	void changeDirection();
	bool isFlying() const { return m_isFlying; }

protected:
	// 描画・アニメーション関連
	Texture getCurrentTexture() const override;
	void drawFlattenedEffect() const;
	void drawHitEffect() const;
	void drawFlyTrail() const;
	void drawErraticPath() const;

private:
	// 内部ヘルパーメソッド
	void updateFlyBehavior();
	void updateFlattenedBehavior();
	void updateHitBehavior();
	void updateErraticMovement();
};
