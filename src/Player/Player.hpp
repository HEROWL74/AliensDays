#pragma once
#include <Siv3D.hpp>
#include "../Player/PlayerColor.hpp"

// プレイヤーのアニメーション状態
enum class PlayerState
{
	Idle,
	Front,
	Walk,
	Jump,
	Duck,
	Hit,
	Climb,
	Sliding,
	Exploding,  // 爆散状態
	Dead        // 死亡状態
};

// プレイヤーの向き
enum class PlayerDirection
{
	Left,
	Right
};

// ファイアボール構造体
struct Fireball
{
	Vec2 position;
	Vec2 velocity;
	double rotation;
	double rotationSpeed;
	bool active;
	double lifetime;
	double maxLifetime;

	Fireball(const Vec2& pos, const Vec2& vel)
		: position(pos), velocity(vel), rotation(0.0)
		, rotationSpeed(8.0), active(true)
		, lifetime(0.0), maxLifetime(3.0)
	{
	}
};

class Player
{
private:
	PlayerColor m_color;
	PlayerStats m_stats;
	Vec2 m_position;
	Vec2 m_velocity;
	PlayerState m_currentState;
	PlayerDirection m_direction;

	// スプライト関連
	HashTable<String, Texture> m_textures;
	double m_animationTimer;
	double m_stateTimer;
	bool m_isGrounded;

	// 無敵時間関連
	bool m_isInvincible;
	double m_invincibleTimer;

	// 爆散エフェクト関連
	bool m_isExploding;
	double m_explosionTimer;
	double m_deathTimer;

	// スライディング関連
	bool m_isSliding;
	double m_slideTimer;
	double m_slideSpeed;
	PlayerDirection m_slideDirection;
	float SLIDE_MIN_SPEED = 3.0f;
	float SLIDE_INITIAL_SPEED = 1.0f;

	// パーティクルシステム
	struct ExplosionParticle
	{
		Vec2 position;
		Vec2 velocity;
		double life;
		double maxLife;
		double size;
		ColorF color;
		double rotation;
		double rotationSpeed;

		ExplosionParticle(const Vec2& pos, const Vec2& vel, const ColorF& col)
			: position(pos), velocity(vel), color(col)
			, life(Random(0.8, 1.5)), maxLife(life)
			, size(Random(3.0, 8.0))
			, rotation(Random(0.0, Math::TwoPi))
			, rotationSpeed(Random(-10.0, 10.0))
		{
		}
	};

	Array<ExplosionParticle> m_explosionParticles;
	Array<Vec2> m_shockwaves;
	Array<double> m_shockwaveTimers;

	Vec2 m_previousPosition;  // 前フレームの位置
	bool m_wasMovingUp;       // 前フレームで上向きに移動していたか


	// ファイアボール関連のメンバー変数
	Array<Fireball> m_fireballs;
	Texture m_fireballTexture;
	int m_fireballCount;
	static constexpr int MAX_FIREBALLS_PER_STAGE = 10;
	static constexpr double FIREBALL_SPEED = 400.0;
	static constexpr double FIREBALL_GRAVITY = 300.0;

	// 定数
	static constexpr double WALK_ANIMATION_SPEED = 0.3;
	static constexpr double CLIMB_ANIMATION_SPEED = 0.4;
	static constexpr double HIT_DURATION = 0.5;
	static constexpr double JUMP_THRESHOLD = 0.1;
	static constexpr double BASE_MOVE_SPEED = 320.0;
	static constexpr double BASE_JUMP_POWER = 960.0;
	static constexpr double BASE_INVINCIBLE_DURATION = 2.0;
	static constexpr double GRAVITY = 1600.0;
	static constexpr double GROUND_Y = 500.0;
	static constexpr double AIR_CONTROL = 0.8;
	static constexpr double EXPLOSION_DURATION = 1.5;
	static constexpr double DEATH_DELAY = 2.0;
	static constexpr int EXPLOSION_PARTICLE_COUNT = 25;
	static constexpr double PARTICLE_GRAVITY = 960.0;

	// スライディング定数
	static constexpr double SLIDE_SPEED = 448.0;
	static constexpr double SLIDE_DURATION = 384.0;
	static constexpr double SLIDE_DECELERATION = 0.85;

	// ジャンプ状態の追跡用（ブロック破壊判定用）
	bool m_wasJumping;
	double m_jumpStateTimer;
	static constexpr double JUMP_STATE_DURATION = 0.5; // ジャンプ状態を維持する時間

public:
	Player();
	Player(PlayerColor color, const Vec2& startPosition);
	~Player() = default;

	// 初期化・終了
	void init(PlayerColor color, const Vec2& startPosition);
	void loadTextures();

	// 更新・描画
	void update();
	void draw() const;

	// 入力処理
	void handleInput();

	// 状態管理
	void setState(PlayerState newState);
	PlayerState getCurrentState() const { return m_currentState; }
	void setDirection(PlayerDirection direction) { m_direction = direction; }
	PlayerDirection getDirection() const { return m_direction; }
	void setGrounded(bool grounded) { m_isGrounded = grounded; }

	// 無敵状態関連
	bool isInvincible() const { return m_isInvincible; }
	void setInvincible(bool invincible) { m_isInvincible = invincible; }
	double getInvincibleTimer() const { return m_invincibleTimer; }

	// 爆散関連のメソッド
	void startExplosion();
	bool isExploding() const { return m_isExploding; }
	bool isDead() const { return m_currentState == PlayerState::Dead; }
	double getDeathTimer() const { return m_deathTimer; }

	// スライディング関連のメソッド
	void startSliding(PlayerDirection direction);
	void updateSliding();
	void endSliding();
	bool wasMovingUpward() const;
	bool isSliding() const { return m_isSliding; }
	double getSlideSpeed() const { return m_slideSpeed; }

	// 位置・移動
	Vec2 getPosition() const { return m_position; }
	void setPosition(const Vec2& position) { m_position = position; }
	Vec2 getVelocity() const { return m_velocity; }
	void setVelocity(const Vec2& velocity) { m_velocity = velocity; }

	// 物理
	void applyGravity();
	bool isGrounded() const { return m_isGrounded; }

	// アニメーション
	void updateAnimation();
	Texture getCurrentTexture() const;
	double getScale() const;
	double getRotation() const;
	ColorF getTint() const;

	// アクション
	void jump();
	void hit();
	void duck();

	// ファイアボール関連のメソッド
	void fireFireball();
	void updateFireballs();
	void drawFireballs() const;
	void deactivateFireball(const Vec2& position);
	const Array<Fireball>& getFireballs() const { return m_fireballs; }
	int getFireballCount() const { return m_fireballCount; }
	int getRemainingFireballs() const { return MAX_FIREBALLS_PER_STAGE - m_fireballCount; }
	void resetFireballCount() { m_fireballCount = 0; }

	// キャラクター情報
	PlayerColor getColor() const { return m_color; }
	PlayerStats getStats() const { return m_stats; }
	int getMaxLife() const { return m_stats.maxLife; }

	// ユーティリティ
	String getColorString() const;
	String getStateString() const;

	// ジャンプ状態を取得するメソッドを追加
	bool isInJumpState() const {
		// より厳格なジャンプ状態の判定
		return (m_currentState == PlayerState::Jump && m_velocity.y < -50.0) ||
			(m_jumpStateTimer > 0.0 && m_velocity.y < -30.0) ||
			(!m_isGrounded && m_velocity.y < -50.0);
	}


private:
	// 内部ヘルパーメソッド
	void updatePhysics();
	void updateBasicPhysics();
	void checkBasicGroundCollision();
	void updateStateTransitions();
	void updateGroundStateTransitions();
	void updateInvincibility();
	String buildTextureKey(const String& action) const;
	String buildTextureKey(const String& action, const String& variant) const;

	// 特性を適用した値を取得
	double getActualMoveSpeed() const { return BASE_MOVE_SPEED * m_stats.moveSpeed; }
	double getActualJumpPower() const { return BASE_JUMP_POWER * m_stats.jumpPower; }
	double getActualInvincibleDuration() const { return BASE_INVINCIBLE_DURATION * m_stats.invincibleTime; }

	// 爆散エフェクトの内部メソッド
	void updateExplosion();
	void createExplosionParticles();
	void createShockwaves();
	void updateExplosionParticles();
	void updateShockwaves();
	void drawExplosionEffect() const;
	void drawExplosionParticles() const;
	void drawShockwaves() const;
};
