#include "SpikeSlime.hpp"
#include "EnemyFactory.hpp"

SpikeSlime::SpikeSlime(const Vec2& startPosition)
	: EnemyBase(EnemyType::SpikeSlime, startPosition)
	, m_directionTimer(0.0)
	, m_patrolDistance(150.0)
	, m_startPosition(startPosition)
	, m_hasHitWall(false)
	, m_isFlattened(false)
	, m_flattenedTimer(0.0)
{
	// SpikeSlime固有の設定
	m_moveSpeed = WALK_SPEED;
	m_collisionWidth = 64.0;
	m_collisionHeight = 64.0;

	init();
}

void SpikeSlime::init()
{
	loadTextures();
	setState(EnemyState::Walk);
	startWalking();
	updateCollisionRect();
}

void SpikeSlime::update()
{
	if (!m_isActive || !m_isAlive || m_state == EnemyState::Dead) return;

	updateAnimation();

	switch (m_state)
	{
	case EnemyState::Walk:
		updateWalkBehavior();
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
	checkWallCollision();
}

void SpikeSlime::draw() const
{
	if (!m_isActive || m_state == EnemyState::Dead) return;

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
		const ColorF fallbackColor = ColorF(0.6, 0.2, 0.6);  // 紫色
		Circle(m_position, 32).draw(fallbackColor);
	}

	// スパイク警告エフェクト（常に表示）
	if (m_isActive && m_isAlive)
	{
		drawSpikeWarning();
	}

	// エフェクト描画
	if (m_state == EnemyState::Hit)
	{
		drawHitEffect();
	}

#ifdef _DEBUG
	m_collisionRect.drawFrame(2.0, ColorF(1.0, 0.0, 1.0, 0.6));
#endif
}

void SpikeSlime::loadTextures()
{
	m_textures.clear();

	const String basePath = U"Sprites/Enemies/";
	Array<String> textureNames = {
		U"slime_spike_rest",
		U"slime_spike_flat",
		U"slime_spike_walk_a",
		U"slime_spike_walk_b"
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

void SpikeSlime::setState(EnemyState newState)
{
	if (m_state == EnemyState::Dead) return;

	EnemyBase::setState(newState);

	switch (newState)
	{
	case EnemyState::Walk:
		m_isFlattened = false;
		startWalking();
		break;
	case EnemyState::Hit:
		m_velocity.x *= 0.5;
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

void SpikeSlime::onHit()
{
	EnemyBase::onHit();
}

void SpikeSlime::onStomp()
{
	// SpikeSlimeは踏むことができない（プレイヤーがダメージを受ける）
	// この関数は呼ばれるべきではないが、安全のため何もしない
}

void SpikeSlime::startWalking()
{
	if (m_state == EnemyState::Walk)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void SpikeSlime::checkWallCollision()
{
	// GameSceneで処理される
}

void SpikeSlime::updateMovement()
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

void SpikeSlime::changeDirection()
{
	m_direction = (m_direction == EnemyDirection::Left) ? EnemyDirection::Right : EnemyDirection::Left;

	if (m_state == EnemyState::Walk)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void SpikeSlime::updateWalkBehavior()
{
	updateMovement();
}

void SpikeSlime::updateHitBehavior()
{
	if (m_stateTimer >= HIT_DURATION)
	{
		setState(EnemyState::Walk);
	}
}

Texture SpikeSlime::getCurrentTexture() const
{
	String textureKey;

	switch (m_state)
	{
	case EnemyState::Idle:
		textureKey = U"slime_spike_rest";
		break;
	case EnemyState::Walk:
	{
		const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
		textureKey = useVariantA ? U"slime_spike_walk_a" : U"slime_spike_walk_b";
	}
	break;
	case EnemyState::Hit:
		textureKey = U"slime_spike_rest";
		break;
	case EnemyState::Dead:
		textureKey = U"slime_spike_flat";
		break;
	default:
		textureKey = U"slime_spike_rest";
		break;
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else if (m_textures.contains(U"slime_spike_rest"))
	{
		return m_textures.at(U"slime_spike_rest");
	}
	return Texture{};
}

void SpikeSlime::drawSpikeWarning() const
{
	// スパイクの危険を示すエフェクト（常に表示）
	const double time = Scene::Time();
	const double pulseAlpha = 0.4 + 0.2 * std::sin(time * 8.0);

	// 赤い危険オーラ
	Circle(m_position, 50).draw(ColorF(1.0, 0.0, 0.0, pulseAlpha * 0.3));
	Circle(m_position, 45).drawFrame(3.0, ColorF(1.0, 0.2, 0.0, pulseAlpha));

	// スパイクを示す突起エフェクト
	for (int i = 0; i < 8; ++i)
	{
		const double angle = i * Math::TwoPi / 8.0 + time * 2.0;
		const double distance = 35.0 + std::sin(time * 6.0 + i * 0.5) * 8.0;
		const Vec2 spikePos = m_position + Vec2(std::cos(angle), std::sin(angle)) * distance;

		const double spikeSize = 4.0 + std::sin(time * 10.0 + i) * 1.0;

		// トゲの描画（×印）
		Line(spikePos.x - spikeSize, spikePos.y - spikeSize,
			 spikePos.x + spikeSize, spikePos.y + spikeSize).draw(2.0, ColorF(1.0, 0.3, 0.3, pulseAlpha));
		Line(spikePos.x + spikeSize, spikePos.y - spikeSize,
			 spikePos.x - spikeSize, spikePos.y + spikeSize).draw(2.0, ColorF(1.0, 0.3, 0.3, pulseAlpha));

		// トゲの中心点
		Circle(spikePos, 1.5).draw(ColorF(1.0, 0.0, 0.0, pulseAlpha));
	}
}

void SpikeSlime::drawHitEffect() const
{
	const double flashRate = 12.0;
	const double alpha = (std::sin(m_stateTimer * flashRate) + 1.0) * 0.25;

	if (alpha > 0.1)
	{
		Circle(m_position, 40).draw(ColorF(1.0, 0.2, 0.2, alpha));
	}
}

String SpikeSlime::currentVisualKey() const
{
	switch (m_state)
	{
	case EnemyState::Idle:
		return U"slime_spike_rest";
		break;
	case EnemyState::Walk:
	{
		const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
		return useVariantA ? U"slime_spike_walk_a" : U"slime_spike_walk_b";
	}
	break;
	case EnemyState::Hit:
		return U"slime_spike_rest";
		break;
	case EnemyState::Dead:
		return U"slime_spike_flat";
		break;
	default:
		return U"slime_spike_rest";
		break;
	}
}

static EnemyAutoRegister _regSpikeSlime{
	U"SpikeSlime",
	[](const Vec2& pos) {
		return std::make_unique<SpikeSlime>(pos);
	}
};
