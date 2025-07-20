#include "Bee.hpp"

Bee::Bee(const Vec2& startPosition)
	: EnemyBase(EnemyType::Bee, startPosition)
	, m_hoverTimer(0.0)
	, m_patrolAngle(0.0)
	, m_startPosition(startPosition)
	, m_targetPosition(startPosition)
	, m_isChasingPlayer(false)
	, m_hasHitWall(false)
	, m_isFlattened(false)
	, m_flattenedTimer(0.0)
	, m_isFlying(true)
{
	// Bee固有の設定
	m_moveSpeed = FLY_SPEED;
	m_collisionWidth = 64.0;
	m_collisionHeight = 64.0;
	m_gravity = 0.0;  // Beeは重力の影響を受けない

	init();
}

void Bee::init()
{
	loadTextures();
	setState(EnemyState::Walk);  // 飛行状態を表現
	startFlying();
	updateCollisionRect();
}

void Bee::update()
{
	if (!m_isActive || !m_isAlive || m_state == EnemyState::Dead) return;

	updateAnimation();

	switch (m_state)
	{
	case EnemyState::Walk:  // 飛行状態
		updateFlyBehavior();
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

	// Beeは重力の影響を受けないので、独自の物理更新
	if (m_isFlying)
	{
		const double deltaTime = Scene::DeltaTime();
		m_position += m_velocity * deltaTime;
		updateCollisionRect();
	}
	else
	{
		updatePhysics();  // 地面に落ちた場合は通常の物理
	}
}

void Bee::draw() const
{
	if (!m_isActive || m_state == EnemyState::Dead) return;

	// 飛行トレイル効果
	if (m_isFlying)
	{
		drawFlyTrail();
		drawWingEffect();
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
		const ColorF fallbackColor = ColorF(1.0, 1.0, 0.0);  // 黄色
		Circle(m_position, 32).draw(fallbackColor);

		// 簡単な羽の表現
		if (m_isFlying)
		{
			const double wingBeat = std::sin(Scene::Time() * 25.0) * 8.0;
			Line(m_position.x - 20, m_position.y - 10 + wingBeat,
				 m_position.x - 10, m_position.y - 15 + wingBeat).draw(2.0, ColorF(0.8, 0.8, 0.8, 0.7));
			Line(m_position.x + 10, m_position.y - 15 + wingBeat,
				 m_position.x + 20, m_position.y - 10 + wingBeat).draw(2.0, ColorF(0.8, 0.8, 0.8, 0.7));
		}
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
	m_collisionRect.drawFrame(2.0, ColorF(1.0, 1.0, 0.0, 0.6));
	Circle(m_startPosition, PATROL_RADIUS).drawFrame(1.0, ColorF(0.0, 1.0, 0.0, 0.3));
#endif
}

void Bee::loadTextures()
{
	m_textures.clear();

	const String basePath = U"Sprites/Enemies/";
	Array<String> textureNames = {
		U"bee_rest",
		U"bee_a",
		U"bee_b"
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

void Bee::setState(EnemyState newState)
{
	if (m_state == EnemyState::Dead) return;

	EnemyBase::setState(newState);

	switch (newState)
	{
	case EnemyState::Walk:
		m_isFlattened = false;
		m_isFlying = true;
		startFlying();
		break;
	case EnemyState::Flattened:
		m_isFlattened = true;
		m_isFlying = false;
		m_flattenedTimer = 0.0;
		m_velocity.x = 0.0;
		m_velocity.y = 0.0;
		m_gravity = 600.0;  // 地面に落ちる
		break;
	case EnemyState::Hit:
		m_velocity *= 0.5;
		break;
	case EnemyState::Dead:
		m_isAlive = false;
		m_isActive = false;
		m_isFlying = false;
		m_velocity.x = 0.0;
		m_velocity.y = 0.0;
		break;
	default:
		break;
	}
}

void Bee::onHit()
{
	EnemyBase::onHit();
}

void Bee::onStomp()
{
	if (m_state != EnemyState::Dead)
	{
		setState(EnemyState::Flattened);
	}
}

void Bee::startFlying()
{
	if (m_state == EnemyState::Walk && m_isFlying)
	{
		calculatePatrolTarget();
	}
}

void Bee::updateMovement()
{
	if (!m_isFlying) return;

	updatePatrol();

	// ホバリング効果
	m_hoverTimer += Scene::DeltaTime();
	const double hoverOffset = std::sin(m_hoverTimer * HOVER_SPEED) * HOVER_AMPLITUDE;

	// ターゲットに向かって移動
	const Vec2 direction = m_targetPosition - m_position;
	const double distance = direction.length();

	if (distance > 20.0)
	{
		const Vec2 normalizedDirection = direction.normalized();
		m_velocity = normalizedDirection * m_moveSpeed * Scene::DeltaTime() * 60.0;

		// ホバリング効果を追加
		m_velocity.y += hoverOffset * Scene::DeltaTime() * 30.0;

		// 向きを更新
		m_direction = (direction.x > 0) ? EnemyDirection::Right : EnemyDirection::Left;
	}
	else
	{
		// ターゲットに到達したら新しいパトロールターゲットを設定
		calculatePatrolTarget();
	}
}

void Bee::updatePatrol()
{
	// パトロール角度を更新
	m_patrolAngle += Scene::DeltaTime() * 0.5;
	if (m_patrolAngle >= Math::TwoPi)
	{
		m_patrolAngle -= Math::TwoPi;
	}
}

void Bee::updateChase(const Vec2& playerPosition)
{
	const double distanceToPlayer = m_position.distanceFrom(playerPosition);

	if (distanceToPlayer <= CHASE_DISTANCE)
	{
		m_isChasingPlayer = true;
		m_targetPosition = playerPosition;
	}
	else if (distanceToPlayer > CHASE_DISTANCE * 1.5)
	{
		m_isChasingPlayer = false;
		calculatePatrolTarget();
	}
}

void Bee::calculatePatrolTarget()
{
	if (!m_isChasingPlayer)
	{
		// 円形パトロール
		const double targetX = m_startPosition.x + std::cos(m_patrolAngle) * PATROL_RADIUS;
		const double targetY = m_startPosition.y + std::sin(m_patrolAngle) * PATROL_RADIUS * 0.5;
		m_targetPosition = Vec2(targetX, targetY);
	}
}

void Bee::updateFlyBehavior()
{
	updateMovement();
}

void Bee::updateFlattenedBehavior()
{
	m_flattenedTimer += Scene::DeltaTime();

	if (m_flattenedTimer >= FLATTENED_DURATION)
	{
		m_state = EnemyState::Dead;
		m_isAlive = false;
		m_isActive = false;
	}
}

void Bee::updateHitBehavior()
{
	if (m_stateTimer >= HIT_DURATION)
	{
		setState(EnemyState::Walk);
	}
}

Texture Bee::getCurrentTexture() const
{
	String textureKey;

	switch (m_state)
	{
	case EnemyState::Idle:
		textureKey = U"bee_rest";
		break;
	case EnemyState::Walk:  // 飛行状態
		if (m_isFlying)
		{
			const bool useVariantA = std::fmod(m_animationTimer, FLY_ANIMATION_SPEED * 2) < FLY_ANIMATION_SPEED;
			textureKey = useVariantA ? U"bee_a" : U"bee_b";
		}
		else
		{
			textureKey = U"bee_rest";
		}
		break;
	case EnemyState::Flattened:
		textureKey = U"bee_rest";
		break;
	case EnemyState::Hit:
		textureKey = U"bee_rest";
		break;
	case EnemyState::Dead:
		textureKey = U"bee_rest";
		break;
	default:
		textureKey = U"bee_rest";
		break;
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else if (m_textures.contains(U"bee_rest"))
	{
		return m_textures.at(U"bee_rest");
	}
	return Texture{};
}

void Bee::drawFlyTrail() const
{
	// 飛行軌跡エフェクト
	for (int i = 0; i < 4; ++i)
	{
		const double trailAlpha = 0.2 - i * 0.04;
		const double trailOffset = i * 12.0;
		const Vec2 trailPos = m_position + Vec2(-trailOffset * (m_direction == EnemyDirection::Left ? -1 : 1), i * 2.0);

		Circle(trailPos, 4.0 - i * 0.8).draw(ColorF(1.0, 1.0, 0.6, trailAlpha));
	}
}

void Bee::drawWingEffect() const
{
	// 羽ばたき効果
	const double time = Scene::Time();
	const double wingBeat = std::sin(time * 25.0);
	const double wingAlpha = 0.4 + 0.2 * std::abs(wingBeat);

	// 羽のブラー効果
	for (int i = 0; i < 3; ++i)
	{
		const double wingOffset = wingBeat * (8.0 + i * 2.0);
		const double alpha = wingAlpha / (i + 1);

		// 左の羽
		Line(m_position.x - 15, m_position.y - 8 + wingOffset,
			 m_position.x - 25, m_position.y - 12 + wingOffset).draw(3.0, ColorF(0.9, 0.9, 0.9, alpha));

		// 右の羽
		Line(m_position.x + 15, m_position.y - 8 + wingOffset,
			 m_position.x + 25, m_position.y - 12 + wingOffset).draw(3.0, ColorF(0.9, 0.9, 0.9, alpha));
	}
}

void Bee::drawFlattenedEffect() const
{
	const double effectTime = m_flattenedTimer;

	for (int i = 0; i < 6; ++i)
	{
		const double angle = i * Math::TwoPi / 6.0 + effectTime * 4.0;
		const double distance = 20.0 + std::sin(effectTime * 12.0) * 6.0;
		const Vec2 starPos = m_position + Vec2(std::cos(angle), std::sin(angle)) * distance;
		const double alpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);

		const double starSize = 2.5;
		Line(starPos.x - starSize, starPos.y, starPos.x + starSize, starPos.y).draw(2.0, ColorF(1.0, 1.0, 0.0, alpha));
		Line(starPos.x, starPos.y - starSize, starPos.x, starPos.y + starSize).draw(2.0, ColorF(1.0, 1.0, 0.0, alpha));
		Circle(starPos, 1.0).draw(ColorF(1.0, 1.0, 0.8, alpha));
	}

	const double ringRadius = effectTime * 35.0;
	const double ringAlpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);
	Circle(m_position, ringRadius).drawFrame(2.0, ColorF(1.0, 1.0, 0.0, ringAlpha * 0.5));
}

void Bee::drawHitEffect() const
{
	const double flashRate = 20.0;
	const double alpha = (std::sin(m_stateTimer * flashRate) + 1.0) * 0.2;

	if (alpha > 0.1)
	{
		Circle(m_position, 30).draw(ColorF(1.0, 0.8, 0.0, alpha));
	}
}
