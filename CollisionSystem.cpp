#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "Stage.hpp"
#include "BlockSystem.hpp"

void CollisionSystem::resolvePlayerCollisions(Player* player, Stage* stage, BlockSystem* blockSystem)
{
	if (!player) return;

	// プレイヤーの物理情報を取得
	Vec2 playerPos = player->getPosition();
	Vec2 playerVel = player->getVelocity();
	Vec2 playerSize = Vec2(50.0, 80.0);  // さらにサイズを小さくして精度向上
	bool wasGrounded = player->isGrounded();

	MovingBody playerBody(playerPos, playerVel, playerSize, wasGrounded);

	// 全ての地形の衝突矩形を収集
	Array<RectF> allTerrainRects;

	// ステージの地形を追加
	if (stage)
	{
		Array<RectF> stageRects = stage->getCollisionRects();
		allTerrainRects.append(stageRects);
	}

	// ブロックシステムの矩形を追加
	if (blockSystem)
	{
		Array<RectF> blockRects = blockSystem->getCollisionRects();
		allTerrainRects.append(blockRects);
	}

	// 衝突判定を実行
	Array<CollisionInfo> collisions = checkCollisionsWithTerrain(playerBody, allTerrainRects);

	// 衝突を解決
	if (!collisions.empty())
	{
		resolveMultipleCollisions(playerBody, collisions);
	}

	// 地面判定
	bool isCurrentlyGrounded = isGrounded(playerBody, allTerrainRects);

	// プレイヤーの状態を更新
	player->setPosition(playerBody.position);
	player->setVelocity(playerBody.velocity);
	player->setGrounded(isCurrentlyGrounded);
}

Array<CollisionInfo> CollisionSystem::checkCollisionsWithTerrain(const MovingBody& body, const Array<RectF>& terrainRects) const
{
	Array<CollisionInfo> collisions;
	const double deltaTime = Scene::DeltaTime();

	// 現在位置での矩形
	RectF currentRect = body.getBounds();

	// 次フレーム位置での矩形
	Vec2 nextPosition = body.position + body.velocity * deltaTime;
	RectF nextRect(nextPosition.x - body.size.x / 2, nextPosition.y - body.size.y / 2, body.size.x, body.size.y);

	for (const auto& terrainRect : terrainRects)
	{
		// 次フレームで衝突する場合（現在は衝突していない場合も含む）
		if (nextRect.intersects(terrainRect))
		{
			CollisionInfo collision = getCollisionInfo(nextRect, terrainRect);
			if (collision.hasCollision)
			{
				// 現在衝突していない場合のみ移動方向を考慮
				if (!currentRect.intersects(terrainRect))
				{
					collision = refineCollisionWithVelocity(collision, body.velocity, currentRect, terrainRect);
				}
				collisions.push_back(collision);
			}
		}
	}

	return collisions;
}

CollisionInfo CollisionSystem::getCollisionInfo(const RectF& moving, const RectF& stationary) const
{
	if (!moving.intersects(stationary))
	{
		return CollisionInfo();
	}

	// 重複量を計算
	double overlapLeft = (moving.x + moving.w) - stationary.x;
	double overlapRight = (stationary.x + stationary.w) - moving.x;
	double overlapTop = (moving.y + moving.h) - stationary.y;
	double overlapBottom = (stationary.y + stationary.h) - moving.y;

	// 最小の重複を見つけて衝突タイプを決定
	double minOverlap = Math::Min(Math::Min(overlapLeft, overlapRight), Math::Min(overlapTop, overlapBottom));

	CollisionType type = CollisionType::None;
	Vec2 normal(0, 0);
	double penetration = 0.0;

	// より厳密な衝突判定：重複の最小値で判定し、さらに移動方向も考慮
	if (Math::Abs(minOverlap - overlapTop) < COLLISION_EPSILON && overlapTop > 0)
	{
		// 上からの衝突（地面）
		type = CollisionType::Ground;
		normal = Vec2(0, -1);
		penetration = overlapTop;
	}
	else if (Math::Abs(minOverlap - overlapBottom) < COLLISION_EPSILON && overlapBottom > 0)
	{
		// 下からの衝突（天井）
		type = CollisionType::Ceiling;
		normal = Vec2(0, 1);
		penetration = overlapBottom;
	}
	else if (Math::Abs(minOverlap - overlapLeft) < COLLISION_EPSILON && overlapLeft > 0)
	{
		// 左からの衝突（右の壁）
		type = CollisionType::RightWall;
		normal = Vec2(-1, 0);
		penetration = overlapLeft;
	}
	else if (Math::Abs(minOverlap - overlapRight) < COLLISION_EPSILON && overlapRight > 0)
	{
		// 右からの衝突（左の壁）
		type = CollisionType::LeftWall;
		normal = Vec2(1, 0);
		penetration = overlapRight;
	}

	Vec2 contactPoint = calculateContactPoint(moving, stationary);

	return CollisionInfo(type, contactPoint, normal, penetration);
}

void CollisionSystem::resolveMultipleCollisions(MovingBody& body, const Array<CollisionInfo>& collisions) const
{
	// 衝突の優先順位: 地面 > 天井 > 壁
	Array<CollisionInfo> groundCollisions;
	Array<CollisionInfo> ceilingCollisions;
	Array<CollisionInfo> wallCollisions;

	for (const auto& collision : collisions)
	{
		switch (collision.type)
		{
		case CollisionType::Ground:
			groundCollisions.push_back(collision);
			break;
		case CollisionType::Ceiling:
			ceilingCollisions.push_back(collision);
			break;
		case CollisionType::LeftWall:
		case CollisionType::RightWall:
			wallCollisions.push_back(collision);
			break;
		default:
			break;
		}
	}

	// 地面衝突を最優先で処理
	if (!groundCollisions.empty())
	{
		// 最も浅い地面衝突を選択
		CollisionInfo bestGroundCollision = groundCollisions[0];
		for (const auto& collision : groundCollisions)
		{
			if (collision.penetration < bestGroundCollision.penetration)
			{
				bestGroundCollision = collision;
			}
		}
		resolveCollision(body, bestGroundCollision);
	}

	// 天井衝突を処理
	if (!ceilingCollisions.empty())
	{
		CollisionInfo bestCeilingCollision = ceilingCollisions[0];
		for (const auto& collision : ceilingCollisions)
		{
			if (collision.penetration < bestCeilingCollision.penetration)
			{
				bestCeilingCollision = collision;
			}
		}
		resolveCollision(body, bestCeilingCollision);
	}

	// 壁衝突を処理
	if (!wallCollisions.empty())
	{
		CollisionInfo bestWallCollision = wallCollisions[0];
		for (const auto& collision : wallCollisions)
		{
			if (collision.penetration < bestWallCollision.penetration)
			{
				bestWallCollision = collision;
			}
		}
		resolveCollision(body, bestWallCollision);
	}
}

void CollisionSystem::resolveCollision(MovingBody& body, const CollisionInfo& collision) const
{
	if (!collision.hasCollision) return;

	const double minPenetration = 1.0; // 最小侵入量閾値を上げる

	switch (collision.type)
	{
	case CollisionType::Ground:
	{
		// 地面に着地 - Y座標を確実に補正
		if (collision.penetration > minPenetration)
		{
			body.position.y -= collision.penetration + SEPARATION_OFFSET;
		}
		if (body.velocity.y > 0)
		{
			body.velocity.y = 0;
		}
		body.isGrounded = true;
	}
	break;

	case CollisionType::Ceiling:
	{
		// 天井に衝突 - Y座標を確実に補正
		if (collision.penetration > minPenetration)
		{
			body.position.y += collision.penetration + SEPARATION_OFFSET;
		}
		if (body.velocity.y < 0)
		{
			body.velocity.y = 0;
		}
	}
	break;

	case CollisionType::LeftWall:
	{
		// 左の壁に衝突 - X座標を確実に補正
		if (collision.penetration > minPenetration)
		{
			body.position.x -= collision.penetration + SEPARATION_OFFSET;
		}
		// 横移動を完全に停止
		body.velocity.x = 0;
	}
	break;

	case CollisionType::RightWall:
	{
		// 右の壁に衝突 - X座標を確実に補正
		if (collision.penetration > minPenetration)
		{
			body.position.x += collision.penetration + SEPARATION_OFFSET;
		}
		// 横移動を完全に停止
		body.velocity.x = 0;
	}
	break;

	default:
		break;
	}
}

bool CollisionSystem::isGrounded(const MovingBody& body, const Array<RectF>& terrainRects) const
{
	// プレイヤーの足元より少し下の矩形で地面をチェック
	RectF groundCheckRect(
		body.position.x - body.size.x / 4,  // 幅を狭める
		body.position.y + body.size.y / 2,  // 足元から
		body.size.x / 2,                    // 半分の幅
		GROUND_TOLERANCE                    // 許容範囲
	);

	for (const auto& terrainRect : terrainRects)
	{
		if (groundCheckRect.intersects(terrainRect))
		{
			return true;
		}
	}

	return false;
}

Vec2 CollisionSystem::findNearestGroundPosition(const Vec2& position, const Array<RectF>& terrainRects) const
{
	double nearestGroundY = position.y + 1000.0;  // デフォルトは下の方
	bool foundGround = false;

	for (const auto& terrainRect : terrainRects)
	{
		// X座標が重なる地形ブロックを探す
		if (position.x >= terrainRect.x && position.x <= terrainRect.x + terrainRect.w)
		{
			// 現在位置より下にある地形の上面を探す
			if (terrainRect.y > position.y && terrainRect.y < nearestGroundY)
			{
				nearestGroundY = terrainRect.y;
				foundGround = true;
			}
		}
	}

	if (foundGround)
	{
		return Vec2(position.x, nearestGroundY);
	}
	else
	{
		// 地面が見つからない場合はデフォルト位置
		return Vec2(position.x, 850.0);  // ステージの地面レベル
	}
}

double CollisionSystem::calculatePenetration(const RectF& rectA, const RectF& rectB, CollisionType type) const
{
	switch (type)
	{
	case CollisionType::Ground:
	case CollisionType::Ceiling:
		return Math::Min(rectA.y + rectA.h - rectB.y, rectB.y + rectB.h - rectA.y);

	case CollisionType::LeftWall:
	case CollisionType::RightWall:
		return Math::Min(rectA.x + rectA.w - rectB.x, rectB.x + rectB.w - rectA.x);

	default:
		return 0.0;
	}
}

Vec2 CollisionSystem::calculateContactPoint(const RectF& rectA, const RectF& rectB) const
{
	// 二つの矩形の重なり部分の中心を接触点とする
	double left = Math::Max(rectA.x, rectB.x);
	double right = Math::Min(rectA.x + rectA.w, rectB.x + rectB.w);
	double top = Math::Max(rectA.y, rectB.y);
	double bottom = Math::Min(rectA.y + rectA.h, rectB.y + rectB.h);

	return Vec2((left + right) / 2.0, (top + bottom) / 2.0);
}

Vec2 CollisionSystem::calculateCollisionNormal(CollisionType type) const
{
	switch (type)
	{
	case CollisionType::Ground:   return Vec2(0, -1);
	case CollisionType::Ceiling:  return Vec2(0, 1);
	case CollisionType::LeftWall: return Vec2(1, 0);
	case CollisionType::RightWall:return Vec2(-1, 0);
	default:                      return Vec2(0, 0);
	}
}

CollisionInfo CollisionSystem::refineCollisionWithVelocity(const CollisionInfo& collision, const Vec2& velocity, const RectF& currentRect, const RectF& terrainRect) const
{
	// より正確な移動方向に基づく衝突判定
	CollisionInfo refinedCollision = collision;

	// 移動速度がほぼゼロの場合は元の判定をそのまま使用
	if (velocity.length() < 10.0)
	{
		return collision;
	}

	// 現在の位置と地形の位置関係を厳密にチェック
	const Vec2 currentCenter = Vec2(currentRect.x + currentRect.w / 2, currentRect.y + currentRect.h / 2);
	const Vec2 terrainCenter = Vec2(terrainRect.x + terrainRect.w / 2, terrainRect.y + terrainRect.h / 2);
	const Vec2 relativePos = currentCenter - terrainCenter;

	// 位置関係と移動方向の両方を考慮
	const double velocityThreshold = 20.0;

	if (Math::Abs(velocity.x) > Math::Abs(velocity.y) + 50.0)
	{
		// 水平移動が支配的
		if (velocity.x > velocityThreshold && relativePos.x < 0)
		{
			// 右向きに移動中で、現在地形の左側にいる → 右の壁に衝突
			refinedCollision.type = CollisionType::RightWall;
			refinedCollision.normal = Vec2(-1, 0);
			refinedCollision.penetration = (currentRect.x + currentRect.w) - terrainRect.x;
		}
		else if (velocity.x < -velocityThreshold && relativePos.x > 0)
		{
			// 左向きに移動中で、現在地形の右側にいる → 左の壁に衝突
			refinedCollision.type = CollisionType::LeftWall;
			refinedCollision.normal = Vec2(1, 0);
			refinedCollision.penetration = (terrainRect.x + terrainRect.w) - currentRect.x;
		}
	}
	else if (Math::Abs(velocity.y) > Math::Abs(velocity.x) + 50.0)
	{
		// 垂直移動が支配的
		if (velocity.y > velocityThreshold && relativePos.y < 0)
		{
			// 下向きに移動中で、現在地形の上側にいる → 地面に衝突
			refinedCollision.type = CollisionType::Ground;
			refinedCollision.normal = Vec2(0, -1);
			refinedCollision.penetration = (currentRect.y + currentRect.h) - terrainRect.y;
		}
		else if (velocity.y < -velocityThreshold && relativePos.y > 0)
		{
			// 上向きに移動中で、現在地形の下側にいる → 天井に衝突
			refinedCollision.type = CollisionType::Ceiling;
			refinedCollision.normal = Vec2(0, 1);
			refinedCollision.penetration = (terrainRect.y + terrainRect.h) - currentRect.y;
		}
	}

	return refinedCollision;
}
