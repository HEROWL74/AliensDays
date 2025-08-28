#include "CollisionSystem.hpp"
#include "../Player/Player.hpp"
#include "../Stages/Stage.hpp"
#include "BlockSystem.hpp"

void CollisionSystem::resolvePlayerCollisions(Player* player, Stage* stage, BlockSystem* blockSystem)
{
	if (!player || !stage) return;

	const Vec2 playerPos = player->getPosition();
	Vec2 playerVel = player->getVelocity();

	const double BLOCK_SIZE = 64.0;
	const double PLAYER_SIZE = BLOCK_SIZE - 4.0; // プレイヤーを少し小さく（60x60）
	const double halfSize = PLAYER_SIZE / 2.0;

	// 次フレームの予測位置を計算
	const double deltaTime = Scene::DeltaTime();
	Vec2 nextPos = playerPos + playerVel * deltaTime;

	// すべてのコリジョンレクトを統合
	Array<RectF> allCollisionRects = getUnifiedCollisionRects(stage, blockSystem);

	bool isGrounded = false;
	Vec2 finalPosition = nextPos;
	Vec2 finalVelocity = playerVel;

	// ★ 改良: X軸とY軸を分離して処理（より自然な移動）
	Vec2 intermediatePos = playerPos;

	// 1. X軸方向の移動を先に処理
	if (Math::Abs(playerVel.x) > 0.1)
	{
		Vec2 xTargetPos = Vec2(nextPos.x, playerPos.y);
		RectF xPlayerRect(xTargetPos.x - halfSize, xTargetPos.y - halfSize, PLAYER_SIZE, PLAYER_SIZE);

		bool xCollision = false;
		for (const auto& blockRect : allCollisionRects)
		{
			if (xPlayerRect.intersects(blockRect))
			{
				// ★ 改良: 壁際での自然な停止
				if (playerVel.x > 0)
				{
					// 右移動中の衝突
					double wallX = blockRect.x - halfSize - 0.5;
					intermediatePos.x = wallX;
				}
				else
				{
					// 左移動中の衝突  
					double wallX = blockRect.x + blockRect.w + halfSize + 0.5;
					intermediatePos.x = wallX;
				}

				// ★ 重要: 壁に当たった時の速度処理を改良
				finalVelocity.x = 0.0;
				xCollision = true;
				break;
			}
		}

		if (!xCollision)
		{
			intermediatePos.x = xTargetPos.x;
		}
	}
	else
	{
		intermediatePos.x = nextPos.x;
	}

	// 2. Y軸方向の移動を処理
	if (Math::Abs(playerVel.y) > 0.1)
	{
		Vec2 yTargetPos = Vec2(intermediatePos.x, nextPos.y);
		RectF yPlayerRect(yTargetPos.x - halfSize, yTargetPos.y - halfSize, PLAYER_SIZE, PLAYER_SIZE);

		bool yCollision = false;
		for (const auto& blockRect : allCollisionRects)
		{
			if (yPlayerRect.intersects(blockRect))
			{
				// Y軸衝突の処理
				if (playerVel.y > 0)
				{
					// 下移動中の衝突（着地）
					intermediatePos.y = blockRect.y - halfSize - 0.5;
					isGrounded = true;
				}
				else
				{
					// 上移動中の衝突（天井）
					intermediatePos.y = blockRect.y + blockRect.h + halfSize + 0.5;
				}
				finalVelocity.y = 0.0;
				yCollision = true;
				break;
			}
		}

		if (!yCollision)
		{
			intermediatePos.y = yTargetPos.y;
		}
	}
	else
	{
		intermediatePos.y = nextPos.y;
	}

	finalPosition = intermediatePos;

	// 改良された接地判定
	if (!isGrounded)
	{
		isGrounded = checkPreciseGroundContact(finalPosition, allCollisionRects);
	}

	// ★ 追加: 壁際でのスタック防止
	// 連続して同じ位置にいる場合は少し押し出す
	static Vec2 previousPos = finalPosition;
	static int stuckCounter = 0;

	if (finalPosition.distanceFrom(previousPos) < 0.5)
	{
		stuckCounter++;
		if (stuckCounter > 5) // 5フレーム連続で同じ位置
		{
			// わずかに押し出す
			if (Math::Abs(playerVel.x) > Math::Abs(playerVel.y))
			{
				finalPosition.x += (playerVel.x > 0) ? -1.0 : 1.0;
			}
			stuckCounter = 0;
		}
	}
	else
	{
		stuckCounter = 0;
	}
	previousPos = finalPosition;

	// プレイヤーの状態を更新
	player->setPosition(finalPosition);
	player->setVelocity(finalVelocity);
	player->setGrounded(isGrounded);

#ifdef _DEBUG
	debugCollisionInfo(finalPosition, finalVelocity, isGrounded, allCollisionRects.size());
#endif
}

CollisionSystem::CollisionResult CollisionSystem::resolveBlockCollision(
	const Vec2& currentPos, const Vec2& nextPos, const Vec2& velocity, const RectF& blockRect)
{
	CollisionResult result;
	result.hasCollision = false;
	result.correctedPosition = nextPos;
	result.correctedVelocity = velocity;

	const double BLOCK_SIZE = 64.0;
	const double PLAYER_SIZE = BLOCK_SIZE - 4.0; // 60x60
	const double halfSize = PLAYER_SIZE / 2.0;

	// 現在位置と次位置での矩形
	RectF currentRect(currentPos.x - halfSize, currentPos.y - halfSize, PLAYER_SIZE, PLAYER_SIZE);
	RectF nextRect(nextPos.x - halfSize, nextPos.y - halfSize, PLAYER_SIZE, PLAYER_SIZE);

	// 衝突判定
	if (!nextRect.intersects(blockRect))
	{
		return result; // 衝突しない
	}

	result.hasCollision = true;

	// 移動方向を判定
	const Vec2 movement = nextPos - currentPos;

	// ★ シンプルな方向判定
	if (Math::Abs(movement.x) > Math::Abs(movement.y))
	{
		// 水平移動が主体
		result.correctedVelocity.x = 0.0;
		result.correctedVelocity.y = velocity.y;

		if (movement.x > 0)
		{
			// 右移動
			result.correctedPosition.x = blockRect.x - halfSize - COLLISION_EPSILON;
			result.collisionType = CollisionType::RightWall;
		}
		else
		{
			// 左移動
			result.correctedPosition.x = blockRect.x + blockRect.w + halfSize + COLLISION_EPSILON;
			result.collisionType = CollisionType::LeftWall;
		}
	}
	else
	{
		// 垂直移動が主体
		result.correctedVelocity.x = velocity.x;
		result.correctedVelocity.y = 0.0;

		if (movement.y > 0)
		{
			// 下移動
			result.correctedPosition.y = blockRect.y - halfSize - COLLISION_EPSILON;
			result.collisionType = CollisionType::Ground;
		}
		else
		{
			// 上移動
			result.correctedPosition.y = blockRect.y + blockRect.h + halfSize + COLLISION_EPSILON;
			result.collisionType = CollisionType::Ceiling;
		}
	}

	return result;
}

Array<RectF> CollisionSystem::getUnifiedCollisionRects(Stage* stage, BlockSystem* blockSystem) const
{
	Array<RectF> allRects;

	// ステージの衝突矩形を追加
	if (stage)
	{
		const Array<RectF> stageRects = stage->getCollisionRects();
		allRects.append(stageRects);
	}

	// BlockSystemの衝突矩形を追加
	if (blockSystem)
	{
		const Array<RectF> blockRects = blockSystem->getCollisionRects();
		allRects.append(blockRects);
	}

	return allRects;
}

bool CollisionSystem::checkPreciseGroundContact(const Vec2& playerPos, const Array<RectF>& terrainRects) const
{
	const double PLAYER_SIZE = 64.0 - 4.0; // 60x60
	const double halfSize = PLAYER_SIZE / 2.0;
	const double GROUND_CHECK_HEIGHT = 2.0;

	// プレイヤーの足元の矩形（中央部分のみ）
	const double footMargin = 4.0;
	RectF groundCheckRect(
		playerPos.x - halfSize + footMargin,
		playerPos.y + halfSize,
		PLAYER_SIZE - footMargin * 2,
		GROUND_CHECK_HEIGHT
	);

	for (const auto& terrainRect : terrainRects)
	{
		if (groundCheckRect.intersects(terrainRect))
		{
			// プレイヤーの足がブロックの上面に接触しているか確認
			double playerBottom = playerPos.y + halfSize;
			double blockTop = terrainRect.y;

			if (Math::Abs(playerBottom - blockTop) <= GROUND_CHECK_HEIGHT)
			{
				// 水平方向の重複も確認
				double playerLeft = playerPos.x - halfSize + footMargin;
				double playerRight = playerPos.x + halfSize - footMargin;
				double blockLeft = terrainRect.x;
				double blockRight = terrainRect.x + terrainRect.w;

				// 最小限の重複があれば接地
				if (playerRight > blockLeft + 2.0 && playerLeft < blockRight - 2.0)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool CollisionSystem::canPlayerFitInGap(const Vec2& position, const Array<RectF>& terrainRects) const
{
	const double BLOCK_SIZE = 64.0;
	const double halfSize = BLOCK_SIZE / 2.0;

	// ★ 修正: プレイヤーサイズを少し小さくして隙間通過を容易に
	const double PLAYER_COLLISION_SIZE = BLOCK_SIZE - 2.0; // 2ピクセル小さく
	const double smallHalfSize = PLAYER_COLLISION_SIZE / 2.0;

	// プレイヤーサイズの矩形を作成（少し小さめ）
	RectF playerRect(
		position.x - smallHalfSize,
		position.y - smallHalfSize,
		PLAYER_COLLISION_SIZE,
		PLAYER_COLLISION_SIZE
	);

	// どのブロックとも衝突しなければ通れる
	for (const auto& terrainRect : terrainRects)
	{
		if (playerRect.intersects(terrainRect))
		{
			return false;
		}
	}
	return true;
}

RectF CollisionSystem::getPlayerRect(const Vec2& position, const Vec2& size) const
{
	// 新システムでは size パラメータは無視し、常にブロックサイズを使用
	const double BLOCK_SIZE = 64.0;
	const double halfSize = BLOCK_SIZE / 2.0;

	return RectF(position.x - halfSize, position.y - halfSize, BLOCK_SIZE, BLOCK_SIZE);
}

Vec2 CollisionSystem::findNearestGroundPosition(const Vec2& position, const Array<RectF>& terrainRects) const
{
	const double BLOCK_SIZE = 64.0;
	const double halfSize = BLOCK_SIZE / 2.0;

	double nearestGroundY = position.y + 1000.0;
	bool foundGround = false;

	// プレイヤーの水平位置（1ブロック幅）でチェック
	double playerLeft = position.x - halfSize;
	double playerRight = position.x + halfSize;

	for (const auto& terrainRect : terrainRects)
	{
		// ブロックとプレイヤーの水平方向の重複をチェック
		double blockLeft = terrainRect.x;
		double blockRight = terrainRect.x + terrainRect.w;

		// 重複判定（1ブロック基準）
		if (playerRight > blockLeft && playerLeft < blockRight)
		{
			// プレイヤーより下にあるブロック
			if (terrainRect.y > position.y && terrainRect.y < nearestGroundY)
			{
				nearestGroundY = terrainRect.y - halfSize;
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
		// デフォルト地面レベル（Stage の基本地面）
		return Vec2(position.x, 832.0 - halfSize);
	}
}

bool CollisionSystem::checkImprovedGroundContact(const Vec2& playerPos, const Vec2& playerSize,
											   const Array<RectF>& terrainRects) const
{
	// 新システムでは playerSize パラメータを無視
	return checkPreciseGroundContact(playerPos, terrainRects);
}

bool CollisionSystem::checkGroundContact(const Vec2& playerPos, const Vec2& playerSize,
									   const Array<RectF>& terrainRects) const
{
	// 新システムでは playerSize パラメータを無視
	return checkPreciseGroundContact(playerPos, terrainRects);
}

void CollisionSystem::checkSlidingBlockCollision(Player* player, const Vec2& playerPos, const Vec2& playerSize,
												const Array<RectF>& blockRects, BlockSystem* blockSystem) const
{
	if (!player || !blockSystem) return;

	const double BLOCK_SIZE = 64.0;
	const double halfSize = BLOCK_SIZE / 2.0;

	// スライディング時のプレイヤー矩形（高さを半分に）
	RectF slidingRect(
		playerPos.x - halfSize,
		playerPos.y,  // 上半分のみ
		BLOCK_SIZE,
		halfSize
	);

	for (const auto& blockRect : blockRects)
	{
		if (slidingRect.intersects(blockRect))
		{
			// ブロックとの相互作用を処理
			blockSystem->handlePlayerInteraction(player);
			break;
		}
	}
}

#ifdef _DEBUG
void CollisionSystem::debugCollisionInfo(const Vec2& position, const Vec2& velocity,
										bool grounded, size_t blockCount) const
{
	static int debugCounter = 0;
	if (debugCounter++ % 120 == 0) // 2秒ごと
	{
		Print << U"=== Collision Debug (64x64 Block System) ===";
		Print << U"Position: ({:.1f}, {:.1f})"_fmt(position.x, position.y);
		Print << U"Velocity: ({:.1f}, {:.1f})"_fmt(velocity.x, velocity.y);
		Print << U"Grounded: " << (grounded ? U"YES" : U"NO");
		Print << U"Block count: " << blockCount;

		// グリッド位置表示
		const double BLOCK_SIZE = 64.0;
		int gridX = static_cast<int>(position.x / BLOCK_SIZE);
		int gridY = static_cast<int>(position.y / BLOCK_SIZE);
		Print << U"Grid Position: ({}, {})"_fmt(gridX, gridY);
	}
}
#endif
