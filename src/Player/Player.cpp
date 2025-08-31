#include "Player.hpp"
#include "../Sound/SoundManager.hpp"

Player::Player()
	: m_color(PlayerColor::Green)
	, m_stats(getPlayerStats(PlayerColor::Green))
	, m_position(Vec2::Zero())
	, m_velocity(Vec2::Zero())
	, m_currentState(PlayerState::Idle)
	, m_direction(PlayerDirection::Right)
	, m_animationTimer(0.0)
	, m_stateTimer(0.0)
	, m_isGrounded(false)
	, m_isInvincible(false)
	, m_invincibleTimer(0.0)
	, m_isExploding(false)
	, m_explosionTimer(0.0)
	, m_deathTimer(0.0)
	, m_fireballCount(0)
	, m_isSliding(false)         
	, m_slideTimer(0.0)          
	, m_slideSpeed(0.0)          
	, m_slideDirection(PlayerDirection::Right)
{
	// パーティクル配列をクリア
	m_explosionParticles.clear();
	m_shockwaves.clear();
	m_shockwaveTimers.clear();
	m_fireballs.clear();
}

Player::Player(PlayerColor color, const Vec2& startPosition)
	: m_color(color)
	, m_stats(getPlayerStats(color))
	, m_position(startPosition)
	, m_velocity(Vec2::Zero())
	, m_currentState(PlayerState::Idle)
	, m_direction(PlayerDirection::Right)
	, m_animationTimer(0.0)
	, m_stateTimer(0.0)
	, m_isGrounded(false)
	, m_isInvincible(false)
	, m_invincibleTimer(0.0)
	, m_isExploding(false)
	, m_explosionTimer(0.0)
	, m_deathTimer(0.0)
	, m_fireballCount(0)
	, m_isSliding(false)
	, m_slideTimer(0.0)  
	, m_slideSpeed(0.0)       
	, m_slideDirection(PlayerDirection::Right) 
{
	// パーティクル配列をクリア
	m_explosionParticles.clear();
	m_shockwaves.clear();
	m_shockwaveTimers.clear();
	m_fireballs.clear();

	loadTextures();
}
void Player::init(PlayerColor color, const Vec2& startPosition)
{
	m_color = color;
	m_stats = getPlayerStats(color);
	m_position = startPosition;
	m_velocity = Vec2::Zero();
	m_currentState = PlayerState::Idle;
	m_direction = PlayerDirection::Right;
	m_animationTimer = 0.0;
	m_stateTimer = 0.0;
	m_isGrounded = false;
	m_isInvincible = false;
	m_invincibleTimer = 0.0;

	// ★ 重要：爆散関連の状態を完全にリセット
	m_isExploding = false;
	m_explosionTimer = 0.0;
	m_deathTimer = 0.0;

	// ★ ファイアボール関連もリセット
	m_fireballCount = 0;
	m_fireballs.clear();

	// パーティクル配列をクリア
	m_explosionParticles.clear();
	m_shockwaves.clear();
	m_shockwaveTimers.clear();

	loadTextures();
}

void Player::loadTextures()
{
	m_textures.clear();

	const String colorStr = getColorString();
	const String basePath = U"Sprites/Characters/";

	// 既存のテクスチャ読み込み...
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
			Print << U"Failed to load texture: " << filepath;
		}
	}

	// ★ ファイアボールテクスチャの読み込み
	m_fireballTexture = Texture(U"Sprites/Tiles/fireball.png");
	if (!m_fireballTexture)
	{
		Print << U"Failed to load fireball texture";
	}
}

void Player::update()
{
	const double deltaTime = Scene::DeltaTime();

	// ★ 追加: 前フレームの位置を記録
	m_previousPosition = m_position;
	m_wasMovingUp = m_velocity.y < 0;

	m_animationTimer += deltaTime;
	m_stateTimer += deltaTime;

	// ★ ジャンプ状態タイマーの更新
	if (m_jumpStateTimer > 0.0)
	{
		m_jumpStateTimer -= deltaTime;
	}

	// 無敵状態の更新
	updateInvincibility();

	// 爆散状態の場合は特別処理
	if (m_isExploding)
	{
		updateExplosion();
		return;
	}

	// 死亡状態の場合は何もしない
	if (m_currentState == PlayerState::Dead)
	{
		return;
	}

	// 入力処理
	handleInput();

	// ★ 修正: 基本的な物理更新のみ（重力など）
	// 衝突判定はCollisionSystemに任せる
	updateBasicPhysics();

	// 状態遷移の更新（最小限に）
	updateStateTransitions();

	// アニメーション更新
	updateAnimation();

	// ファイアボール更新
	updateFireballs();
}


void Player::draw() const
{
	// 爆散中の場合はエフェクトのみ描画
	if (m_isExploding)
	{
		drawExplosionEffect();
		return;
	}

	// 死亡状態の場合は何も描画しない
	if (m_currentState == PlayerState::Dead)
	{
		return;
	}

	// 通常のプレイヤー描画
	const Texture currentTexture = getCurrentTexture();

	if (currentTexture)
	{
		// プレイヤーサイズを少し小さく描画（60x60相当）
		const double scale = 0.9375; // 64 * 0.9375 = 60
		const double rotation = getRotation();
		const ColorF tint = getTint();

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
		// フォールバック: 少し小さい円
		const ColorF fallbackColor = getTint();
		Circle(m_position, 23).draw(fallbackColor); // 25 -> 23に縮小
	}

	// デバッグ情報
#ifdef _DEBUG
	const String debugText = U"{} - {} - Inv:{} - {} - FB:{}/{} - Active:{}"_fmt(
		getColorString(),
		getStateString(),
		m_isInvincible ? U"Yes" : U"No",
		m_stats.trait,
		m_fireballCount,
		MAX_FIREBALLS_PER_STAGE,
		m_fireballs.size()
	);
	Font(16)(debugText).draw(m_position.x - 70, m_position.y - 60, ColorF(1.0, 1.0, 1.0));

	const String statsText = U"Move:{:.1f} Jump:{:.1f} Life:{} Size:60x60"_fmt(
		m_stats.moveSpeed, m_stats.jumpPower, m_stats.maxLife
	);
	Font(14)(statsText).draw(m_position.x - 70, m_position.y - 45, ColorF(0.8, 0.8, 1.0));

	// 衝突判定の可視化（開発用）
	const double PLAYER_SIZE = 60.0;
	const double halfSize = PLAYER_SIZE / 2.0;
	RectF(m_position.x - halfSize, m_position.y - halfSize, PLAYER_SIZE, PLAYER_SIZE)
		.drawFrame(2.0, ColorF(1.0, 0.0, 1.0, 0.5));

	// ファイアボールのデバッグ描画（ワールド座標）
	for (size_t i = 0; i < m_fireballs.size(); ++i)
	{
		if (m_fireballs[i].active)
		{
			const String fbInfo = U"FB{}:({:.0f},{:.0f})"_fmt(i, m_fireballs[i].position.x, m_fireballs[i].position.y);
			Font(12)(fbInfo).draw(m_position.x - 70, m_position.y - 30 + i * 15, ColorF(1.0, 1.0, 0.0));
		}
	}
#endif
}




void Player::setState(PlayerState newState)
{
	// 同じ状態への変更は無視
	if (m_currentState == newState) return;

	// ヒット状態中は一定時間他の状態変更を制限
	if (m_currentState == PlayerState::Hit && newState != PlayerState::Hit &&
		newState != PlayerState::Exploding && newState != PlayerState::Dead &&
		m_stateTimer < HIT_DURATION * 0.7)
	{
		return;
	}

	// 爆散・死亡状態からは他の状態に変更できない
	if (m_currentState == PlayerState::Exploding || m_currentState == PlayerState::Dead)
	{
		if (newState != PlayerState::Dead) return;
	}

	m_currentState = newState;
	m_stateTimer = 0.0;
	m_animationTimer = 0.0;
}

void Player::updatePhysics()
{
	// このメソッドは updateBasicPhysics() に置き換え
	updateBasicPhysics();
}

void Player::updateBasicPhysics()
{
	const double BLOCK_SIZE = 64.0;
	const double deltaTime = Scene::DeltaTime();

	// ★ 改善された重力システム
	if (!m_isGrounded)
	{
		// より自然な重力カーブ
		const double baseGravity = BLOCK_SIZE * 25.0;
		double currentGravity = baseGravity;

		// 上昇中は重力を弱める（ふわりとした感じ）
		if (m_velocity.y < 0.0)
		{
			currentGravity *= 0.7;
		}

		m_velocity.y += currentGravity * deltaTime;

		// より現実的な終端速度
		const double MAX_FALL_SPEED = BLOCK_SIZE * 12.0;
		if (m_velocity.y > MAX_FALL_SPEED)
		{
			m_velocity.y = MAX_FALL_SPEED;
		}
	}

	// 水平方向の最大速度制限
	const double MAX_HORIZONTAL_SPEED = BLOCK_SIZE * 8.0;
	if (std::abs(m_velocity.x) > MAX_HORIZONTAL_SPEED)
	{
		m_velocity.x = (m_velocity.x > 0) ? MAX_HORIZONTAL_SPEED : -MAX_HORIZONTAL_SPEED;
	}
}


void Player::handleInput()
{
	if (m_isExploding || m_currentState == PlayerState::Dead) return;

	const double BLOCK_SIZE = 64.0;

	// 入力状態の取得
	bool leftPressed = KeyLeft.pressed() || KeyA.pressed();
	bool rightPressed = KeyRight.pressed() || KeyD.pressed();
	bool downPressed = KeyDown.pressed() || KeyS.pressed();
	bool jumpPressed = KeySpace.pressed() || KeyUp.pressed() || KeyW.pressed();
	bool jumpDown = KeySpace.down() || KeyUp.down() || KeyW.down();
	bool hasHorizontalInput = leftPressed || rightPressed;

	// 方向設定
	if (leftPressed) setDirection(PlayerDirection::Left);
	if (rightPressed) setDirection(PlayerDirection::Right);

	// スライディング処理
	if (m_isSliding)
	{
		m_slideSpeed *= SLIDE_DECELERATION;
		const double slideVelocity = (m_slideDirection == PlayerDirection::Right) ? m_slideSpeed : -m_slideSpeed;
		m_velocity.x = slideVelocity;

		const double minSlideSpeed = BLOCK_SIZE * 0.5;
		if (m_slideSpeed < minSlideSpeed || !m_isGrounded || !downPressed)
		{
			m_isSliding = false;
			m_slideSpeed = 0.0;
		}

		if (KeyF.down()) fireFireball();
		return;
	}

	// スライディング開始判定
	if (downPressed && hasHorizontalInput && m_isGrounded && m_currentState != PlayerState::Hit)
	{
		m_isSliding = true;
		m_slideSpeed = BLOCK_SIZE * 7.0;
		m_slideDirection = rightPressed ? PlayerDirection::Right : PlayerDirection::Left;
		setState(PlayerState::Duck);

		if (KeyF.down()) fireFireball();
		return;
	}

	// ★ 改善されたジャンプ処理
	if (jumpDown && m_isGrounded && !m_isSliding && m_currentState != PlayerState::Hit)
	{
		jump();
	}

	// ★ 可変高さジャンプの改良
	if (!jumpPressed && m_velocity.y < 0.0 && !m_isGrounded)
	{
		// ジャンプボタンを離したら上昇力を大幅に減衰
		m_velocity.y *= 0.3;
	}

	// ★ 改善された水平移動処理 - 壁際の挙動を改良
	if (hasHorizontalInput && !downPressed)
	{
		double baseMoveSpeed = BLOCK_SIZE * 5.0;
		double moveSpeed = baseMoveSpeed * m_stats.moveSpeed;

		// 地上と空中で異なる制御
		if (m_isGrounded)
		{
			// ★ 修正: 地上での移動をより滑らかに
			double targetVelX = (rightPressed ? 1.0 : -1.0) * moveSpeed;

			// 現在の速度が0に近く、目標速度と逆方向の場合は即座に変更
			if (Math::Abs(m_velocity.x) < 10.0 ||
				(m_velocity.x > 0 && targetVelX < 0) ||
				(m_velocity.x < 0 && targetVelX > 0))
			{
				m_velocity.x = targetVelX * 0.7; // 即座に70%の速度で開始
			}
			else
			{
				// 通常の加速処理
				m_velocity.x = Math::Lerp(m_velocity.x, targetVelX, 0.25);
			}

			if (m_currentState != PlayerState::Hit)
			{
				setState(PlayerState::Walk);
			}
		}
		else
		{
			// ★ 修正: 空中での移動制御を改良
			double airMoveSpeed = moveSpeed * 0.75;
			double targetVelX = (rightPressed ? 1.0 : -1.0) * airMoveSpeed;

			// 空中では壁に当たっていても入力方向に少し力を加える
			double airControl = 0.12;
			m_velocity.x = Math::Lerp(m_velocity.x, targetVelX, airControl);
		}

		if (!m_tutorialNotifiedMove && Math::Abs(m_velocity.x) > (BLOCK_SIZE * 0.2))
		{
			m_tutorialNotifiedMove = true;
			TutorialEmit(TutorialEvent::MoveLeftRight, m_position);
		}
	}
	else if (!downPressed)
	{
		// ★ 修正: 入力がない場合の減速を改良
		if (m_isGrounded)
		{
			// 地上：強い摩擦だが、壁に当たっている時は即座に停止
			const double friction = (Math::Abs(m_velocity.x) > BASE_MOVE_SPEED * 0.1) ? 0.75 : 0.5;
			m_velocity.x *= friction;

			if (Math::Abs(m_velocity.x) < BLOCK_SIZE * 0.1)
			{
				m_velocity.x = 0.0;
			}

			if (m_currentState == PlayerState::Walk && Math::Abs(m_velocity.x) < BLOCK_SIZE * 0.5)
			{
				setState(PlayerState::Idle);
			}
		}
		else
		{
			m_velocity.x *= 0.98;  // 空中：非常に弱い空気抵抗
		}
	}

	// しゃがみ処理
	if (downPressed && !hasHorizontalInput && m_isGrounded && m_currentState != PlayerState::Hit)
	{
		setState(PlayerState::Duck);
		m_velocity.x *= 0.5;
	}
	else if (m_currentState == PlayerState::Duck && !downPressed && !m_isSliding)
	{
		setState(PlayerState::Idle);
	}

	// ファイアボール
	if (KeyF.down())
	{
		fireFireball();
	}
}


void Player::checkBasicGroundCollision()
{
	const double groundLevel = 850.0; // ステージの基本地面レベル

	if (m_position.y >= groundLevel)
	{
		m_position.y = groundLevel;
		m_velocity.y = 0.0;
		m_isGrounded = true;

		// ジャンプ状態から着地状態への変更
		if (m_currentState == PlayerState::Jump)
		{
			// 移動入力があるかチェック
			bool hasMovementInput = (KeyLeft.pressed() || KeyA.pressed() ||
									KeyRight.pressed() || KeyD.pressed());

			if (hasMovementInput)
			{
				setState(PlayerState::Walk);
			}
			else
			{
				setState(PlayerState::Idle);
			}
		}
	}
	else if (m_position.y < groundLevel - 10.0)
	{
		// 地面から離れている場合は接地状態を解除
		m_isGrounded = false;
	}
}

void Player::updateStateTransitions()
{
	if (m_isSliding) return;

	// ヒット状態の自動解除
	if (m_currentState == PlayerState::Hit && m_stateTimer >= HIT_DURATION)
	{
		if (m_isGrounded)
		{
			setState(PlayerState::Idle);
		}
		else
		{
			setState(PlayerState::Jump);
		}
	}

	// ★ より自然なジャンプ状態の判定
	const double JUMP_VELOCITY_THRESHOLD = -30.0;  // より低い閾値

	if (m_velocity.y < JUMP_VELOCITY_THRESHOLD && !m_isGrounded)
	{
		if (!m_wasJumping)
		{
			m_jumpStateTimer = JUMP_STATE_DURATION;
			m_wasJumping = true;
		}
	}
	else if (m_isGrounded)
	{
		m_wasJumping = false;
		m_jumpStateTimer = 0.0;
	}

	// ★ 改善された状態遷移ロジック
	if (!m_isGrounded)
	{
		// 空中にいる場合はジャンプ状態
		if (m_currentState != PlayerState::Jump &&
			m_currentState != PlayerState::Hit)
		{
			setState(PlayerState::Jump);
		}
	}
	else if (m_isGrounded && m_currentState == PlayerState::Jump)
	{
		// 着地時の状態遷移
		bool downPressed = KeyDown.pressed() || KeyS.pressed();
		bool hasHorizontalInput = KeyLeft.pressed() || KeyA.pressed() ||
			KeyRight.pressed() || KeyD.pressed();

		if (downPressed)
		{
			setState(PlayerState::Duck);
		}
		else if (hasHorizontalInput)
		{
			setState(PlayerState::Walk);
		}
		else
		{
			setState(PlayerState::Idle);
		}
	}

	// ジャンプ状態タイマーの更新
	if (m_jumpStateTimer > 0.0)
	{
		m_jumpStateTimer -= Scene::DeltaTime();
	}
}

void Player::updateGroundStateTransitions()
{
	// 爆散中や死亡中は状態変更しない
	if (m_currentState == PlayerState::Exploding || m_currentState == PlayerState::Dead)
	{
		return;
	}

	// ヒット状態中は一定時間変更を制限
	if (m_currentState == PlayerState::Hit && m_stateTimer < HIT_DURATION * 0.7)
	{
		return;
	}

	// 接地状態に応じた遷移
	if (m_isGrounded)
	{
		// 地上での状態遷移
		if (m_currentState == PlayerState::Jump)
		{
			// ジャンプから着地
			bool hasMovementInput = (KeyLeft.pressed() || KeyA.pressed() ||
									KeyRight.pressed() || KeyD.pressed());
			bool isDucking = (KeyDown.pressed() || KeyS.pressed());

			if (isDucking)
			{
				setState(PlayerState::Duck);
			}
			else if (hasMovementInput)
			{
				setState(PlayerState::Walk);
			}
			else
			{
				setState(PlayerState::Idle);
			}
		}
	}
	else
	{
		// 空中での状態遷移
		if (m_currentState != PlayerState::Jump &&
			m_currentState != PlayerState::Hit &&
			m_currentState != PlayerState::Exploding &&
			m_currentState != PlayerState::Dead)
		{
			setState(PlayerState::Jump);
		}
	}
}
void Player::updateInvincibility()
{
	if (m_isInvincible)
	{
		m_invincibleTimer += Scene::DeltaTime();

		// 無敵時間が終了したら無敵状態を解除（特性を適用した時間）
		if (m_invincibleTimer >= getActualInvincibleDuration())
		{
			m_isInvincible = false;
			m_invincibleTimer = 0.0;
		}
	}
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
	case PlayerState::Sliding:  // ★ 追加
		textureKey = U"duck";   // しゃがみテクスチャを流用（専用テクスチャがあれば変更）
		break;
	case PlayerState::Hit:
		textureKey = U"hit";
		break;
	case PlayerState::Climb:
	{
		const bool useVariantA = std::fmod(m_animationTimer, CLIMB_ANIMATION_SPEED * 2) < CLIMB_ANIMATION_SPEED;
		textureKey = useVariantA ? U"climb_a" : U"climb_b";
	}
	break;
	case PlayerState::Exploding:
	case PlayerState::Dead:
		return Texture{};
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
	// 無敵状態の場合は点滅効果
	if (m_isInvincible)
	{
		const double blinkRate = 10.0;  // 点滅の速度
		const double alpha = (std::sin(m_invincibleTimer * blinkRate) + 1.0) * 0.5;
		return ColorF(1.0, 1.0, 1.0, 0.3 + alpha * 0.7);  // 透明度で点滅
	}

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

void Player::jump()
{
	if (!m_isGrounded) return;

	const double BLOCK_SIZE = 64.0;

	// ★ より自然なジャンプ力の計算
	double baseJumpPower = BLOCK_SIZE * 12.0;  // 基本値を少し下げる
	double jumpPower = baseJumpPower * m_stats.jumpPower;

	m_velocity.y = -jumpPower;
	if (!m_tutorialNotifiedJump /* && いまジャンプが成立した */) {
		m_tutorialNotifiedJump = true;
		TutorialEmit(TutorialEvent::Jump, m_position);
	}
	setState(PlayerState::Jump);
	m_isGrounded = false;

	// ジャンプ状態タイマーを設定
	m_jumpStateTimer = JUMP_STATE_DURATION;
	m_wasJumping = true;

	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_JUMP);
}

void Player::hit()
{
	// 無敵状態の場合はダメージを受けない
	if (m_isInvincible)
	{
		return;
	}

	setState(PlayerState::Hit);

	// 無敵状態を開始
	m_isInvincible = true;
	m_invincibleTimer = 0.0;

	// ダメージ音を再生
	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_HURT);

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

// 爆散開始メソッド
void Player::startExplosion()
{
	if (m_isExploding || m_currentState == PlayerState::Dead) return;

	m_isExploding = true;
	m_explosionTimer = 0.0;
	m_deathTimer = 0.0;
	setState(PlayerState::Exploding);

	// 移動を停止
	m_velocity = Vec2::Zero();

	// 爆散パーティクルを生成
	createExplosionParticles();
	createShockwaves();

	// 爆散音を再生
	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_DISAPPEAR);
}

// 爆散状態の更新
void Player::updateExplosion()
{
	const double deltaTime = Scene::DeltaTime();

	m_explosionTimer += deltaTime;
	m_deathTimer += deltaTime;

	// パーティクルとエフェクトの更新
	updateExplosionParticles();
	updateShockwaves();

	// 爆散演出が終了したら死亡状態に移行
	if (m_explosionTimer >= EXPLOSION_DURATION)
	{
		m_isExploding = false;
		setState(PlayerState::Dead);
	}
}

// 爆散パーティクルの生成
void Player::createExplosionParticles()
{
	m_explosionParticles.clear();

	for (int i = 0; i < EXPLOSION_PARTICLE_COUNT; ++i)
	{
		// ランダムな方向と速度
		const double angle = Random(0.0, Math::TwoPi);
		const double speed = Random(100.0, 300.0);
		const Vec2 velocity = Vec2(std::cos(angle), std::sin(angle)) * speed;

		// プレイヤーの色に応じたパーティクル色
		ColorF particleColor;
		switch (m_color)
		{
		case PlayerColor::Green:  particleColor = ColorF(0.3, 1.0, 0.3); break;
		case PlayerColor::Pink:   particleColor = ColorF(1.0, 0.5, 0.8); break;
		case PlayerColor::Purple: particleColor = ColorF(0.8, 0.3, 1.0); break;
		case PlayerColor::Beige:  particleColor = ColorF(0.9, 0.8, 0.6); break;
		case PlayerColor::Yellow: particleColor = ColorF(1.0, 1.0, 0.3); break;
		default: particleColor = ColorF(1.0, 1.0, 1.0); break;
		}

		// 少しランダム性を加える
		particleColor.r += Random(-0.2, 0.2);
		particleColor.g += Random(-0.2, 0.2);
		particleColor.b += Random(-0.2, 0.2);
		particleColor.r = Math::Clamp(particleColor.r, 0.0, 1.0);
		particleColor.g = Math::Clamp(particleColor.g, 0.0, 1.0);
		particleColor.b = Math::Clamp(particleColor.b, 0.0, 1.0);

		// パーティクルの初期位置（プレイヤー周辺）
		const Vec2 startPos = m_position + Vec2(Random(-15.0, 15.0), Random(-20.0, 10.0));

		m_explosionParticles.emplace_back(startPos, velocity, particleColor);
	}
}

// 衝撃波の生成
void Player::createShockwaves()
{
	m_shockwaves.clear();
	m_shockwaveTimers.clear();

	// 複数の衝撃波を時間差で生成
	for (int i = 0; i < 3; ++i)
	{
		m_shockwaves.push_back(m_position);
		m_shockwaveTimers.push_back(i * 0.15); // 0.15秒間隔
	}
}

// 爆散パーティクルの更新
void Player::updateExplosionParticles()
{
	const double deltaTime = Scene::DeltaTime();

	for (auto it = m_explosionParticles.begin(); it != m_explosionParticles.end();)
	{
		auto& particle = *it;

		// 物理更新
		particle.position += particle.velocity * deltaTime;
		particle.velocity.y += PARTICLE_GRAVITY * deltaTime; // 重力適用
		particle.velocity *= 0.98; // 空気抵抗

		// 回転更新
		particle.rotation += particle.rotationSpeed * deltaTime;

		// 生存時間更新
		particle.life -= deltaTime;

		// サイズの変化
		particle.size *= 0.995;

		// 寿命が尽きたパーティクルを削除
		if (particle.life <= 0.0 || particle.size <= 0.5)
		{
			it = m_explosionParticles.erase(it);
		}
		else
		{
			++it;
		}
	}
}

// 衝撃波の更新
void Player::updateShockwaves()
{
	const double deltaTime = Scene::DeltaTime();

	for (size_t i = 0; i < m_shockwaveTimers.size();)
	{
		m_shockwaveTimers[i] += deltaTime;

		// 衝撃波の寿命（1秒）
		if (m_shockwaveTimers[i] > 1.0)
		{
			m_shockwaves.erase(m_shockwaves.begin() + i);
			m_shockwaveTimers.erase(m_shockwaveTimers.begin() + i);
		}
		else
		{
			++i;
		}
	}
}

// 爆散エフェクトの描画
void Player::drawExplosionEffect() const
{
	drawShockwaves();
	drawExplosionParticles();

	// 中央の爆発光
	const double progress = m_explosionTimer / EXPLOSION_DURATION;
	const double flashAlpha = Math::Max(0.0, 1.0 - progress);

	if (flashAlpha > 0.0)
	{
		// メイン爆発光
		const double explosionRadius = 60.0 + progress * 40.0;
		Circle(m_position, explosionRadius).draw(ColorF(1.0, 0.8, 0.2, flashAlpha * 0.3));
		Circle(m_position, explosionRadius * 0.7).draw(ColorF(1.0, 0.9, 0.4, flashAlpha * 0.5));
		Circle(m_position, explosionRadius * 0.4).draw(ColorF(1.0, 1.0, 0.8, flashAlpha * 0.7));

		// 内側の白い光
		Circle(m_position, explosionRadius * 0.2).draw(ColorF(1.0, 1.0, 1.0, flashAlpha * 0.9));
	}
}

// 爆散パーティクルの描画
void Player::drawExplosionParticles() const
{
	for (const auto& particle : m_explosionParticles)
	{
		const double alpha = particle.life / particle.maxLife;
		const ColorF color = ColorF(particle.color.r, particle.color.g, particle.color.b, alpha);

		// パーティクルを回転する矩形として描画
		const double size = particle.size;
		const RectF particleRect(particle.position.x - size / 2, particle.position.y - size / 2, size, size);

		// 簡易的な回転描画（矩形）
		RectF(particleRect).draw(color);

		// 光る効果
		Circle(particle.position, size * 0.6).draw(ColorF(color.r, color.g, color.b, alpha * 0.3));
	}
}

// 衝撃波の描画
void Player::drawShockwaves() const
{
	for (size_t i = 0; i < m_shockwaves.size(); ++i)
	{
		const double timer = m_shockwaveTimers[i];
		if (timer < 0.0) continue;

		const Vec2 center = m_shockwaves[i];
		const double radius = timer * 150.0; // 衝撃波の拡散速度
		const double alpha = Math::Max(0.0, 1.0 - timer);

		// 衝撃波リング
		Circle(center, radius).drawFrame(4.0, ColorF(1.0, 0.6, 0.2, alpha * 0.6));
		Circle(center, radius).drawFrame(2.0, ColorF(1.0, 0.9, 0.6, alpha * 0.8));
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
	case PlayerState::Idle:      return U"Idle";
	case PlayerState::Front:     return U"Front";
	case PlayerState::Walk:      return U"Walk";
	case PlayerState::Jump:      return U"Jump";
	case PlayerState::Duck:      return U"Duck";
	case PlayerState::Sliding:   return U"Sliding";  // ★ 追加
	case PlayerState::Hit:       return U"Hit";
	case PlayerState::Climb:     return U"Climb";
	case PlayerState::Exploding: return U"Exploding";
	case PlayerState::Dead:      return U"Dead";
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

// ★ 新しいファイアボール関連メソッドの実装
void Player::fireFireball()
{
	// 残り回数チェック
	if (m_fireballCount >= MAX_FIREBALLS_PER_STAGE)
	{
		return;
	}

	// 爆散中や死亡中は発射できない
	if (m_isExploding || m_currentState == PlayerState::Dead)
	{
		return;
	}

	// ファイアボール作成
	Vec2 fireballPos = m_position + Vec2(0, -10); // プレイヤーの少し上から発射
	Vec2 fireballVel;

	if (m_direction == PlayerDirection::Left)
	{
		fireballVel = Vec2(-FIREBALL_SPEED, -100.0); // 少し上向きに発射
		fireballPos.x -= 30; // 左向きの場合は少し左にオフセット
	}
	else
	{
		fireballVel = Vec2(FIREBALL_SPEED, -100.0);
		fireballPos.x += 30; // 右向きの場合は少し右にオフセット
	}

	// ファイアボールを配列に追加
	m_fireballs.emplace_back(fireballPos, fireballVel);
	m_fireballCount++;

	// ファイアボール発射音を再生
	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_JUMP);
}

void Player::updateFireballs()
{
	const double deltaTime = Scene::DeltaTime();

	for (auto it = m_fireballs.begin(); it != m_fireballs.end();)
	{
		if (!it->active)
		{
			it = m_fireballs.erase(it);
			continue;
		}

		// 物理更新
		it->velocity.y += FIREBALL_GRAVITY * deltaTime;
		it->position += it->velocity * deltaTime;

		// 回転更新
		it->rotation += it->rotationSpeed * deltaTime;

		// 寿命更新
		it->lifetime += deltaTime;
		if (it->lifetime >= it->maxLifetime)
		{
			it->active = false;
		}

		// 画面外チェック（ワールド座標基準）
		const double worldBoundary = 6000.0; // ステージの大体の幅
		if (it->position.y > 1200 ||  // 画面下
			it->position.x < -200 ||  // 画面左
			it->position.x > worldBoundary) // 画面右
		{
			it->active = false;
		}

		// ★ ステージとの衝突判定を追加（地面や壁に当たったら消滅）
		// これはGameSceneで処理されるべきですが、基本的な地面衝突はここで処理
		if (it->position.y > 1000) // 地面に当たった場合
		{
			it->active = false;
		}

		++it;
	}
}

void Player::drawFireballs() const
{
	// Player::draw()内ではファイアボールを描画しない
	// GameScene::drawPlayerFireballs()で描画するため

#ifdef _DEBUG
	// デバッグ時のみローカル座標で描画
	if (!m_fireballTexture) return;

	for (const auto& fireball : m_fireballs)
	{
		if (fireball.active)
		{
			// 回転しながら描画（ワールド座標そのまま）
			if (m_fireballTexture)
			{
				m_fireballTexture.rotated(fireball.rotation).drawAt(fireball.position);
			}
			else
			{
				// フォールバック
				Circle(fireball.position, 16).draw(ColorF(1.0, 0.5, 0.0));
			}

			// 軌跡エフェクト
			const double trailAlpha = 0.3;
			Circle(fireball.position, 8).draw(ColorF(1.0, 0.5, 0.0, trailAlpha));
		}
	}
#endif
}

void Player::deactivateFireball(const Vec2& position)
{
	for (auto& fireball : m_fireballs)
	{
		if (fireball.active)
		{
			const double distance = fireball.position.distanceFrom(position);
			if (distance < 32.0) // 32ピクセル以内のファイアボールを無効化
			{
				fireball.active = false;
				break; // 最初に見つかった一つだけ無効化
			}
		}
	}
}



void Player::startSliding(PlayerDirection direction)
{
	if (!m_isGrounded) return;

	m_isSliding = true;
	m_slideTimer = 0.0;
	m_slideDirection = direction;
	m_slideSpeed = SLIDE_SPEED;

	// しゃがみ状態にする
	setState(PlayerState::Duck);

	// スライディング方向に初速を与える
	const double slideVelocity = (direction == PlayerDirection::Right) ? m_slideSpeed : -m_slideSpeed;
	m_velocity.x = slideVelocity;
}

void Player::updateSliding()
{
	const double deltaTime = Scene::DeltaTime();
	m_slideTimer += deltaTime;

	// スライディング速度の減速
	m_slideSpeed *= SLIDE_DECELERATION;

	// 速度を更新
	const double slideVelocity = (m_slideDirection == PlayerDirection::Right) ? m_slideSpeed : -m_slideSpeed;
	m_velocity.x = slideVelocity;

	// スライディング終了条件
	if (m_slideTimer >= SLIDE_DURATION || m_slideSpeed < 50.0 || !m_isGrounded)
	{
		endSliding();
	}
}



void Player::endSliding()
{
	m_isSliding = false;
	m_slideTimer = 0.0;
	m_slideSpeed = 0.0;

	// スライディング終了後もDuck状態を維持
	// 下キーが離されたときのみ通常の状態遷移を許可
	// Duck状態のままにしておく
}

// ★ 前フレームから上に移動しているかの判定メソッドを追加
bool Player::wasMovingUpward() const
{
	return m_position.y < m_previousPosition.y - 1.0;
}
