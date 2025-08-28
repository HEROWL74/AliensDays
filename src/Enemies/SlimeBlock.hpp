#pragma once
#include "EnemyBase.hpp"

class SlimeBlock final : public EnemyBase
{
private:
	// SlimeBlock固有のパラメータ
	static constexpr double WALK_SPEED = 40.0;           // 歩行速度（遅い）
	static constexpr double JUMP_POWER = 400.0;          // ジャンプ力
	static constexpr double WALK_ANIMATION_SPEED = 0.8;  // 歩行アニメーション速度
	static constexpr double FLATTENED_DURATION = 2.0;    // 踏まれた状態の持続時間（長い）
	static constexpr double HIT_DURATION = 0.6;          // ヒット状態の持続時間
	static constexpr double JUMP_INTERVAL = 2.5;         // ジャンプの間隔
	static constexpr double DIRECTION_CHANGE_TIME = 5.0; // 方向転換の間隔

	// 移動関連
	double m_jumpTimer;
	double m_directionTimer;
	double m_patrolDistance;
	Vec2 m_startPosition;
	bool m_hasHitWall;
	bool m_canJump;

	// エフェクト関連
	bool m_isFlattened;
	double m_flattenedTimer;
	bool m_isJumping;

public:
	SlimeBlock(const Vec2& startPosition);
	~SlimeBlock() override = default;

	//ファクトリーパターンのEnemyKey
	String typeKey() const noexcept override { return U"SlimeBlock"; }
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

	// SlimeBlock固有のメソッド
	void startWalking();
	void performJump();
	void updateMovement();
	void changeDirection();
	bool isJumping() const { return m_isJumping; }

protected:
	// 描画・アニメーション関連
	Texture getCurrentTexture() const override;
	void drawFlattenedEffect() const;
	void drawHitEffect() const;
	void drawJumpPreparation() const;

private:
	// 内部ヘルパーメソッド
	void updateWalkBehavior();
	void updateFlattenedBehavior();
	void updateHitBehavior();
	void updateJumpState();
};
