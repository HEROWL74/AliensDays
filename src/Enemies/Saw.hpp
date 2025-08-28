#pragma once
#include "EnemyBase.hpp"

class Saw final : public EnemyBase
{
private:
	// Saw固有のパラメータ
	static constexpr double MOVE_SPEED = 80.0;           // 移動速度
	static constexpr double ROTATE_SPEED = 8.0;          // 回転速度
	static constexpr double SAW_ANIMATION_SPEED = 0.2;   // ノコギリアニメーション速度
	static constexpr double DANGER_RADIUS = 90.0;        // 危険範囲
	static constexpr double DIRECTION_CHANGE_TIME = 3.0; // 方向転換の間隔

	// 移動関連
	double m_directionTimer;
	double m_patrolDistance;
	Vec2 m_startPosition;
	bool m_hasHitWall;
	double m_rotationAngle;

	// エフェクト関連
	bool m_isSpinning;
	double m_sparkTimer;

public:
	Saw(const Vec2& startPosition);
	~Saw() override = default;

	//ファクトリーパターンのEnemyKey
	String typeKey() const noexcept override { return U"Saw"; }
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

	// Saw固有のメソッド
	void startMoving();
	void updateMovement();
	void changeDirection();
	bool isDangerous() const { return m_isActive && m_isAlive; }

protected:
	// 描画・アニメーション関連
	Texture getCurrentTexture() const override;
	void drawDangerEffect() const;
	void drawSparks() const;
	void drawRotationEffect() const;

private:
	// 内部ヘルパーメソッド
	void updateRotation();
	void updateSparks();
};
