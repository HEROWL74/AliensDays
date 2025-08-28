#include "NormalSlime.hpp"
#include "EnemyFactory.hpp"

NormalSlime::NormalSlime(const Vec2& startPosition)
	: EnemyBase(EnemyType::NormalSlime, startPosition)
	, m_directionTimer(0.0)
	, m_patrolDistance(200.0)
	, m_startPosition(startPosition)
	, m_hasHitWall(false)
	, m_isFlattened(false)
	, m_flattenedTimer(0.0)
{
	// NormalSlime固有の設定
	m_moveSpeed = WALK_SPEED;
	m_collisionWidth = 64.0;
	m_collisionHeight = 64.0;

	init();
}

void NormalSlime::init()
{
	loadTextures();
	setState(EnemyState::Walk);
	startWalking();
	updateCollisionRect();
}

void NormalSlime::update()
{
	// 非アクティブまたは死亡状態なら更新しない
	if (!m_isActive || !m_isAlive || m_state == EnemyState::Dead) return;

	// アニメーションタイマー更新
	updateAnimation();

	// 状態別の更新処理
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
		// 死亡状態では何もしない
		return;
	default:
		break;
	}

	// 物理更新
	updatePhysics();

	// 壁との衝突チェック
	checkWallCollision();
}

void NormalSlime::draw() const
{
	// 非アクティブまたは死亡状態なら描画しない
	if (!m_isActive || m_state == EnemyState::Dead) return;

	const Texture currentTexture = getCurrentTexture();

	if (currentTexture)
	{
		// 向きに応じて左右反転
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
		// フォールバック：テクスチャがない場合は色付きの円
		const ColorF fallbackColor = ColorF(0.2, 0.8, 0.2);  // 緑色
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

	// デバッグ用当たり判定表示
#ifdef _DEBUG
	m_collisionRect.drawFrame(2.0, ColorF(1.0, 0.0, 0.0, 0.6));
#endif
}
void NormalSlime::loadTextures()
{
	m_textures.clear();

	const String basePath = U"Sprites/Enemies/";

	// スライムのテクスチャを読み込み
	Array<String> textureNames = {
		U"slime_normal_rest",
		U"slime_normal_flat",
		U"slime_normal_walk_a",
		U"slime_normal_walk_b"
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

void NormalSlime::setState(EnemyState newState)
{
	// 既に死亡状態の場合は状態変更を無視
	if (m_state == EnemyState::Dead) return;

	EnemyBase::setState(newState);

	// NormalSlime固有の状態変更処理
	switch (newState)
	{
	case EnemyState::Walk:
		m_isFlattened = false;
		startWalking();
		break;
	case EnemyState::Flattened:
		m_isFlattened = true;
		m_flattenedTimer = 0.0;  // タイマーをリセット
		m_velocity.x = 0.0;      // 移動停止
		m_velocity.y = 0.0;      // 完全に停止
		break;
	case EnemyState::Hit:
		m_velocity.x *= 0.5;  // ヒット時は速度を減少
		break;
	case EnemyState::Dead:
		// 死亡状態では何もしない（無限再帰を防ぐ）
		m_isAlive = false;
		m_isActive = false;
		m_velocity.x = 0.0;
		m_velocity.y = 0.0;
		break;
	default:
		break;
	}
}

void NormalSlime::onHit()
{
	EnemyBase::onHit();
	// ヒット時の追加処理（ノックバック等）
}

void NormalSlime::onStomp()
{
	// 既にFlattened状態または死亡状態の場合は何もしない
	if (m_state == EnemyState::Flattened || m_state == EnemyState::Dead)
	{
		return;
	}

	// 踏まれたら一旦Flattened状態にする（即座に消さない）
	setState(EnemyState::Flattened);
}

void NormalSlime::startWalking()
{
	if (m_state == EnemyState::Walk)
	{
		// 歩行開始
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

void NormalSlime::checkWallCollision()
{
	// 簡易的な壁判定（実際のステージ衝突判定はGameSceneで処理）
	// ここでは方向転換のトリガーとしてのみ使用
}

void NormalSlime::updateMovement()
{
	// 移動状態での処理
	if (m_state == EnemyState::Walk)
	{
		// 一定時間で方向転換
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

void NormalSlime::updateWalkBehavior()
{
	updateMovement();
}

void NormalSlime::updateFlattenedBehavior()
{
	m_flattenedTimer += Scene::DeltaTime();

	// 一定時間後に死亡状態に変更（アニメーション表示後）
	if (m_flattenedTimer >= FLATTENED_DURATION)
	{
		// 死亡状態に変更
		m_state = EnemyState::Dead;  // 直接設定（無限再帰を防ぐ）
		m_isAlive = false;
		m_isActive = false;
	}
}

void NormalSlime::updateHitBehavior()
{
	// ヒット状態の持続時間をチェック
	if (m_stateTimer >= HIT_DURATION)
	{
		setState(EnemyState::Walk);
	}
}

void NormalSlime::changeDirection()
{
	// 方向を反転
	m_direction = (m_direction == EnemyDirection::Left) ? EnemyDirection::Right : EnemyDirection::Left;

	// 速度を更新
	if (m_state == EnemyState::Walk)
	{
		const double speed = (m_direction == EnemyDirection::Left) ? -m_moveSpeed : m_moveSpeed;
		m_velocity.x = speed;
	}
}

Texture NormalSlime::getCurrentTexture() const
{
	String textureKey;

	switch (m_state)
	{
	case EnemyState::Idle:
		textureKey = U"slime_normal_rest";
		break;
	case EnemyState::Walk:
	{
		// ウォークアニメーション（walk_a と walk_b を交互に）
		const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
		textureKey = useVariantA ? U"slime_normal_walk_a" : U"slime_normal_walk_b";
	}
	break;
	case EnemyState::Flattened:
		textureKey = U"slime_normal_flat";
		break;
	case EnemyState::Hit:
		textureKey = U"slime_normal_rest";  // ヒット時は静止画像
		break;
	case EnemyState::Dead:
		textureKey = U"slime_normal_flat";  // 死亡時は平たい状態
		break;
	default:
		textureKey = U"slime_normal_rest";
		break;
	}

	if (m_textures.contains(textureKey))
	{
		return m_textures.at(textureKey);
	}
	else
	{
		// フォールバック
		if (m_textures.contains(U"slime_normal_rest"))
		{
			return m_textures.at(U"slime_normal_rest");
		}
		return Texture{};
	}
}

void NormalSlime::drawFlattenedEffect() const
{
	// 踏まれた時のエフェクト
	const double effectTime = m_flattenedTimer;

	// 星型エフェクト（より目立つように）
	for (int i = 0; i < 8; ++i)  // 5個から8個に増加
	{
		const double angle = i * Math::TwoPi / 8.0 + effectTime * 3.0;  // 回転速度を上げる
		const double distance = 25.0 + std::sin(effectTime * 10.0) * 15.0;  // より大きく動く
		const Vec2 starPos = m_position + Vec2(std::cos(angle), std::sin(angle)) * distance;
		const double alpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);

		// 星の描画（より大きく）
		const double starSize = 4.0 + std::sin(effectTime * 8.0) * 2.0;
		Line(starPos.x - starSize, starPos.y, starPos.x + starSize, starPos.y).draw(3.0, ColorF(1.0, 1.0, 0.0, alpha));
		Line(starPos.x, starPos.y - starSize, starPos.x, starPos.y + starSize).draw(3.0, ColorF(1.0, 1.0, 0.0, alpha));

		// 中心の光る点
		Circle(starPos, 2.0).draw(ColorF(1.0, 1.0, 1.0, alpha * 0.8));
	}

	// 中心から広がるリング
	const double ringRadius = effectTime * 60.0;  // 広がるリング
	const double ringAlpha = Math::Max(0.0, 1.0 - effectTime / FLATTENED_DURATION);
	Circle(m_position, ringRadius).drawFrame(3.0, ColorF(1.0, 0.8, 0.0, ringAlpha * 0.5));
}
void NormalSlime::drawHitEffect() const
{
	// ヒット時のエフェクト（点滅）
	const double flashRate = 10.0;
	const double alpha = (std::sin(m_stateTimer * flashRate) + 1.0) * 0.3;

	if (alpha > 0.1)
	{
		Circle(m_position, 40).draw(ColorF(1.0, 0.0, 0.0, alpha));
	}
}

String NormalSlime::currentVisualKey() const
{
	switch (m_state)
	{
	case EnemyState::Idle:
		return U"slime_normal_rest";
		break;
	case EnemyState::Walk:
	{
		const bool useVariantA = std::fmod(m_animationTimer, WALK_ANIMATION_SPEED * 2) < WALK_ANIMATION_SPEED;
		return useVariantA ? U"slime_normal_walk_a" : U"slime_normal_walk_b";
	}
	break;
	case EnemyState::Flattened:
		return U"slime_normal_flat";
		break;
	case EnemyState::Hit:
		return U"slime_normal_rest"; 
		break;
	case EnemyState::Dead:
		return  U"slime_normal_flat"; 
		break;
	default:
		return U"slime_normal_rest";
		break;
	}
}

static EnemyAutoRegister _regNormalSlime{
	U"NormalSlime",
	[](const Vec2& pos) {
		return std::make_unique<NormalSlime>(pos);
	}
};

