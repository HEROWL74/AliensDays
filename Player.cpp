#include "Player.hpp"

Player::Player()
	: m_color(PlayerColor::Green)
	, m_position(Vec2::Zero())
	, m_velocity(Vec2::Zero())
	, m_currentState(PlayerState::Idle)
	, m_direction(PlayerDirection::Right)
	, m_animationTimer(0.0)
	, m_stateTimer(0.0)
	, m_isGrounded(false)
{
}

Player::Player(PlayerColor color, const Vec2& startPosition)
	: m_color(color)
	, m_position(startPosition)
	, m_velocity(Vec2::Zero())
	, m_currentState(PlayerState::Idle)
	, m_direction(PlayerDirection::Right)
	, m_animationTimer(0.0)
	, m_stateTimer(0.0)
	, m_isGrounded(false)
{
	loadTextures();
}

void Player::init(PlayerColor color, const Vec2& startPosition)
{
	m_color = color;
	m_position = startPosition;
	m_velocity = Vec2::Zero();
	m_currentState = PlayerState::Idle;
	m_direction = PlayerDirection::Right;
	m_animationTimer = 0.0;
	m_stateTimer = 0.0;
	m_isGrounded = false;

	loadTextures();
}

void Player::loadTextures()
{
	m_textures.clear();

	const String colorStr = getColorString();
	const String basePath = U"Sprites/Characters/";

	// 各アニメーション状態のテクスチャを読み込み
	Array<String> actions = {
		U"duck", U"front", U"hit", U"idle", U"jump",
		U"climb_a", U"climb_b", U"walk_a", U"walk_b"
	};

	for (const auto& action : actions)
	{
		const String filename = U"character_{}_{}.png"_fmt(colorStr, action);
		const String filepath = basePath + filename;
		const String key = action;

		Texture texture(filepath);
		if (texture)
		{
			m_textures[key] = texture;
		}
		else
		{
			// フォールバック：読み込みに失敗した場合はログ出力
			Print << U"Failed to load texture: " << filepath;
		}
	}
}

void Player::update()
{
	const double deltaTime = Scene::DeltaTime();

	m_animationTimer += deltaTime;
	m_stateTimer += deltaTime;

	// 入力処理
	handleInput();

	// 物理更新
	updatePhysics();

	// 状態遷移の更新
	updateStateTransitions();

	// アニメーション更新
	updateAnimation();
}

void Player::draw() const
{
	const Texture currentTexture = getCurrentTexture();

	if (currentTexture)
	{
		const double scale = getScale();
		const double rotation = getRotation();
		const ColorF tint = getTint();

		// 向きに応じて左右反転
		if (m_direction == PlayerDirection::Left)
		{
			currentTexture.scaled(scale).mirrored().rotated(rotation).drawAt(m_position, tint);
		}
		else
		{
			currentTexture.scaled(scale).rotated(rotation).drawAt(m_position, tint);
		}
	}
	else
	{
		// フォールバック：テクスチャがない場合は円で表示
		const ColorF fallbackColor = getTint();
		Circle(m_position, 25).draw(fallbackColor);
	}

	// デバッグ情報（開発時のみ）
#ifdef _DEBUG
	const String debugText = U"{} - {}"_fmt(getColorString(), getStateString());
	Font(16)(debugText).draw(m_position.x - 50, m_position.y - 60, ColorF(1.0, 1.0, 1.0));
#endif
}

void Player::handleInput()
{
	// 移動入力
	Vec2 inputVector(0, 0);

	if (KeyLeft.pressed() || KeyA.pressed())
	{
		inputVector.x = -1;
		setDirection(PlayerDirection::Left);
	}
	if (KeyRight.pressed() || KeyD.pressed())
	{
		inputVector.x = 1;
		setDirection(PlayerDirection::Right);
	}

	// 移動処理（地上・空中両方で有効）
	if (inputVector.x != 0.0)
	{
		double moveSpeed = MOVE_SPEED;

		// 空中では移動速度を調整
		if (!m_isGrounded)
		{
			moveSpeed *= AIR_CONTROL;  // 空中制御係数を適用
		}

		m_velocity.x = inputVector.x * moveSpeed;

		// しゃがみ中の地上でなければ歩行状態に
		if (m_isGrounded && m_currentState != PlayerState::Duck)
		{
			setState(PlayerState::Walk);
		}
	}
	else
	{
		// 地上で移動していない場合はX速度を減衰
		if (m_isGrounded)
		{
			m_velocity.x = 0.0;

			// 移動していない場合はアイドル状態に
			if (m_currentState == PlayerState::Walk)
			{
				setState(PlayerState::Idle);
			}
		}
		else
		{
			// 空中では空気抵抗で徐々に減速
			m_velocity.x *= 0.95;
		}
	}

	// ジャンプ（地上でのみ可能）
	if ((KeySpace.down() || KeyUp.down() || KeyW.down()) && m_isGrounded && m_currentState != PlayerState::Duck)
	{
		jump();
	}

	// しゃがみ（地上でのみ可能）
	if (KeyDown.pressed() || KeyS.pressed())
	{
		if (m_isGrounded)
		{
			duck();
		}
	}
	else if (m_currentState == PlayerState::Duck)
	{
		// しゃがみキーを離したらアイドル状態に
		setState(PlayerState::Idle);
	}
}

void Player::setState(PlayerState newState)
{
	if (m_currentState != newState)
	{
		m_currentState = newState;
		m_stateTimer = 0.0;
		m_animationTimer = 0.0;
	}
}

void Player::updatePhysics()
{
	const double deltaTime = Scene::DeltaTime();

	// 重力適用
	if (!m_isGrounded)
	{
		applyGravity();
	}

	// 位置更新
	m_position += m_velocity * deltaTime;

	// 地面との衝突判定
	checkGroundCollision();

	// 画面端での制限を削除（ステージの境界はGameSceneで管理）
	// 修正前：m_position.x = Clamp(m_position.x, 50.0, Scene::Width() - 50.0);
	// 修正後：この制限を削除してステージ全体を移動可能にする
}

void Player::updateStateTransitions()
{
	// ヒット状態の自動解除
	if (m_currentState == PlayerState::Hit && m_stateTimer >= HIT_DURATION)
	{
		setState(PlayerState::Idle);
	}

	// 空中状態の判定（GameSceneで管理されるため、ここでは基本的な処理のみ）
	// 実際の状態遷移はGameSceneのupdatePlayerStageCollisionで行われる
}

void Player::updateAnimation()
{
	// アニメーション固有の処理は各状態で個別に実装可能
	// 現在は基本的なタイマー更新のみ
}

Texture Player::getCurrentTexture() const
{
	String textureKey;

	switch (m_currentState)
	{
	case PlayerState::Idle:
		textureKey = U"idle";
		break;
	case PlayerState::Front:
		textureKey = U"front";
		break;
	case PlayerState::Walk:
	{
		// ウォークアニメーション（walk_a と walk_b を交互に）
		const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
		textureKey = useVariantA ? U"walk_a" : U"walk_b";
	}
	break;
	case PlayerState::Jump:
		textureKey = U"jump";
		break;
	case PlayerState::Duck:
		textureKey = U"duck";
		break;
	case PlayerState::Hit:
		textureKey = U"hit";
		break;
	case PlayerState::Climb:
	{
		// クライムアニメーション（climb_a と climb_b を交互に）
		const bool useVariantA = std::fmod(m_animationTimer, CLIMB_ANIMATION_SPEED * 2) < CLIMB_ANIMATION_SPEED;
		textureKey = useVariantA ? U"climb_a" : U"climb_b";
	}
	break;
	default:
		textureKey = U"idle";
		break;
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else
	{
		// フォールバック：アイドルテクスチャ
		if (m_textures.contains(U"idle"))
		{
			return m_textures.at(U"idle");
		}
		return Texture{};
	}
}

double Player::getScale() const
{
	// 全ての状態で統一サイズに変更
	return 1.0;
}

double Player::getRotation() const
{
	switch (m_currentState)
	{
	case PlayerState::Hit:
		// ヒット時は軽く震える
		return std::sin(m_stateTimer * 30.0) * 0.1;
	default:
		return 0.0;
	}
}

ColorF Player::getTint() const
{
	switch (m_currentState)
	{
	case PlayerState::Hit:
		// ヒット時は赤く点滅
	{
		const double flashIntensity = std::sin(m_stateTimer * 15.0);
		return ColorF(1.0, 1.0 - flashIntensity * 0.5, 1.0 - flashIntensity * 0.5);
	}
	default:
		return ColorF(1.0, 1.0, 1.0);
	}
}

void Player::applyGravity()
{
	m_velocity.y += GRAVITY * Scene::DeltaTime();
}

void Player::checkGroundCollision()
{
	// この関数は現在GameSceneの衝突判定に置き換えられているため、
	// 簡易的な画面下端チェックのみ実行
	if (m_position.y >= Scene::Height() - 50.0)
	{
		m_position.y = Scene::Height() - 50.0;
		m_velocity.y = 0.0;
		m_isGrounded = true;
	}
	else
	{
		// 地面判定はGameSceneで処理されるため、ここでは空中状態のみ設定
		// m_isGrounded = false; // GameSceneで適切に設定される
	}
}

void Player::jump()
{
	if (m_isGrounded)
	{
		m_velocity.y = -JUMP_POWER;
		setState(PlayerState::Jump);
		m_isGrounded = false;
	}
}

void Player::hit()
{
	setState(PlayerState::Hit);
	// ヒット時は少し後ろに押し戻される
	const double knockbackForce = (m_direction == PlayerDirection::Right) ? -100.0 : 100.0;
	m_velocity.x = knockbackForce;
}

void Player::duck()
{
	if (m_isGrounded)
	{
		setState(PlayerState::Duck);
		m_velocity.x = 0.0;  // しゃがみ中は移動停止
	}
}

String Player::getColorString() const
{
	switch (m_color)
	{
	case PlayerColor::Green:  return U"green";
	case PlayerColor::Pink:   return U"pink";
	case PlayerColor::Purple: return U"purple";
	case PlayerColor::Beige:  return U"beige";
	case PlayerColor::Yellow: return U"yellow";
	default: return U"green";
	}
}

String Player::getStateString() const
{
	switch (m_currentState)
	{
	case PlayerState::Idle:  return U"Idle";
	case PlayerState::Front: return U"Front";
	case PlayerState::Walk:  return U"Walk";
	case PlayerState::Jump:  return U"Jump";
	case PlayerState::Duck:  return U"Duck";
	case PlayerState::Hit:   return U"Hit";
	case PlayerState::Climb: return U"Climb";
	default: return U"Unknown";
	}
}

String Player::buildTextureKey(const String& action) const
{
	return action;
}

String Player::buildTextureKey(const String& action, const String& variant) const
{
	return U"{}_{}"_fmt(action, variant);
}
