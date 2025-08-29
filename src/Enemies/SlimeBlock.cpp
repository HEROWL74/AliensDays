#include "SlimeBlock.hpp"
#include "EnemyFactory.hpp"

SlimeBlock::SlimeBlock(const Vec2& startPosition)
	: EnemyBase(EnemyType::SlimeBlock, startPosition)
	, m_jumpTimer(0.0)
	, m_directionTimer(0.0)
	, m_patrolDistance(180.0)
	, m_startPosition(startPosition)
	, m_hasHitWall(false)
	, m_canJump(true)
	, m_isFlattened(false)
	, m_flattenedTimer(0.0)
	, m_isJumping(false)
{
	// SlimeBlock固有の設定
	m_moveSpeed = WALK_SPEED;
	m_collisionWidth = 64.0;
	m_collisionHeight = 64.0;

	init();
}

void SlimeBlock::init()
{
	loadTextures();
	setState(EnemyState::Walk);
	startWalking();
	updateCollisionRect();
}

void SlimeBlock::update()
{
	if (!m_isActive || !m_isAlive || m_state == EnemyState::Dead) return;

	updateAnimation();
	updateJumpState();

	switch (m_state)
	{
	case EnemyState::Walk:
		updateWalkBehavior();
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

void SlimeBlock::draw() const
{
	if (!m_isActive || m_state == EnemyState::Dead) return;

	// ジャンプ準備エフェクト
	if (m_canJump && !m_isJumping && m_jumpTimer > JUMP_INTERVAL - 0.5)
	{
		drawJumpPreparation();
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
		const ColorF fallbackColor = ColorF(0.2, 0.5, 0.8);  // 青色
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
	m_collisionRect.drawFrame(2.0, ColorF(0.0, 0.5, 1.0, 0.6));
#endif
}

void SlimeBlock::loadTextures()
{
	m_textures.clear();

	const String basePath = U"Sprites/Enemies/";
	Array<String> textureNames = {
		U"slime_block_rest",
		U"slime_block_jump",
		U"slime_block_walk_a",
		U"slime_block_walk_b"
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

void SlimeBlock::setState(EnemyState newState)
{
	if (m_state == EnemyState::Dead) return;

	EnemyBase::setState(newState);

	switch (newState)
	{
	case EnemyState::Walk:
		m_isFlattened = false;
		startWalking();
		break;
	case EnemyState::Flattened:
		m_isFlattened = true;
		m_isJumping = false;
		m_flattenedTimer = 0.0;
		m_velocity.x = 0.0;
		m_velocity.y = 0.0;
		break;
	case EnemyState::Hit:
		m_velocity.x *= 0.2;
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

void SlimeBlock::onHit()
{
	EnemyBase::onHit();
}


void SlimeBlock::onStomp()
{
	// 既にFlattened状態または死亡状態の場合は何もしない
	if (m_state == EnemyState::Flattened || m_state == EnemyState::Dead)
	{
		return;
	}

	setState(EnemyState::Flattened);
}
void SlimeBlock::startWalking()
{
	if (m_state == EnemyState::Walk)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void SlimeBlock::performJump()
{
	if (m_canJump && m_isGrounded && m_state == EnemyState::Walk)
	{
		m_velocity.y = -JUMP_POWER;
		m_isJumping = true;
		m_jumpTimer = 0.0;
		m_canJump = false;
	}
}

void SlimeBlock::updateMovement()
{
	if (m_state == EnemyState::Walk)
	{
		// ジャンプタイマー更新
		m_jumpTimer += Scene::DeltaTime();
		if (m_jumpTimer >= JUMP_INTERVAL && !m_isJumping)
		{
			m_canJump = true;
			performJump();
		}

		// 方向転換タイマー
		m_directionTimer += Scene::DeltaTime();
		if (m_directionTimer >= DIRECTION_CHANGE_TIME)
		{
			changeDirection();
			m_directionTimer = 0.0;
		}

		// パトロール範囲チェック
		const double distanceFromStart = std::abs(m_position.x - m_startPosition.x);
		if (distanceFromStart > m_patrolDistance)
		{
			changeDirection();
		}
	}
}

void SlimeBlock::changeDirection()
{
	m_direction = (m_direction == EnemyDirection::Left) ? EnemyDirection::Right : EnemyDirection::Left;

	if (m_state == EnemyState::Walk)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void SlimeBlock::updateWalkBehavior()
{
	updateMovement();
}

void SlimeBlock::updateFlattenedBehavior()
{
	m_flattenedTimer += Scene::DeltaTime();

	if (m_flattenedTimer >= FLATTENED_DURATION)
	{
		m_state = EnemyState::Dead;
		m_isAlive = false;
		m_isActive = false;
	}
}

void SlimeBlock::updateHitBehavior()
{
	if (m_stateTimer >= HIT_DURATION)
	{
		setState(EnemyState::Walk);
	}
}

void SlimeBlock::updateJumpState()
{
	// 地面に着地したらジャンプ状態を解除
	if (m_isJumping && m_isGrounded)
	{
		m_isJumping = false;
		m_canJump = false;
		m_jumpTimer = 0.0;
	}
}

Texture SlimeBlock::getCurrentTexture() const
{
	String textureKey;

	switch (m_state)
	{
	case EnemyState::Idle:
		textureKey = U"slime_block_rest";
		break;
	case EnemyState::Walk:
		if (m_isJumping)
		{
			textureKey = U"slime_block_jump";
		}
		else
		{
			const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
			textureKey = useVariantA ? U"slime_block_walk_a" : U"slime_block_walk_b";
		}
		break;
	case EnemyState::Flattened:
		textureKey = U"slime_block_rest";
		break;
	case EnemyState::Hit:
		textureKey = U"slime_block_rest";
		break;
	case EnemyState::Dead:
		textureKey = U"slime_block_rest";
		break;
	default:
		textureKey = U"slime_block_rest";
		break;
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else if (m_textures.contains(U"slime_block_rest"))
	{
		return m_textures.at(U"slime_block_rest");
	}
	return Texture{};
}

void SlimeBlock::drawJumpPreparation() const
{
	// ジャンプ準備の視覚的フィードバック
	const double time = Scene::Time();
	const double pulseAlpha = 0.4 + 0.3 * std::sin(time * 12.0);

	// 下向きの圧縮エフェクト
	Circle(m_position + Vec2(0, 20), 30).draw(ColorF(0.2, 0.5, 1.0, pulseAlpha));

	// 上向きの矢印エフェクト
	for (int i = 0; i < 3; ++i)
	{
		const double arrowY = m_position.y - 30 - i * 15;
		const double arrowAlpha = pulseAlpha * (1.0 - i * 0.3);

		// 簡単な上向き矢印
		Line(m_position.x, arrowY, m_position.x - 8, arrowY + 8).draw(2.0, ColorF(0.2, 1.0, 0.2, arrowAlpha));
		Line(m_position.x, arrowY, m_position.x + 8, arrowY + 8).draw(2.0, ColorF(0.2, 1.0, 0.2, arrowAlpha));
	}
}

void SlimeBlock::drawFlattenedEffect() const
{
	const double effectTime = m_flattenedTimer;

	for (int i = 0; i < 8; ++i)
	{
		const double angle = i * Math::TwoPi / 8.0 + effectTime * 2.0;
		const double distance = 35.0 + std::sin(effectTime * 6.0) * 12.0;
		const Vec2 starPos = m_position + Vec2(std::cos(angle), std::sin(angle)) * distance;
		const double alpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);

		const double starSize = 4.0;
		Line(starPos.x - starSize, starPos.y, starPos.x + starSize, starPos.y).draw(2.0, ColorF(0.2, 0.5, 1.0, alpha));
		Line(starPos.x, starPos.y - starSize, starPos.x, starPos.y + starSize).draw(2.0, ColorF(0.2, 0.5, 1.0, alpha));
		Circle(starPos, 2.0).draw(ColorF(0.5, 0.8, 1.0, alpha));
	}

	const double ringRadius = effectTime * 60.0;
	const double ringAlpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);
	Circle(m_position, ringRadius).drawFrame(3.0, ColorF(0.2, 0.5, 1.0, ringAlpha * 0.5));
}

void SlimeBlock::drawHitEffect() const
{
	const double flashRate = 10.0;
	const double alpha = (std::sin(m_stateTimer * flashRate) + 1.0) * 0.3;

	if (alpha > 0.1)
	{
		Circle(m_position, 45).draw(ColorF(0.5, 0.5, 1.0, alpha));
	}
}

String SlimeBlock::currentVisualKey() const
{
	switch (m_state)
	{
	case EnemyState::Idle:
		return U"slime_block_rest";
	case EnemyState::Walk:
		if (m_isJumping)
		{
			return U"slime_block_jump";
		}
		else
		{
			const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
			return useVariantA ? U"slime_block_walk_a" : U"slime_block_walk_b";
		}
		return U"slime_block_rest";
	case EnemyState::Flattened:
		return U"slime_block_rest";
	case EnemyState::Hit:
		return U"slime_block_rest";
	case EnemyState::Dead:
		return U"slime_block_rest";
	default:
		return U"slime_block_rest";
	}
}

static EnemyAutoRegister _regSlimeBlock{
	U"SlimeBlock",
	[](const Vec2& pos) {
		return std::make_unique<SlimeBlock>(pos);
	}
};
