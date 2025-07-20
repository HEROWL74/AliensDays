#include "Player.hpp"
#include "SoundManager.hpp"

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
	, m_fireballCount(0)  // ★ 初期化追加
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
{
	// パーティクル配列もクリア
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

	m_animationTimer += deltaTime;
	m_stateTimer += deltaTime;

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

	// 通常のプレイヤー描画のみ
	const Texture currentTexture = getCurrentTexture();

	if (currentTexture)
	{
		const double scale = getScale();
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
		const ColorF fallbackColor = getTint();
		Circle(m_position, 25).draw(fallbackColor);
	}

	// デバッグ情報（開発時のみ）
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

	const String statsText = U"Move:{:.1f} Jump:{:.1f} Life:{}"_fmt(
		m_stats.moveSpeed, m_stats.jumpPower, m_stats.maxLife
	);
	Font(14)(statsText).draw(m_position.x - 70, m_position.y - 45, ColorF(0.8, 0.8, 1.0));

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

void Player::handleInput()
{
	// 爆散中や死亡中は入力を受け付けない
	if (m_isExploding || m_currentState == PlayerState::Dead) return;

	// 移動入力
	Vec2 inputVector(0, 0);
	bool hasMovementInput = false;

	if (KeyLeft.pressed() || KeyA.pressed())
	{
		inputVector.x = -1;
		setDirection(PlayerDirection::Left);
		hasMovementInput = true;
	}
	if (KeyRight.pressed() || KeyD.pressed())
	{
		inputVector.x = 1;
		setDirection(PlayerDirection::Right);
		hasMovementInput = true;
	}

	// 移動処理
	if (inputVector.x != 0.0)
	{
		double moveSpeed = getActualMoveSpeed();

		if (!m_isGrounded)
		{
			moveSpeed *= AIR_CONTROL;
		}

		m_velocity.x = inputVector.x * moveSpeed;

		// ★ 修正: 状態変更をより慎重に
		if (m_isGrounded &&
			m_currentState != PlayerState::Duck &&
			m_currentState != PlayerState::Hit &&
			m_currentState != PlayerState::Jump)
		{
			setState(PlayerState::Walk);
		}
	}
	else
	{
		if (m_isGrounded)
		{
			m_velocity.x = 0.0;

			// ★ 修正: アイドル状態への変更をより慎重に
			if (m_currentState == PlayerState::Walk &&
				m_currentState != PlayerState::Hit &&
				m_currentState != PlayerState::Duck)
			{
				setState(PlayerState::Idle);
			}
		}
		else
		{
			m_velocity.x *= 0.95;
		}
	}

	// ジャンプ - 条件をシンプルに
	if ((KeySpace.down() || KeyUp.down() || KeyW.down()) && m_isGrounded)
	{
		jump();
	}

	// しゃがみ
	if (KeyDown.pressed() || KeyS.pressed())
	{
		if (m_isGrounded && m_currentState != PlayerState::Hit)
		{
			duck();
		}
	}
	else if (m_currentState == PlayerState::Duck)
	{
		setState(PlayerState::Idle);
	}

	// ファイアボール発射
	if (KeyF.down())
	{
		fireFireball();
	}
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
	const double deltaTime = Scene::DeltaTime();

	// 重力適用（地上にいない場合のみ）
	if (!m_isGrounded)
	{
		applyGravity();
	}

	// ★ 重要修正: 位置更新は CollisionSystem で処理されるため、ここでは速度更新のみ
	m_position += m_velocity * deltaTime;  // この行をコメントアウト

	// ★ フォールバック地面衝突判定は CollisionSystem に任せるため削除
	 //checkBasicGroundCollision(); 

	// 速度の減衰処理（空中での横移動制御）
	if (!m_isGrounded)
	{
		m_velocity.x *= 0.98;  // 空中での横移動減衰
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
	// ヒット状態の自動解除
	if (m_currentState == PlayerState::Hit && m_stateTimer >= HIT_DURATION)
	{
		if (m_isGrounded)
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
		else
		{
			setState(PlayerState::Jump);
		}
	}

	// ★ 新規: 地面状態に応じた状態遷移（CollisionSystemが更新した後）
	updateGroundStateTransitions();
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
	case PlayerState::Exploding:
	case PlayerState::Dead:
		// 爆散・死亡状態では通常テクスチャは表示しない
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
	if (m_isGrounded)
	{
		m_velocity.y = -getActualJumpPower();
		setState(PlayerState::Jump);
		m_isGrounded = false;  
		SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_JUMP);
	}
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
