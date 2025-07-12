#pragma once
#include <Siv3D.hpp>

// 敵の種類
enum class EnemyType
{
	Bee,
	NormalSlime,
	Saw,
	Ladybug,
	SlimeBlock,
	SpikeSlime
};

// 敵の状態
enum class EnemyState
{
	Idle,
	Walk,
	Hit,
	Dead,
	Flattened,  // 踏まれた状態（スライム系用）
	Attack      // 攻撃状態
};

// 敵の向き
enum class EnemyDirection
{
	Left,
	Right
};

// 敵の基底クラス
class EnemyBase
{
protected:
	// 基本情報
	EnemyType m_type;
	Vec2 m_position;
	Vec2 m_velocity;
	EnemyState m_state;
	EnemyDirection m_direction;
	bool m_isActive;
	bool m_isAlive;

	// アニメーション関連
	HashTable<String, Texture> m_textures;
	double m_animationTimer;
	double m_stateTimer;

	// 物理パラメータ
	double m_moveSpeed;
	double m_gravity;
	bool m_isGrounded;

	// 当たり判定
	RectF m_collisionRect;
	double m_collisionWidth;
	double m_collisionHeight;

	// エフェクト関連
	double m_effectTimer;
	bool m_hasEffect;

public:
	EnemyBase(EnemyType type, const Vec2& startPosition);
	virtual ~EnemyBase() = default;

	// 純粋仮想関数
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void draw() const = 0;
	virtual void loadTextures() = 0;

	// 状態管理
	virtual void setState(EnemyState newState);
	virtual void onHit();
	virtual void onStomp();
	virtual void onDestroy();

	// 当たり判定
	virtual RectF getCollisionRect() const;
	virtual bool checkCollisionWith(const RectF& rect) const;

	// 位置・移動
	Vec2 getPosition() const { return m_position; }
	void setPosition(const Vec2& position) { m_position = position; }
	Vec2 getVelocity() const { return m_velocity; }
	void setVelocity(const Vec2& velocity) { m_velocity = velocity; }

	// 状態取得
	EnemyType getType() const { return m_type; }
	EnemyState getState() const { return m_state; }
	EnemyDirection getDirection() const { return m_direction; }
	bool isActive() const { return m_isActive; }
	bool isAlive() const { return m_isAlive; }
	bool isGrounded() const { return m_isGrounded; }

	// 物理更新
	virtual void updatePhysics();
	virtual void applyGravity();

	// ユーティリティ
	void setDirection(EnemyDirection direction) { m_direction = direction; }
	void setGrounded(bool grounded) { m_isGrounded = grounded; }
	virtual String getStateString() const;
	virtual String getTypeString() const;
	virtual Texture getCurrentTexture() const = 0;

protected:
	// 内部ヘルパーメソッド
	virtual void updateAnimation();
	virtual void updateCollisionRect();

	virtual String buildTextureKey(const String& action) const;
	virtual String buildTextureKey(const String& action, const String& variant) const;
};
