#pragma once
#include "EnemyBase.hpp"

class NormalSlime final : public EnemyBase
{
private:
	// NormalSlime固有のパラメータ
	static constexpr double WALK_SPEED = 80.0;           // 歩行速度
	static constexpr double WALK_ANIMATION_SPEED = 0.5;  // 歩行アニメーション速度
	static constexpr double FLATTENED_DURATION = 1.0;    // 踏まれた状態の持続時間
	static constexpr double HIT_DURATION = 0.5;          // ヒット状態の持続時間
	static constexpr double DIRECTION_CHANGE_TIME = 3.0; // 方向転換の間隔

	// 移動関連
	double m_directionTimer;
	double m_patrolDistance;
	Vec2 m_startPosition;
	bool m_hasHitWall;

	// エフェクト関連
	bool m_isFlattened;
	double m_flattenedTimer;

public:
	NormalSlime(const Vec2& startPosition);
	~NormalSlime() override = default;

	// EnemyBaseの純粋仮想関数の実装
	void init() override;
	void update() override;
	void draw() const override;
	void loadTextures() override;

	// 状態管理のオーバーライド
	void setState(EnemyState newState) override;
	void onHit() override;
	void onStomp() override;

	// NormalSlime固有のメソッド
	void startWalking();
	void checkWallCollision();
	void updateMovement();
	void changeDirection();  // publicに移動

protected:
	// 描画・アニメーション関連
	Texture getCurrentTexture() const override;
	void drawFlattenedEffect() const;
	void drawHitEffect() const;

private:
	// 内部ヘルパーメソッド
	void updateWalkBehavior();
	void updateFlattenedBehavior();
	void updateHitBehavior();
};
