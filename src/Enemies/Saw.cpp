#include "Saw.hpp"

Saw::Saw(const Vec2& startPosition)
	: EnemyBase(EnemyType::Saw, startPosition)
	, m_directionTimer(0.0)
	, m_patrolDistance(250.0)
	, m_startPosition(startPosition)
	, m_hasHitWall(false)
	, m_rotationAngle(0.0)
	, m_isSpinning(true)
	, m_sparkTimer(0.0)
{
	// Saw固有の設定
	m_moveSpeed = MOVE_SPEED;
	m_collisionWidth = 64.0;
	m_collisionHeight = 64.0;

	init();
}

void Saw::init()
{
	loadTextures();
	setState(EnemyState::Walk);
	startMoving();
	updateCollisionRect();
}

void Saw::update()
{
	if (!m_isActive || !m_isAlive || m_state == EnemyState::Dead) return;

	updateAnimation();
	updateRotation();
	updateSparks();
	updateMovement();
	updatePhysics();
}

void Saw::draw() const
{
	if (!m_isActive || m_state == EnemyState::Dead) return;

	// 危険エフェクト
	if (isDangerous())
	{
		drawDangerEffect();
	}

	// 回転エフェクト
	drawRotationEffect();

	const Texture currentTexture = getCurrentTexture();

	if (currentTexture)
	{
		// Sawは常に回転している
		currentTexture.rotated(m_rotationAngle).drawAt(m_position);
	}
	else
	{
		// フォールバック：回転する円
		const ColorF fallbackColor = ColorF(0.7, 0.7, 0.7);
		Circle(m_position, 32).draw(fallbackColor);

		// ノコギリの歯を表現
		for (int i = 0; i < 8; ++i)
		{
			const double angle = i * Math::TwoPi / 8.0 + m_rotationAngle;
			const Vec2 toothPos = m_position + Vec2(std::cos(angle), std::sin(angle)) * 30.0;
			Line(m_position, toothPos).draw(3.0, ColorF(0.9, 0.9, 0.9));
		}
	}

	// スパーク効果
	drawSparks();

#ifdef _DEBUG
	m_collisionRect.drawFrame(2.0, ColorF(0.7, 0.7, 0.7, 0.6));
	Circle(m_position, DANGER_RADIUS).drawFrame(1.0, ColorF(1.0, 0.0, 0.0, 0.3));
#endif
}

void Saw::loadTextures()
{
	m_textures.clear();

	const String basePath = U"Sprites/Enemies/";
	Array<String> textureNames = {
		U"saw_rest",
		U"saw_a",
		U"saw_b"
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

void Saw::setState(EnemyState newState)
{
	if (m_state == EnemyState::Dead) return;

	EnemyBase::setState(newState);

	switch (newState)
	{
	case EnemyState::Walk:
		startMoving();
		break;
	case EnemyState::Hit:
		// Sawは基本的にヒットしない（常に危険）
		break;
	case EnemyState::Dead:
		m_isAlive = false;
		m_isActive = false;
		m_isSpinning = false;
		m_velocity.x = 0.0;
		m_velocity.y = 0.0;
		break;
	default:
		break;
	}
}

void Saw::onHit()
{
	// Sawは通常のヒットでは破壊されない
	// 特殊な攻撃でのみ破壊可能
}

void Saw::onStomp()
{
	// Sawは踏みつけでは破壊されない
	// むしろプレイヤーがダメージを受ける
}

void Saw::startMoving()
{
	if (m_state == EnemyState::Walk)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void Saw::updateMovement()
{
	if (m_state == EnemyState::Walk)
	{
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
	}
}

void Saw::changeDirection()
{
	m_direction = (m_direction == EnemyDirection::Left) ? EnemyDirection::Right : EnemyDirection::Left;

	if (m_state == EnemyState::Walk)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void Saw::updateRotation()
{
	if (m_isSpinning)
	{
		m_rotationAngle += ROTATE_SPEED * Scene::DeltaTime();
		if (m_rotationAngle >= Math::TwoPi)
		{
			m_rotationAngle -= Math::TwoPi;
		}
	}
}

void Saw::updateSparks()
{
	m_sparkTimer += Scene::DeltaTime();
}

Texture Saw::getCurrentTexture() const
{
	String textureKey;

	if (m_isSpinning)
	{
		const bool useVariantA = std::fmod(m_animationTimer, SAW_ANIMATION_SPEED * 2) < SAW_ANIMATION_SPEED;
		textureKey = useVariantA ? U"saw_a" : U"saw_b";
	}
	else
	{
		textureKey = U"saw_rest";
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else if (m_textures.contains(U"saw_rest"))
	{
		return m_textures.at(U"saw_rest");
	}
	return Texture{};
}

void Saw::drawDangerEffect() const
{
	const double time = Scene::Time();
	const double pulseAlpha = 0.2 + 0.15 * std::sin(time * 10.0);

	// 危険範囲の表示
	Circle(m_position, DANGER_RADIUS).draw(ColorF(1.0, 0.0, 0.0, pulseAlpha * 0.3));
	Circle(m_position, DANGER_RADIUS - 10).drawFrame(2.0, ColorF(1.0, 0.2, 0.0, pulseAlpha));
}

void Saw::drawSparks() const
{
	if (!m_isSpinning) return;

	const double time = Scene::Time();

	// スパーク効果
	for (int i = 0; i < 6; ++i)
	{
		const double sparkTime = time + i * 0.3;
		const double sparkAlpha = (std::sin(sparkTime * 8.0) + 1.0) * 0.25;

		if (sparkAlpha > 0.1)
		{
			const double angle = i * Math::TwoPi / 6.0 + m_rotationAngle * 2.0;
			const double distance = 35.0 + std::sin(sparkTime * 12.0) * 8.0;
			const Vec2 sparkPos = m_position + Vec2(std::cos(angle), std::sin(angle)) * distance;

			// 小さな火花
			Circle(sparkPos, 2.0).draw(ColorF(1.0, 0.8, 0.2, sparkAlpha));
			Circle(sparkPos, 1.0).draw(ColorF(1.0, 1.0, 0.8, sparkAlpha * 1.5));
		}
	}
}

void Saw::drawRotationEffect() const
{
	if (!m_isSpinning) return;

	// 回転ブラー効果
	const double blurAlpha = 0.1;
	for (int i = 0; i < 4; ++i)
	{
		const double blurAngle = m_rotationAngle + i * Math::Pi / 2.0;
		const double blurRadius = 28.0 + i * 2.0;

		Circle(m_position, blurRadius).draw(ColorF(0.8, 0.8, 0.8, blurAlpha));
	}
}
