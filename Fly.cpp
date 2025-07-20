#include "Fly.hpp"

Fly::Fly(const Vec2& startPosition)
	: EnemyBase(EnemyType::Fly, startPosition)  // Use correct Fly type
	, m_erraticTimer(0.0)
	, m_directionTimer(0.0)
	, m_patrolDistance(150.0)
	, m_startPosition(startPosition)
	, m_erraticOffset(Vec2::Zero())
	, m_hasHitWall(false)
	, m_isFlattened(false)
	, m_flattenedTimer(0.0)
	, m_isFlying(true)
{
	// Fly固有の設定
	m_moveSpeed = FLY_SPEED;
	m_collisionWidth = 48.0;  // Flyはより小さい
	m_collisionHeight = 48.0;
	m_gravity = 0.0;  // Flyは重力の影響を受けない

	init();
}

void Fly::init()
{
	loadTextures();
	setState(EnemyState::Walk);
	startFlying();
	updateCollisionRect();
}

void Fly::update()
{
	if (!m_isActive || !m_isAlive || m_state == EnemyState::Dead) return;

	updateAnimation();
	updateErraticMovement();

	switch (m_state)
	{
	case EnemyState::Walk:
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

	// Flyは重力の影響を受けないので、独自の物理更新
	if (m_isFlying)
	{
		const double deltaTime = Scene::DeltaTime();
		m_position += m_velocity * deltaTime;
		updateCollisionRect();
	}
	else
	{
		updatePhysics();
	}
}

void Fly::draw() const
{
	if (!m_isActive || m_state == EnemyState::Dead) return;

	// 不規則な軌跡効果
	if (m_isFlying)
	{
		drawErraticPath();
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
		const ColorF fallbackColor = ColorF(0.3, 0.3, 0.3);  // グレー
		Circle(m_position, 24).draw(fallbackColor);

		// 簡単な羽の表現
		if (m_isFlying)
		{
			const double wingBeat = std::sin(Scene::Time() * 30.0) * 5.0;
			Line(m_position.x - 12, m_position.y - 8 + wingBeat,
				 m_position.x - 8, m_position.y - 10 + wingBeat).draw(1.0, ColorF(0.7, 0.7, 0.7, 0.8));
			Line(m_position.x + 8, m_position.y - 10 + wingBeat,
				 m_position.x + 12, m_position.y - 8 + wingBeat).draw(1.0, ColorF(0.7, 0.7, 0.7, 0.8));
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
	m_collisionRect.drawFrame(2.0, ColorF(0.5, 0.5, 0.5, 0.6));
#endif
}

void Fly::loadTextures()
{
	m_textures.clear();

	const String basePath = U"Sprites/Enemies/";
	Array<String> textureNames = {
		U"fly_rest",
		U"fly_a",
		U"fly_b"
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

void Fly::setState(EnemyState newState)
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
		m_gravity = 600.0;
		break;
	case EnemyState::Hit:
		m_velocity *= 0.7;
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

void Fly::onHit()
{
	EnemyBase::onHit();
}

void Fly::onStomp()
{
	if (m_state != EnemyState::Dead)
	{
		setState(EnemyState::Flattened);
	}
}

void Fly::startFlying()
{
	if (m_state == EnemyState::Walk && m_isFlying)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void Fly::updateMovement()
{
	if (!m_isFlying) return;

	m_directionTimer += Scene::DeltaTime();
	if (m_directionTimer >= DIRECTION_CHANGE_TIME)
	{
		changeDirection();
		m_directionTimer = 0.0;
	}

	const double distanceFromStart = std::abs(m_position.x - m_startPosition.x);
	if (distanceFromStart > m_patrolDistance)
	{
		changeDirection();
	}

	// 不規則な動きを基本移動に追加
	m_velocity += m_erraticOffset * Scene::DeltaTime() * 100.0;
}

void Fly::changeDirection()
{
	m_direction = (m_direction == EnemyDirection::Left) ? EnemyDirection::Right : EnemyDirection::Left;

	if (m_state == EnemyState::Walk && m_isFlying)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void Fly::updateErraticMovement()
{
	if (!m_isFlying) return;

	m_erraticTimer += Scene::DeltaTime();

	// 不規則な動きを計算
	const double erraticX = std::sin(m_erraticTimer * ERRATIC_SPEED) * ERRATIC_AMPLITUDE;
	const double erraticY = std::cos(m_erraticTimer * ERRATIC_SPEED * 1.3) * ERRATIC_AMPLITUDE * 0.7;

	m_erraticOffset = Vec2(erraticX, erraticY);
}

void Fly::updateFlyBehavior()
{
	updateMovement();
}

void Fly::updateFlattenedBehavior()
{
	m_flattenedTimer += Scene::DeltaTime();

	if (m_flattenedTimer >= FLATTENED_DURATION)
	{
		m_state = EnemyState::Dead;
		m_isAlive = false;
		m_isActive = false;
	}
}

void Fly::updateHitBehavior()
{
	if (m_stateTimer >= HIT_DURATION)
	{
		setState(EnemyState::Walk);
	}
}

Texture Fly::getCurrentTexture() const
{
	String textureKey;

	switch (m_state)
	{
	case EnemyState::Idle:
		textureKey = U"fly_rest";
		break;
	case EnemyState::Walk:
		if (m_isFlying)
		{
			const bool useVariantA = std::fmod(m_animationTimer, FLY_ANIMATION_SPEED * 2) < FLY_ANIMATION_SPEED;
			textureKey = useVariantA ? U"fly_a" : U"fly_b";
		}
		else
		{
			textureKey = U"fly_rest";
		}
		break;
	case EnemyState::Flattened:
		textureKey = U"fly_rest";
		break;
	case EnemyState::Hit:
		textureKey = U"fly_rest";
		break;
	case EnemyState::Dead:
		textureKey = U"fly_rest";
		break;
	default:
		textureKey = U"fly_rest";
		break;
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else if (m_textures.contains(U"fly_rest"))
	{
		return m_textures.at(U"fly_rest");
	}
	return Texture{};
}

void Fly::drawFlyTrail() const
{
	// 小さな軌跡エフェクト
	for (int i = 0; i < 3; ++i)
	{
		const double trailAlpha = 0.15 - i * 0.04;
		const double trailOffset = i * 8.0;
		const Vec2 trailPos = m_position + Vec2(-trailOffset * (m_direction == EnemyDirection::Left ? -1 : 1), i * 1.5);

		Circle(trailPos, 2.0 - i * 0.5).draw(ColorF(0.5, 0.5, 0.5, trailAlpha));
	}
}

void Fly::drawErraticPath() const
{
	// 不規則な動きの軌跡を表示
	const double time = Scene::Time();

	for (int i = 0; i < 5; ++i)
	{
		const double pathTime = time - i * 0.1;
		const double pathX = std::sin(pathTime * ERRATIC_SPEED) * ERRATIC_AMPLITUDE * 0.3;
		const double pathY = std::cos(pathTime * ERRATIC_SPEED * 1.3) * ERRATIC_AMPLITUDE * 0.2;
		const Vec2 pathPos = m_position + Vec2(pathX, pathY);
		const double pathAlpha = 0.1 - i * 0.015;

		if (pathAlpha > 0.0)
		{
			Circle(pathPos, 1.0).draw(ColorF(0.3, 0.3, 0.3, pathAlpha));
		}
	}
}

void Fly::drawFlattenedEffect() const
{
	const double effectTime = m_flattenedTimer;

	for (int i = 0; i < 4; ++i)
	{
		const double angle = i * Math::TwoPi / 4.0 + effectTime * 5.0;
		const double distance = 15.0 + std::sin(effectTime * 15.0) * 4.0;
		const Vec2 starPos = m_position + Vec2(std::cos(angle), std::sin(angle)) * distance;
		const double alpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);

		const double starSize = 1.5;
		Line(starPos.x - starSize, starPos.y, starPos.x + starSize, starPos.y).draw(1.0, ColorF(0.5, 0.5, 0.5, alpha));
		Line(starPos.x, starPos.y - starSize, starPos.x, starPos.y + starSize).draw(1.0, ColorF(0.5, 0.5, 0.5, alpha));
		Circle(starPos, 0.8).draw(ColorF(0.7, 0.7, 0.7, alpha));
	}

	const double ringRadius = effectTime * 25.0;
	const double ringAlpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);
	Circle(m_position, ringRadius).drawFrame(1.0, ColorF(0.5, 0.5, 0.5, ringAlpha * 0.5));
}

void Fly::drawHitEffect() const
{
	const double flashRate = 25.0;
	const double alpha = (std::sin(m_stateTimer * flashRate) + 1.0) * 0.15;

	if (alpha > 0.05)
	{
		Circle(m_position, 20).draw(ColorF(0.7, 0.7, 0.7, alpha));
	}
}
