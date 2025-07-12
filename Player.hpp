#pragma once
#include <Siv3D.hpp>
#include "PlayerColor.hpp"

// プレイヤーのアニメーション状態
enum class PlayerState
{
	Idle,
	Front,
	Walk,
	Jump,
	Duck,
	Hit,
	Climb
};

// プレイヤーの向き
enum class PlayerDirection
{
	Left,
	Right
};

class Player
{
private:
	// プレイヤー情報
	PlayerColor m_color;
	Vec2 m_position;
	Vec2 m_velocity;
	PlayerState m_currentState;
	PlayerDirection m_direction;

	// スプライト関連
	HashTable<String, Texture> m_textures;  // スプライトテクスチャ
	double m_animationTimer;
	double m_stateTimer;
	bool m_isGrounded;

	// アニメーション設定
	static constexpr double WALK_ANIMATION_SPEED = 0.3;  // ウォークアニメーション速度
	static constexpr double CLIMB_ANIMATION_SPEED = 0.4; // クライムアニメーション速度
	static constexpr double HIT_DURATION = 0.5;         // ヒット状態の持続時間
	static constexpr double JUMP_THRESHOLD = 0.1;       // ジャンプ判定の閾値

	// 物理パラメータ
	static constexpr double MOVE_SPEED = 350.0;         // 移動速度（上げました）
	static constexpr double JUMP_POWER = 650.0;         // ジャンプ力（上げました）
	static constexpr double GRAVITY = 600.0;            // 重力
	static constexpr double GROUND_Y = 500.0;           // 地面のY座標
	static constexpr double AIR_CONTROL = 0.8;          // 空中制御係数（新規追加）

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
	void setGrounded(bool grounded) { m_isGrounded = grounded; }  // 接地状態設定

	// 位置・移動
	Vec2 getPosition() const { return m_position; }
	void setPosition(const Vec2& position) { m_position = position; }
	//void setPosition(const Vec2& position) const { const_cast<Player*>(this)->m_position = position; }  // const版追加
	Vec2 getVelocity() const { return m_velocity; }
	void setVelocity(const Vec2& velocity) { m_velocity = velocity; }

	// 物理
	void applyGravity();
	void checkGroundCollision();
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

	// ユーティリティ
	String getColorString() const;
	String getStateString() const;

private:
	// 内部ヘルパーメソッド
	void updatePhysics();
	void updateStateTransitions();
	String buildTextureKey(const String& action) const;
	String buildTextureKey(const String& action, const String& variant) const;
};
