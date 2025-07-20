#include "Ladybug.hpp"

Ladybug::Ladybug(const Vec2& startPosition)
	: EnemyBase(EnemyType::Ladybug, startPosition)
	, m_modeTimer(0.0)
	, m_patrolDistance(200.0)
	, m_startPosition(startPosition)
	, m_flyTargetPosition(startPosition)
	, m_isFlyMode(false)
	, m_hasHitWall(false)
	, m_isFlattened(false)
	, m_flattenedTimer(0.0)
{
	// Ladybug固有の設定
	m_moveSpeed = WALK_SPEED;
	m_collisionWidth = 64.0;
	m_collisionHeight = 64.0;

	init();
}

void Ladybug::init()
{
	loadTextures();
	setState(EnemyState::Walk);
	startWalking();
	updateCollisionRect();
}

void Ladybug::update()
{
	if (!m_isActive || !m_isAlive || m_state == EnemyState::Dead) return;

	updateAnimation();

	switch (m_state)
	{
	case EnemyState::Walk:
		if (m_isFlyMode)
		{
			updateFlyBehavior();
		}
		else
		{
			updateWalkBehavior();
		}
		break;
	case EnemyState::Flattened:
		updateFlattenedBehavior();
		break;
	case EnemyState::Hit:
		updateHitBehavior();
		break;
	case EnemyState::Dead:
		return;
	default:
		break;
	}

	updatePhysics();
}

void Ladybug::draw() const
{
	if (!m_isActive || m_state == EnemyState::Dead) return;

	// 飛行時のトレイル効果
	if (m_isFlyMode && m_state != EnemyState::Flattened)
	{
		drawFlyTrail();
	}

	const Texture currentTexture = getCurrentTexture();

	if (currentTexture)
	{
		if (m_direction == EnemyDirection::Left)
		{
			currentTexture.mirrored().drawAt(m_position);
		}
		else
		{
			currentTexture.drawAt(m_position);
		}
	}
	else
	{
		const ColorF fallbackColor = ColorF(0.8, 0.2, 0.2);  // 赤色
		Circle(m_position, 32).draw(fallbackColor);
	}

	// エフェクト描画
	if (m_state == EnemyState::Flattened)
	{
		drawFlattenedEffect();
	}
	else if (m_state == EnemyState::Hit)
	{
		drawHitEffect();
	}

#ifdef _DEBUG
	m_collisionRect.drawFrame(2.0, ColorF(1.0, 0.5, 0.0, 0.6));
	if (m_isFlyMode)
	{
		Line(m_position, m_flyTargetPosition).draw(2.0, ColorF(0.0, 1.0, 1.0, 0.5));
	}
#endif
}

void Ladybug::loadTextures()
{
	m_textures.clear();

	const String basePath = U"Sprites/Enemies/";
	Array<String> textureNames = {
		U"ladybug_rest",
		U"ladybug_fly",
		U"ladybug_walk_a",
		U"ladybug_walk_b"
	};

	for (const auto& textureName : textureNames)
	{
		const String filepath = basePath + textureName + U".png";
		Texture texture(filepath);

		if (texture)
		{
			m_textures[textureName] = texture;
		}
		else
		{
			Print << U"Failed to load texture: " << filepath;
		}
	}
}

void Ladybug::setState(EnemyState newState)
{
	if (m_state == EnemyState::Dead) return;

	EnemyBase::setState(newState);

	switch (newState)
	{
	case EnemyState::Walk:
		m_isFlattened = false;
		if (m_isFlyMode)
		{
			startFlying();
		}
		else
		{
			startWalking();
		}
		break;
	case EnemyState::Flattened:
		m_isFlattened = true;
		m_isFlyMode = false;  // 踏まれたら飛行モード終了
		m_flattenedTimer = 0.0;
		m_velocity.x = 0.0;
		m_velocity.y = 0.0;
		break;
	case EnemyState::Hit:
		m_velocity.x *= 0.3;
		break;
	case EnemyState::Dead:
		m_isAlive = false;
		m_isActive = false;
		m_velocity.x = 0.0;
		m_velocity.y = 0.0;
		break;
	default:
		break;
	}
}

void Ladybug::onHit()
{
	EnemyBase::onHit();
}

void Ladybug::onStomp()
{
	// 既にFlattened状態または死亡状態の場合は何もしない
	if (m_state == EnemyState::Flattened || m_state == EnemyState::Dead)
	{
		return;
	}

	setState(EnemyState::Flattened);
}

void Ladybug::startWalking()
{
	if (m_state == EnemyState::Walk && !m_isFlyMode)
	{
		m_moveSpeed = WALK_SPEED;
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
		m_velocity.y = 0.0;  // 地上歩行
	}
}

void Ladybug::startFlying()
{
	if (m_state == EnemyState::Walk && m_isFlyMode)
	{
		m_moveSpeed = FLY_SPEED;
		calculateFlyTarget();
	}
}

void Ladybug::updateMovement()
{
	m_modeTimer += Scene::DeltaTime();

	// モード切り替え
	if (m_isFlyMode && m_modeTimer >= FLY_MODE_DURATION)
	{
		switchMode();
	}
	else if (!m_isFlyMode && m_modeTimer >= WALK_MODE_DURATION)
	{
		switchMode();
	}

	// パトロール範囲チェック
	const double distanceFromStart = std::abs(m_position.x - m_startPosition.x);
	if (distanceFromStart > m_patrolDistance)
	{
		changeDirection();
	}
}

void Ladybug::changeDirection()
{
	m_direction = (m_direction == EnemyDirection::Left) ? EnemyDirection::Right : EnemyDirection::Left;

	if (m_state == EnemyState::Walk)
	{
		if (m_isFlyMode)
		{
			calculateFlyTarget();
		}
		else
		{
			const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
			m_velocity.x = speed;
		}
	}
}

void Ladybug::switchMode()
{
	m_isFlyMode = !m_isFlyMode;
	m_modeTimer = 0.0;

	if (m_state == EnemyState::Walk)
	{
		if (m_isFlyMode)
		{
			startFlying();
		}
		else
		{
			startWalking();
		}
	}
}

void Ladybug::calculateFlyTarget()
{
	const double targetX = m_startPosition.x + (m_direction == EnemyDirection::Left ? -m_patrolDistance : m_patrolDistance);
	const double targetY = m_startPosition.y - FLY_HEIGHT;
	m_flyTargetPosition = Vec2(targetX, targetY);
}

void Ladybug::updateWalkBehavior()
{
	updateMovement();
}

void Ladybug::updateFlyBehavior()
{
	updateMovement();

	// 飛行ターゲットに向かって移動
	const Vec2 direction = m_flyTargetPosition - m_position;
	const double distance = direction.length();

	if (distance > 10.0)
	{
		const Vec2 normalizedDirection = direction.normalized();
		m_velocity = normalizedDirection * m_moveSpeed * Scene::DeltaTime() * 60.0;
	}
	else
	{
		// ターゲットに到達したら新しいターゲットを計算
		changeDirection();
	}
}

void Ladybug::updateFlattenedBehavior()
{
	m_flattenedTimer += Scene::DeltaTime();

	if (m_flattenedTimer >= FLATTENED_DURATION)
	{
		m_state = EnemyState::Dead;
		m_isAlive = false;
		m_isActive = false;
	}
}

void Ladybug::updateHitBehavior()
{
	if (m_stateTimer >= HIT_DURATION)
	{
		setState(EnemyState::Walk);
	}
}

Texture Ladybug::getCurrentTexture() const
{
	String textureKey;

	switch (m_state)
	{
	case EnemyState::Idle:
		textureKey = U"ladybug_rest";
		break;
	case EnemyState::Walk:
		if (m_isFlyMode)
		{
			textureKey = U"ladybug_fly";
		}
		else
		{
			const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
			textureKey = useVariantA ? U"ladybug_walk_a" : U"ladybug_walk_b";
		}
		break;
	case EnemyState::Flattened:
		textureKey = U"ladybug_rest";
		break;
	case EnemyState::Hit:
		textureKey = U"ladybug_rest";
		break;
	case EnemyState::Dead:
		textureKey = U"ladybug_rest";
		break;
	default:
		textureKey = U"ladybug_rest";
		break;
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else if (m_textures.contains(U"ladybug_rest"))
	{
		return m_textures.at(U"ladybug_rest");
	}
	return Texture{};
}

void Ladybug::drawFlyTrail() const
{
	// 飛行時のトレイル効果
	const double time = Scene::Time();

	for (int i = 0; i < 5; ++i)
	{
		const double trailAlpha = 0.3 - i * 0.05;
		const double trailOffset = i * 8.0;
		const Vec2 trailPos = m_position + Vec2(-trailOffset * (m_direction == EnemyDirection::Left ? -1 : 1), 0);

		Circle(trailPos, 3.0 - i * 0.5).draw(ColorF(1.0, 0.8, 0.2, trailAlpha));
	}

	// 羽ばたき効果
	const double wingBeat = std::sin(time * 20.0) * 0.3;
	Circle(m_position, 25.0 + wingBeat).draw(ColorF(1.0, 1.0, 1.0, 0.1));
}

void Ladybug::drawFlattenedEffect() const
{
	const double effectTime = m_flattenedTimer;

	for (int i = 0; i < 6; ++i)
	{
		const double angle = i * Math::TwoPi / 6.0 + effectTime * 3.0;
		const double distance = 25.0 + std::sin(effectTime * 10.0) * 8.0;
		const Vec2 starPos = m_position + Vec2(std::cos(angle), std::sin(angle)) * distance;
		const double alpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);

		const double starSize = 3.0;
		Line(starPos.x - starSize, starPos.y, starPos.x + starSize, starPos.y).draw(2.0, ColorF(1.0, 0.5, 0.0, alpha));
		Line(starPos.x, starPos.y - starSize, starPos.x, starPos.y + starSize).draw(2.0, ColorF(1.0, 0.5, 0.0, alpha));
		Circle(starPos, 1.5).draw(ColorF(1.0, 0.8, 0.2, alpha));
	}

	const double ringRadius = effectTime * 40.0;
	const double ringAlpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);
	Circle(m_position, ringRadius).drawFrame(2.0, ColorF(1.0, 0.5, 0.0, ringAlpha * 0.5));
}

void Ladybug::drawHitEffect() const
{
	const double flashRate = 15.0;
	const double alpha = (std::sin(m_stateTimer * flashRate) + 1.0) * 0.2;

	if (alpha > 0.1)
	{
		Circle(m_position, 35).draw(ColorF(1.0, 0.3, 0.0, alpha));
	}
}
