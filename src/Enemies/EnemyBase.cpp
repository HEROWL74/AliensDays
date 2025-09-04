#include "EnemyBase.hpp"

EnemyBase::EnemyBase(EnemyType type, const Vec2& startPosition)
	: m_type(type)
	, m_position(startPosition)
	, m_velocity(Vec2::Zero())
	, m_state(EnemyState::Idle)
	, m_direction(EnemyDirection::Left)
	, m_isActive(true)
	, m_isAlive(true)
	, m_animationTimer(0.0)
	, m_stateTimer(0.0)
	, m_moveSpeed(50.0)
	, m_gravity(600.0)
	, m_isGrounded(false)
	, m_collisionWidth(64.0)
	, m_collisionHeight(64.0)
	, m_effectTimer(0.0)
	, m_hasEffect(false)
	, m_isTransformed(false)
	, m_blackFireFrame(0)
	, m_blackFireAnimTimer(0.0)
{
	updateCollisionRect();
}

void EnemyBase::setState(EnemyState newState)
{
	if (m_state != newState)
	{
		m_state = newState;
		m_stateTimer = 0.0;
		m_animationTimer = 0.0;
	}
}

void EnemyBase::onHit()
{
	// 基本的なヒット処理
	setState(EnemyState::Hit);
}

void EnemyBase::onStomp()
{
	// 既にFlattened状態または死亡状態の場合は何もしない
	if (m_state == EnemyState::Flattened || m_state == EnemyState::Dead)
	{
		return;
	}

	// 基本的な踏まれた処理
	setState(EnemyState::Flattened);
	m_velocity.x = 0.0;
	m_velocity.y = 0.0;
}

void EnemyBase::onDestroy()
{
	// setStateを呼ばずに直接設定
	m_state = EnemyState::Dead;
	m_isAlive = false;
	m_isActive = false;
}

RectF EnemyBase::getCollisionRect() const
{
	return m_collisionRect;
}

bool EnemyBase::checkCollisionWith(const RectF& rect) const
{
	return m_collisionRect.intersects(rect);
}

void EnemyBase::updatePhysics()
{
	const double deltaTime = Scene::DeltaTime();

	// 重力適用
	if (!m_isGrounded)
	{
		applyGravity();
	}

	// 位置更新
	m_position += m_velocity * deltaTime;

	// 当たり判定矩形更新
	updateCollisionRect();
}

void EnemyBase::applyGravity()
{
	m_velocity.y += m_gravity * Scene::DeltaTime();
}

void EnemyBase::updateAnimation()
{
	m_animationTimer += Scene::DeltaTime();
	m_stateTimer += Scene::DeltaTime();

	// エフェクトタイマー更新
	if (m_hasEffect)
	{
		m_effectTimer += Scene::DeltaTime();
	}
}

void EnemyBase::updateCollisionRect()
{
	const double halfWidth = m_collisionWidth / 2.0;
	const double halfHeight = m_collisionHeight / 2.0;
	m_collisionRect = RectF(
		m_position.x - halfWidth,
		m_position.y - halfHeight,
		m_collisionWidth,
		m_collisionHeight
	);
}

String EnemyBase::getStateString() const
{
	switch (m_state)
	{
	case EnemyState::Idle:      return U"Idle";
	case EnemyState::Walk:      return U"Walk";
	case EnemyState::Hit:       return U"Hit";
	case EnemyState::Dead:      return U"Dead";
	case EnemyState::Flattened: return U"Flattened";
	case EnemyState::Attack:    return U"Attack";
	default: return U"Unknown";
	}
}


String EnemyBase::buildTextureKey(const String& action) const
{
	return action;
}

String EnemyBase::buildTextureKey(const String& action, const String& variant) const
{
	return U"{}_{}"_fmt(action, variant);
}
