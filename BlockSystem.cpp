#include "BlockSystem.hpp"
#include "Player.hpp"
#include "Stage.hpp"
#include "SoundManager.hpp"

BlockSystem::BlockSystem()
	: m_coinsFromBlocks(0)
{
}

void BlockSystem::init()
{
	loadTextures();
	createBrickFragmentTextures();
	m_coinsFromBlocks = 0;
}

void BlockSystem::loadTextures()
{
	m_coinBlockActiveTexture = Texture(U"Sprites/Tiles/block_coin_active.png");
	m_coinBlockEmptyTexture = Texture(U"Sprites/Tiles/block_coin.png");
	m_brickBlockTexture = Texture(U"Sprites/Tiles/block_empty.png");

	if (!m_coinBlockActiveTexture) Print << U"Failed to load coin block active texture";
	if (!m_coinBlockEmptyTexture) Print << U"Failed to load coin block empty texture";
	if (!m_brickBlockTexture) Print << U"Failed to load brick block texture";
}

void BlockSystem::createBrickFragmentTextures()
{
	if (!m_brickBlockTexture) return;

	// レンガブロックを4分割した破片テクスチャを作成
	// この実装では簡単のため、元のテクスチャをそのまま使用
	for (int i = 0; i < 4; i++) {
		m_brickFragmentTextures.push_back(m_brickBlockTexture);
	}
}

void BlockSystem::update(Player* player)
{
	// 新しいシステムでは相互作用のみを処理
	handlePlayerInteraction(player);
}

Array<RectF> BlockSystem::getCollisionRects() const
{
	Array<RectF> collisionRects;
	collisionRects.reserve(m_blocks.size()); // パフォーマンス向上

	for (const auto& block : m_blocks)
	{
		if (block && isBlockSolid(*block))
		{
			// ★ 全てのブロックを64x64基準で統一
			RectF blockRect(block->position.x, block->position.y, BLOCK_SIZE, BLOCK_SIZE);
			collisionRects.push_back(blockRect);
		}
	}

	return collisionRects;
}


bool BlockSystem::hasBlockAt(const Vec2& position) const
{
	const double BLOCK_SIZE = 64.0;
	const double halfSize = BLOCK_SIZE / 2.0;

	// ★ 指定位置に1ブロック分の矩形でブロックがあるかチェック
	const RectF checkRect(
		position.x - halfSize,
		position.y - halfSize,
		BLOCK_SIZE,
		BLOCK_SIZE
	);

	for (const auto& block : m_blocks)
	{
		if (block && isBlockSolid(*block))
		{
			RectF blockRect = getBlockRect(*block);
			if (blockRect.intersects(checkRect))
			{
				return true;
			}
		}
	}
	return false;
}

void BlockSystem::handlePlayerInteraction(Player* player)
{
	if (!player) return;

	// ブロック叩きなどの相互作用のみを処理
	// 物理的な衝突判定は CollisionSystem に任せる
	updateBlockInteractions(player);
}

void BlockSystem::updateBlockInteractions(Player* player)
{
	if (!player) return;

	const double BLOCK_SIZE = 64.0;
	const Vec2 playerPos = player->getPosition();

	for (auto& block : m_blocks)
	{
		if (!block || block->state == BlockState::DESTROYED) continue;

		// バウンスアニメーション更新
		updateBlockAnimation(*block);

		// ★ 1ブロック基準での距離チェック
		const double distance = block->position.distanceFrom(playerPos);
		const double INTERACTION_RANGE = BLOCK_SIZE * 2.0; // 2ブロック分の範囲

		// デバウンス処理の条件を1ブロック基準で設定
		if (block->wasHit && distance > INTERACTION_RANGE)
		{
			block->wasHit = false;
		}

		// 通常のヒット判定
		if (!block->wasHit && checkPlayerHitFromBelow(*block, player))
		{
			block->wasHit = true;

			switch (block->type)
			{
			case BlockType::COIN_BLOCK:
				handleCoinBlockHit(*block);
				break;
			case BlockType::BRICK_BLOCK:
				handleBrickBlockHit(*block);
				break;
			}
		}

		// スライディング中の特別判定（1ブロック基準で調整）
		if (player->isSliding() && distance < BLOCK_SIZE * 1.5 && !block->wasHit)
		{
			const double halfSize = BLOCK_SIZE / 2.0;

			// スライディング時のプレイヤー矩形（高さを半分に）
			const RectF slidingPlayerRect(
				playerPos.x - halfSize,
				playerPos.y, // 上半分のみ
				BLOCK_SIZE,
				halfSize
			);

			const RectF blockRect = getBlockRect(*block);

			if (slidingPlayerRect.intersects(blockRect))
			{
				block->wasHit = true;

				switch (block->type)
				{
				case BlockType::COIN_BLOCK:
					handleCoinBlockHit(*block);
					break;
				case BlockType::BRICK_BLOCK:
					handleBrickBlockHit(*block);
					break;
				}
			}
		}
	}

	updateFragments();

	m_blocks.erase(
		std::remove_if(m_blocks.begin(), m_blocks.end(),
			[](const std::unique_ptr<Block>& block) {
				return !block || block->state == BlockState::DESTROYED;
			}),
		m_blocks.end()
	);

	m_fragments.erase(
		std::remove_if(m_fragments.begin(), m_fragments.end(),
			[](const std::unique_ptr<BlockFragment>& fragment) {
				return !fragment || fragment->life <= 0.0;
			}),
		m_fragments.end()
	);
}

void BlockSystem::updateBlockAnimation(Block& block)
{
	// バウンスアニメーション更新のみ
	if (block.bounceTimer > 0.0)
	{
		block.bounceTimer -= Scene::DeltaTime();

		if (block.bounceTimer <= 0.0)
		{
			block.bounceTimer = 0.0;
			block.bounceAnimation = 0.0;
		}
		else
		{
			const double progress = 1.0 - (block.bounceTimer / BOUNCE_DURATION);
			block.bounceAnimation = std::sin(progress * Math::Pi) * BOUNCE_HEIGHT;
		}
	}
}

void BlockSystem::updateFragments()
{
	const double deltaTime = Scene::DeltaTime();
	const double BLOCK_SIZE = 64.0;

	for (auto& fragment : m_fragments)
	{
		if (!fragment) continue;

		// 物理更新
		fragment->position += fragment->velocity * deltaTime;
		fragment->velocity.y += FRAGMENT_GRAVITY * deltaTime;

		// ★ 地面との衝突処理を1ブロック基準で設定
		const double GROUND_LEVEL = 12.5 * BLOCK_SIZE; // 地面レベル
		if (fragment->position.y >= GROUND_LEVEL && fragment->velocity.y > 0 && !fragment->bounced)
		{
			fragment->position.y = GROUND_LEVEL;
			fragment->velocity.y *= -0.4;  // バウンス
			fragment->velocity.x *= 0.8;   // 摩擦
			fragment->bounced = true;
			fragment->rotationSpeed *= 0.6;
		}

		fragment->velocity.x *= 0.995; // 空気抵抗
		fragment->rotation += fragment->rotationSpeed * deltaTime;
		fragment->life -= deltaTime;
	}
}

void BlockSystem::addBlockAtGrid(int gridX, int gridY, BlockType type)
{
	const double BLOCK_SIZE = 64.0;
	const Vec2 worldPos(gridX * BLOCK_SIZE, gridY * BLOCK_SIZE);

	switch (type)
	{
	case BlockType::COIN_BLOCK:
		addCoinBlock(worldPos);
		break;
	case BlockType::BRICK_BLOCK:
		addBrickBlock(worldPos);
		break;
	}
}

//指定グリッド範囲にブロックがあるかチェック
bool BlockSystem::hasBlockInGridRange(int startX, int startY, int width, int height) const
{
	const double BLOCK_SIZE = 64.0;

	for (int x = startX; x < startX + width; ++x)
	{
		for (int y = startY; y < startY + height; ++y)
		{
			Vec2 checkPos(x * BLOCK_SIZE + BLOCK_SIZE / 2, y * BLOCK_SIZE + BLOCK_SIZE / 2);
			if (hasBlockAt(checkPos))
			{
				return true;
			}
		}
	}
	return false;
}

void BlockSystem::handleCoinBlockHit(Block& block)
{
	if (block.state == BlockState::ACTIVE)
	{
		// コインブロックをアクティブから空に変更
		block.state = BlockState::EMPTY;
		block.texture = m_coinBlockEmptyTexture;

		// バウンスアニメーション開始
		block.bounceTimer = BOUNCE_DURATION;

		// コイン獲得
		m_coinsFromBlocks++;

		// サウンド再生
		SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_COIN);
	}
}

void BlockSystem::handleBrickBlockHit(Block& block)
{
	// レンガブロックを破壊
	block.state = BlockState::DESTROYED;

	// 破片を生成
	createBrickFragments(block.position);

	// サウンド再生
	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_BREAK_BLOCK);
}

void BlockSystem::createBrickFragments(const Vec2& blockPos)
{
	const double BLOCK_SIZE = 64.0;

	// ★ 4つの破片を1ブロック基準で作成
	for (int i = 0; i < 4; i++)
	{
		// 破片の初期位置（ブロック内の4分割位置）
		Vec2 fragmentPos = blockPos + Vec2(
			(i % 2) * (BLOCK_SIZE / 2) + (BLOCK_SIZE / 4),
			(i / 2) * (BLOCK_SIZE / 2) + (BLOCK_SIZE / 4)
		);

		// より自然な破片の飛び散り方を1ブロック基準で計算
		double baseVelX = (i % 2 == 0) ? -1.0 : 1.0;  // 左右方向
		double baseVelY = (i / 2 == 0) ? -1.5 : -0.8; // 上下方向

		// ランダム要素を追加
		double randomFactorX = Random(-0.5, 0.5);
		double randomFactorY = Random(0.0, 0.5);

		// ★ 速度を1ブロック基準で設定
		Vec2 velocity(
			baseVelX * (BLOCK_SIZE * 2.0 + randomFactorX * BLOCK_SIZE * 0.8), // 2-2.8ブロック/秒
			baseVelY * BLOCK_SIZE * 3.0 - randomFactorY * BLOCK_SIZE * 1.5    // 1.5-4.5ブロック/秒（上向き）
		);

		// 破片を追加
		if (i < m_brickFragmentTextures.size())
		{
			auto fragment = std::make_unique<BlockFragment>(
				fragmentPos, velocity, m_brickFragmentTextures[i]
			);

			// より自然な回転速度を設定
			fragment->rotationSpeed = Random(-10.0, 10.0);

			m_fragments.push_back(std::move(fragment));
		}
	}
}
void BlockSystem::draw(const Vec2& cameraOffset) const
{
	// ブロックの描画
	for (const auto& block : m_blocks)
	{
		if (block && block->state != BlockState::DESTROYED)
		{
			drawBlock(*block, cameraOffset);
		}
	}

	// 破片の描画
	drawFragments(cameraOffset);
}

void BlockSystem::drawBlock(const Block& block, const Vec2& cameraOffset) const
{
	// 画面座標に変換
	Vec2 screenPos = block.position - cameraOffset;
	screenPos.y -= block.bounceAnimation;

	// 画面外なら描画しない（最適化）
	if (screenPos.x < -BLOCK_SIZE || screenPos.x > Scene::Width() + BLOCK_SIZE) return;

	// テクスチャを決定
	Texture texture;
	switch (block.type)
	{
	case BlockType::COIN_BLOCK:
		texture = (block.state == BlockState::ACTIVE) ? m_coinBlockActiveTexture : m_coinBlockEmptyTexture;
		break;
	case BlockType::BRICK_BLOCK:
		texture = m_brickBlockTexture;
		break;
	}

	// ブロック描画
	if (texture)
	{
		texture.resized(BLOCK_SIZE, BLOCK_SIZE).draw(screenPos);
	}
	else
	{
		// フォールバック：色付きの矩形
		ColorF fallbackColor = (block.type == BlockType::COIN_BLOCK) ?
			ColorF(1.0, 1.0, 0.0) : ColorF(0.6, 0.3, 0.1);
		RectF(screenPos, BLOCK_SIZE, BLOCK_SIZE).draw(fallbackColor);
	}

#ifdef _DEBUG
	// デバッグ用当たり判定表示
	RectF(screenPos, BLOCK_SIZE, BLOCK_SIZE).drawFrame(2.0, ColorF(1.0, 0.0, 0.0, 0.5));
#endif
}

void BlockSystem::drawFragments(const Vec2& cameraOffset) const
{
	for (const auto& fragment : m_fragments)
	{
		if (!fragment) continue;

		// 画面座標に変換
		Vec2 screenPos = fragment->position - cameraOffset;

		// 画面外なら描画しない
		if (screenPos.x < -64 || screenPos.x > Scene::Width() + 64) continue;

		// 透明度を生存期間に応じて設定
		double lifeRatio = fragment->life / FRAGMENT_LIFE;
		double alpha = lifeRatio;

		if (alpha > 0 && fragment->texture)
		{
			const double fragmentSize = 32.0;

			if (fragment->rotation != 0.0)
			{
				Vec2 center = screenPos + Vec2(fragmentSize / 2, fragmentSize / 2);
				fragment->texture.resized(fragmentSize, fragmentSize)
					.rotated(fragment->rotation)
					.drawAt(center, ColorF(1.0, 1.0, 1.0, alpha));
			}
			else
			{
				fragment->texture.resized(fragmentSize, fragmentSize)
					.draw(screenPos, ColorF(1.0, 1.0, 1.0, alpha));
			}
		}
	}
}

bool BlockSystem::checkPlayerHitFromBelow(const Block& block, Player* player) const
{
	if (!player) return false;

	const Vec2 playerPos = player->getPosition();
	const Vec2 playerVel = player->getVelocity();
	const double BLOCK_SIZE = 64.0;

	// ★ 重要: ブロック叩き判定を厳格化
	const double halfSize = BLOCK_SIZE / 2.0;

	// プレイヤーの衝突矩形（1ブロックサイズ）
	const RectF playerRect(
		playerPos.x - halfSize,
		playerPos.y - halfSize,
		BLOCK_SIZE,
		BLOCK_SIZE
	);

	// ブロックの衝突矩形
	const RectF blockRect(block.position.x, block.position.y, BLOCK_SIZE, BLOCK_SIZE);

	// X軸の重なり判定（1ブロック基準）
	const double playerLeft = playerRect.x;
	const double playerRight = playerRect.x + playerRect.w;
	const double blockLeft = blockRect.x;
	const double blockRight = blockRect.x + blockRect.w;

	// 水平方向の十分な重なりがあるかチェック（最低30%の重なりが必要）
	const double overlapLeft = Math::Max(playerLeft, blockLeft);
	const double overlapRight = Math::Min(playerRight, blockRight);
	const double overlapWidth = overlapRight - overlapLeft;
	const double minOverlap = BLOCK_SIZE * 0.3; // 30%以上の重なりが必要

	if (overlapWidth < minOverlap) return false;

	// Y軸の位置関係判定（より厳格に）
	const double playerTop = playerRect.y;
	const double playerBottom = playerRect.y + playerRect.h;
	const double blockTop = blockRect.y;
	const double blockBottom = blockRect.y + blockRect.h;

	// ★ 修正: より厳格な下からのヒット判定
	const double HIT_TOLERANCE = BLOCK_SIZE * 0.2; // 許容範囲を狭く

	// プレイヤーの上端がブロックの下端付近にあるかチェック
	bool isPlayerBelow = (playerTop > blockBottom - HIT_TOLERANCE) &&
		(playerTop < blockBottom + HIT_TOLERANCE);

	// ★ 重要: 上向きの動きがあるかの厳格なチェック
	bool isMovingUp = false;

	if (player->getCurrentState() == PlayerState::Jump && playerVel.y < -50.0)
	{
		// ジャンプ状態で十分な上向き速度がある
		isMovingUp = true;
	}
	else if (player->isInJumpState() && playerVel.y < -30.0)
	{
		// ジャンプ状態タイマーが有効で上向き速度がある
		isMovingUp = true;
	}

	// スライディング中の特別判定は距離をより厳格に
	bool isSlidingHit = player->isSliding() &&
		std::abs(playerTop - blockBottom) <= HIT_TOLERANCE * 0.5; // より狭い範囲

	// ★ 追加条件: プレイヤーが前フレームより上に移動している
	static Vec2 previousPlayerPos = playerPos;
	bool isMovingUpward = (playerPos.y < previousPlayerPos.y - 1.0);
	previousPlayerPos = playerPos;

	return (isPlayerBelow && isMovingUp && isMovingUpward) || isSlidingHit;
}

// ★ レガシーメソッド（互換性のため保持、段階的削除予定）
bool BlockSystem::checkCollision(const RectF& playerRect) const
{
	for (const auto& block : m_blocks)
	{
		if (!isBlockSolid(*block)) continue;

		const RectF blockRect = getBlockRect(*block);
		if (checkAABBCollision(playerRect, blockRect))
		{
			return true;
		}
	}
	return false;
}

void BlockSystem::handlePlayerCollision(Player* player)
{
	// このメソッドは新しいシステムでは使用されない
	// 互換性のため空実装で保持
	if (!player) return;

	// 新しいシステムでは CollisionSystem が物理衝突を処理し、
	// handlePlayerInteraction() がゲームロジックを処理する
}

bool BlockSystem::checkPlayerLandingOnBlocks(const RectF& playerRect) const
{
	// レガシーメソッド - 新しいシステムでは CollisionSystem が処理
	return false;
}

void BlockSystem::handleBlockLanding(Player* player)
{
	// レガシーメソッド - 新しいシステムでは CollisionSystem が処理
}

double BlockSystem::findNearestBlockTop(const Vec2& playerPos) const
{
	// レガシーメソッド - 新しいシステムでは CollisionSystem が処理
	return -1.0;
}

bool BlockSystem::checkAABBCollision(const RectF& rect1, const RectF& rect2) const
{
	return rect1.intersects(rect2);
}

void BlockSystem::resolveCollision(Player* player, const Block& block)
{
	// レガシーメソッド - 新しいシステムでは CollisionSystem が処理
}

bool BlockSystem::isBlockSolid(const Block& block) const
{
	// 破壊されたブロックは固体ではない
	if (block.state == BlockState::DESTROYED) return false;

	// コインブロックは空になっても固体
	return true;
}

RectF BlockSystem::getBlockRect(const Block& block) const
{
	// ★ 全てのブロックを64x64基準で統一
	return RectF(block.position, BLOCK_SIZE, BLOCK_SIZE);
}


void BlockSystem::addCoinBlock(const Vec2& position)
{
	auto block = std::make_unique<Block>(position, BlockType::COIN_BLOCK);
	if (block) {
		block->texture = m_coinBlockActiveTexture;
		m_blocks.push_back(std::move(block));
	}
}

void BlockSystem::addBrickBlock(const Vec2& position)
{
	auto block = std::make_unique<Block>(position, BlockType::BRICK_BLOCK);
	if (block) {
		block->texture = m_brickBlockTexture;
		m_blocks.push_back(std::move(block));
	}
}

void BlockSystem::clearAllBlocks()
{
	m_blocks.clear();
	m_fragments.clear();
	m_coinsFromBlocks = 0;
}

int BlockSystem::getActiveBlockCount() const
{
	int count = 0;
	for (const auto& block : m_blocks) {
		if (block && block->state != BlockState::DESTROYED) {
			count++;
		}
	}
	return count;
}

void BlockSystem::generateBlocksForStage(StageNumber stageNumber)
{
	switch (stageNumber) {
	case StageNumber::Stage1:
		generateBlocksForGrassStage();
		break;
	case StageNumber::Stage2:
		generateBlocksForSandStage();
		break;
	case StageNumber::Stage3:
		generateBlocksForPurpleStage();
		break;
	case StageNumber::Stage4:
		generateBlocksForSnowStage();
		break;
	case StageNumber::Stage5:
		generateBlocksForStoneStage();
		break;
	case StageNumber::Stage6:
		generateBlocksForDirtStage();
		break;
	}
}

void BlockSystem::generateBlocksForGrassStage()
{
	clearAllBlocks();

	// Stage1: 草原ステージ - 横スクロールアクションとして楽しめる配置
	// ブロックは離れた位置に配置し、ジャンプでのみ到達可能にする
	// 
	// 低い位置の基本ブロック群
	Array<Vec2> coinBlocks = {
		Vec2(6 * 64, 10 * 64),   // X: 384, Y: 640 - 地面から少し上
		Vec2(12 * 64, 11 * 64),  // X: 768, Y: 704 - 低い位置
		Vec2(18 * 64, 9 * 64),   // X: 1152, Y: 576 - ジャンプ必要
		Vec2(24 * 64, 10 * 64),  // X: 1536, Y: 640
	};

	Array<Vec2> brickBlocks = {
		Vec2(9 * 64, 11 * 64),   // X: 576, Y: 704
		Vec2(15 * 64, 10 * 64),  // X: 960, Y: 640
		Vec2(21 * 64, 8 * 64),   // X: 1344, Y: 512 - 高い位置
	};

	// === 中盤エリア (1600-3200px)
	// より高い位置と間隔の広いブロック
	coinBlocks.append({
		Vec2(30 * 64, 7 * 64),   // X: 1920, Y: 448 - 高い位置
		Vec2(36 * 64, 9 * 64),   // X: 2304, Y: 576
		Vec2(42 * 64, 6 * 64),   // X: 2688, Y: 384 - かなり高い
		Vec2(48 * 64, 8 * 64),   // X: 3072, Y: 512
	});

	brickBlocks.append({
		Vec2(27 * 64, 10 * 64),  // X: 1728, Y: 640
		Vec2(33 * 64, 8 * 64),   // X: 2112, Y: 512
		Vec2(39 * 64, 7 * 64),   // X: 2496, Y: 448
		Vec2(45 * 64, 9 * 64),   // X: 2880, Y: 576
	});

	// === 終盤エリア (3200-4800px)
	// 離れた位置に配置し、正確なジャンプが必要
	coinBlocks.append({
		Vec2(54 * 64, 5 * 64),   // X: 3456, Y: 320 - 非常に高い
		Vec2(60 * 64, 7 * 64),   // X: 3840, Y: 448
		Vec2(66 * 64, 4 * 64),   // X: 4224, Y: 256 - 最高点
		Vec2(72 * 64, 8 * 64),   // X: 4608, Y: 512
	});

	brickBlocks.append({
		Vec2(51 * 64, 9 * 64),   // X: 3264, Y: 576
		Vec2(57 * 64, 6 * 64),   // X: 3648, Y: 384
		Vec2(63 * 64, 8 * 64),   // X: 4032, Y: 512
		Vec2(69 * 64, 7 * 64),   // X: 4416, Y: 448
	});

	// ブロックを配置
	for (const auto& pos : coinBlocks) {
		addCoinBlock(pos);
	}

	for (const auto& pos : brickBlocks) {
		addBrickBlock(pos);
	}
}
void BlockSystem::generateBlocksForSandStage()
{
	clearAllBlocks();

	// コインブロックの配置（64の倍数座標、空中プラットフォームと重複回避）
	Array<Vec2> coinBlocks = {
		// 前半エリア（空中プラットフォームを避けた位置）
		Vec2(4 * 64, 10 * 64),   // X: 256, Y: 640
		Vec2(9 * 64, 8 * 64),    // X: 576, Y: 512
		Vec2(14 * 64, 6 * 64),   // X: 896, Y: 384
		Vec2(19 * 64, 9 * 64),   // X: 1216, Y: 576

		// 中間エリア（空中プラットフォームを避けた位置）
		Vec2(22 * 64, 7 * 64),   // X: 1408, Y: 448
		Vec2(27 * 64, 5 * 64),   // X: 1728, Y: 320
		Vec2(33 * 64, 8 * 64),   // X: 2112, Y: 512
		Vec2(38 * 64, 6 * 64),   // X: 2432, Y: 384

		// 後半エリア（空中プラットフォームを避けた位置）
		Vec2(41 * 64, 7 * 64),   // X: 2624, Y: 448
		Vec2(46 * 64, 5 * 64),   // X: 2944, Y: 320
		Vec2(50 * 64, 8 * 64),   // X: 3200, Y: 512
		Vec2(55 * 64, 6 * 64),   // X: 3520, Y: 384

		// 終盤エリア（空中プラットフォームを避けた位置）
		Vec2(60 * 64, 7 * 64),   // X: 3840, Y: 448
		Vec2(65 * 64, 5 * 64),   // X: 4160, Y: 320
		Vec2(69 * 64, 8 * 64),   // X: 4416, Y: 512
		Vec2(73 * 64, 6 * 64)    // X: 4672, Y: 384
	};

	// レンガブロックの配置（64の倍数座標、他のブロックと重複回避）
	Array<Vec2> brickBlocks = {
		// 前半エリア
		Vec2(6 * 64, 11 * 64),   // X: 384, Y: 704
		Vec2(11 * 64, 9 * 64),   // X: 704, Y: 576
		Vec2(16 * 64, 7 * 64),   // X: 1024, Y: 448
		Vec2(20 * 64, 10 * 64),  // X: 1280, Y: 640

		// 中間エリア
		Vec2(24 * 64, 8 * 64),   // X: 1536, Y: 512
		Vec2(29 * 64, 6 * 64),   // X: 1856, Y: 384
		Vec2(35 * 64, 9 * 64),   // X: 2240, Y: 576
		Vec2(39 * 64, 7 * 64),   // X: 2496, Y: 448

		// 後半エリア
		Vec2(43 * 64, 8 * 64),   // X: 2752, Y: 512
		Vec2(47 * 64, 6 * 64),   // X: 3008, Y: 384
		Vec2(52 * 64, 9 * 64),   // X: 3328, Y: 576
		Vec2(57 * 64, 7 * 64),   // X: 3648, Y: 448

		// 終盤エリア
		Vec2(61 * 64, 8 * 64),   // X: 3904, Y: 512
		Vec2(66 * 64, 6 * 64),   // X: 4224, Y: 384
		Vec2(70 * 64, 9 * 64),   // X: 4480, Y: 576
		Vec2(74 * 64, 7 * 64)    // X: 4736, Y: 448
	};

	// ブロックを配置
	for (const auto& pos : coinBlocks) {
		addCoinBlock(pos);
	}

	for (const auto& pos : brickBlocks) {
		addBrickBlock(pos);
	}
}

void BlockSystem::generateBlocksForPurpleStage()
{
	clearAllBlocks();

	// コインブロックの配置（64の倍数座標、空中プラットフォームと重複回避）
	Array<Vec2> coinBlocks = {
		// 前半エリア（空中プラットフォームを避けた位置）
		Vec2(3 * 64, 10 * 64),   // X: 192, Y: 640
		Vec2(8 * 64, 8 * 64),    // X: 512, Y: 512
		Vec2(13 * 64, 6 * 64),   // X: 832, Y: 384
		Vec2(17 * 64, 9 * 64),   // X: 1088, Y: 576

		// 中間エリア（空中プラットフォームを避けた位置）
		Vec2(21 * 64, 7 * 64),   // X: 1344, Y: 448
		Vec2(26 * 64, 5 * 64),   // X: 1664, Y: 320
		Vec2(32 * 64, 8 * 64),   // X: 2048, Y: 512
		Vec2(36 * 64, 6 * 64),   // X: 2304, Y: 384

		// 後半エリア（空中プラットフォームを避けた位置）
		Vec2(40 * 64, 7 * 64),   // X: 2560, Y: 448
		Vec2(45 * 64, 5 * 64),   // X: 2880, Y: 320
		Vec2(48 * 64, 8 * 64),   // X: 3072, Y: 512
		Vec2(54 * 64, 6 * 64),   // X: 3456, Y: 384

		// 終盤エリア（空中プラットフォームを避けた位置）
		Vec2(59 * 64, 7 * 64),   // X: 3776, Y: 448
		Vec2(63 * 64, 5 * 64),   // X: 4032, Y: 320
		Vec2(67 * 64, 8 * 64),   // X: 4288, Y: 512
		Vec2(71 * 64, 6 * 64)    // X: 4544, Y: 384
	};

	// レンガブロックの配置（64の倍数座標、他のブロックと重複回避）
	Array<Vec2> brickBlocks = {
		// 前半エリア
		Vec2(5 * 64, 11 * 64),   // X: 320, Y: 704
		Vec2(10 * 64, 9 * 64),   // X: 640, Y: 576
		Vec2(15 * 64, 7 * 64),   // X: 960, Y: 448
		Vec2(19 * 64, 10 * 64),  // X: 1216, Y: 640

		// 中間エリア
		Vec2(23 * 64, 8 * 64),   // X: 1472, Y: 512
		Vec2(28 * 64, 6 * 64),   // X: 1792, Y: 384
		Vec2(34 * 64, 9 * 64),   // X: 2176, Y: 576
		Vec2(38 * 64, 7 * 64),   // X: 2432, Y: 448

		// 後半エリア
		Vec2(42 * 64, 8 * 64),   // X: 2688, Y: 512
		Vec2(46 * 64, 6 * 64),   // X: 2944, Y: 384
		Vec2(51 * 64, 9 * 64),   // X: 3264, Y: 576
		Vec2(56 * 64, 7 * 64),   // X: 3584, Y: 448

		// 終盤エリア
		Vec2(62 * 64, 8 * 64),   // X: 3968, Y: 512
		Vec2(65 * 64, 6 * 64),   // X: 4160, Y: 384
		Vec2(68 * 64, 9 * 64),   // X: 4352, Y: 576
		Vec2(72 * 64, 7 * 64)    // X: 4608, Y: 448
	};

	// ブロックを配置
	for (const auto& pos : coinBlocks) {
		addCoinBlock(pos);
	}

	for (const auto& pos : brickBlocks) {
		addBrickBlock(pos);
	}
}

void BlockSystem::generateBlocksForSnowStage()
{
	clearAllBlocks();

	// コインブロックの配置（64の倍数座標、空中プラットフォームと重複回避）
	Array<Vec2> coinBlocks = {
		// 前半エリア（空中プラットフォームを避けた位置）
		Vec2(7 * 64, 10 * 64),   // X: 448, Y: 640
		Vec2(11 * 64, 8 * 64),   // X: 704, Y: 512
		Vec2(15 * 64, 6 * 64),   // X: 960, Y: 384
		Vec2(21 * 64, 9 * 64),   // X: 1344, Y: 576

		// 中間エリア（空中プラットフォームを避けた位置）
		Vec2(24 * 64, 7 * 64),   // X: 1536, Y: 448
		Vec2(29 * 64, 5 * 64),   // X: 1856, Y: 320
		Vec2(34 * 64, 8 * 64),   // X: 2176, Y: 512
		Vec2(39 * 64, 6 * 64),   // X: 2496, Y: 384

		// 後半エリア（空中プラットフォームを避けた位置）
		Vec2(42 * 64, 7 * 64),   // X: 2688, Y: 448
		Vec2(47 * 64, 5 * 64),   // X: 3008, Y: 320
		Vec2(51 * 64, 8 * 64),   // X: 3264, Y: 512
		Vec2(56 * 64, 6 * 64),   // X: 3584, Y: 384

		// 終盤エリア（空中プラットフォームを避けた位置）
		Vec2(61 * 64, 7 * 64),   // X: 3904, Y: 448
		Vec2(66 * 64, 5 * 64),   // X: 4224, Y: 320
		Vec2(70 * 64, 8 * 64),   // X: 4480, Y: 512
		Vec2(75 * 64, 6 * 64)    // X: 4800, Y: 384
	};

	// レンガブロックの配置（64の倍数座標、他のブロックと重複回避）
	Array<Vec2> brickBlocks = {
		// 前半エリア
		Vec2(8 * 64, 11 * 64),   // X: 512, Y: 704
		Vec2(13 * 64, 9 * 64),   // X: 832, Y: 576
		Vec2(17 * 64, 7 * 64),   // X: 1088, Y: 448
		Vec2(22 * 64, 10 * 64),  // X: 1408, Y: 640

		// 中間エリア
		Vec2(26 * 64, 8 * 64),   // X: 1664, Y: 512
		Vec2(30 * 64, 6 * 64),   // X: 1920, Y: 384
		Vec2(36 * 64, 9 * 64),   // X: 2304, Y: 576
		Vec2(40 * 64, 7 * 64),   // X: 2560, Y: 448

		// 後半エリア
		Vec2(45 * 64, 8 * 64),   // X: 2880, Y: 512
		Vec2(48 * 64, 6 * 64),   // X: 3072, Y: 384
		Vec2(54 * 64, 9 * 64),   // X: 3456, Y: 576
		Vec2(58 * 64, 7 * 64),   // X: 3712, Y: 448

		// 終盤エリア
		Vec2(63 * 64, 8 * 64),   // X: 4032, Y: 512
		Vec2(67 * 64, 6 * 64),   // X: 4288, Y: 384
		Vec2(72 * 64, 9 * 64),   // X: 4608, Y: 576
		Vec2(76 * 64, 7 * 64)    // X: 4864, Y: 448
	};

	// ブロックを配置
	for (const auto& pos : coinBlocks) {
		addCoinBlock(pos);
	}

	for (const auto& pos : brickBlocks) {
		addBrickBlock(pos);
	}
}

void BlockSystem::generateBlocksForStoneStage()
{
	clearAllBlocks();

	// コインブロックの配置（64の倍数座標、空中プラットフォームと重複回避）
	Array<Vec2> coinBlocks = {
		// 前半エリア（空中プラットフォームを避けた位置）  
		Vec2(4 * 64, 10 * 64),   // X: 256, Y: 640
		Vec2(10 * 64, 8 * 64),   // X: 640, Y: 512
		Vec2(14 * 64, 6 * 64),   // X: 896, Y: 384
		Vec2(20 * 64, 9 * 64),   // X: 1280, Y: 576

		// 中間エリア（空中プラットフォームを避けた位置）
		Vec2(23 * 64, 7 * 64),   // X: 1472, Y: 448
		Vec2(28 * 64, 5 * 64),   // X: 1792, Y: 320
		Vec2(33 * 64, 8 * 64),   // X: 2112, Y: 512
		Vec2(38 * 64, 6 * 64),   // X: 2432, Y: 384

		// 後半エリア（空中プラットフォームを避けた位置）
		Vec2(41 * 64, 7 * 64),   // X: 2624, Y: 448
		Vec2(46 * 64, 5 * 64),   // X: 2944, Y: 320
		Vec2(50 * 64, 8 * 64),   // X: 3200, Y: 512
		Vec2(55 * 64, 6 * 64),   // X: 3520, Y: 384

		// 終盤エリア（空中プラットフォームを避けた位置）
		Vec2(60 * 64, 7 * 64),   // X: 3840, Y: 448
		Vec2(65 * 64, 5 * 64),   // X: 4160, Y: 320
		Vec2(69 * 64, 8 * 64),   // X: 4416, Y: 512
		Vec2(73 * 64, 6 * 64)    // X: 4672, Y: 384
	};

	// レンガブロックの配置（64の倍数座標、他のブロックと重複回避）
	Array<Vec2> brickBlocks = {
		// 前半エリア
		Vec2(7 * 64, 11 * 64),   // X: 448, Y: 704
		Vec2(12 * 64, 9 * 64),   // X: 768, Y: 576
		Vec2(16 * 64, 7 * 64),   // X: 1024, Y: 448
		Vec2(21 * 64, 10 * 64),  // X: 1344, Y: 640

		// 中間エリア
		Vec2(25 * 64, 8 * 64),   // X: 1600, Y: 512
		Vec2(30 * 64, 6 * 64),   // X: 1920, Y: 384
		Vec2(35 * 64, 9 * 64),   // X: 2240, Y: 576
		Vec2(39 * 64, 7 * 64),   // X: 2496, Y: 448

		// 後半エリア
		Vec2(43 * 64, 8 * 64),   // X: 2752, Y: 512
		Vec2(47 * 64, 6 * 64),   // X: 3008, Y: 384
		Vec2(52 * 64, 9 * 64),   // X: 3328, Y: 576
		Vec2(57 * 64, 7 * 64),   // X: 3648, Y: 448

		// 終盤エリア
		Vec2(62 * 64, 8 * 64),   // X: 3968, Y: 512
		Vec2(66 * 64, 6 * 64),   // X: 4224, Y: 384
		Vec2(71 * 64, 9 * 64),   // X: 4544, Y: 576
		Vec2(74 * 64, 7 * 64)    // X: 4736, Y: 448
	};

	// ブロックを配置
	for (const auto& pos : coinBlocks) {
		addCoinBlock(pos);
	}

	for (const auto& pos : brickBlocks) {
		addBrickBlock(pos);
	}
}

void BlockSystem::generateBlocksForDirtStage()
{
	clearAllBlocks();

	// コインブロックの配置（64の倍数座標、空中プラットフォームと重複回避）
	Array<Vec2> coinBlocks = {
		// 前半エリア（空中プラットフォームを避けた位置）
		Vec2(3 * 64, 10 * 64),   // X: 192, Y: 640
		Vec2(9 * 64, 8 * 64),    // X: 576, Y: 512
		Vec2(13 * 64, 6 * 64),   // X: 832, Y: 384
		Vec2(19 * 64, 9 * 64),   // X: 1216, Y: 576

		// 中間エリア（空中プラットフォームを避けた位置）
		Vec2(22 * 64, 7 * 64),   // X: 1408, Y: 448
		Vec2(27 * 64, 5 * 64),   // X: 1728, Y: 320
		Vec2(32 * 64, 8 * 64),   // X: 2048, Y: 512
		Vec2(36 * 64, 6 * 64),   // X: 2304, Y: 384

		// 後半エリア（空中プラットフォームを避けた位置）
		Vec2(40 * 64, 7 * 64),   // X: 2560, Y: 448
		Vec2(45 * 64, 5 * 64),   // X: 2880, Y: 320
		Vec2(48 * 64, 8 * 64),   // X: 3072, Y: 512
		Vec2(54 * 64, 6 * 64),   // X: 3456, Y: 384

		// 終盤エリア（空中プラットフォームを避けた位置）
		Vec2(59 * 64, 7 * 64),   // X: 3776, Y: 448
		Vec2(63 * 64, 5 * 64),   // X: 4032, Y: 320
		Vec2(67 * 64, 8 * 64),   // X: 4288, Y: 512
		Vec2(71 * 64, 6 * 64)    // X: 4544, Y: 384
	};

	// レンガブロックの配置（64の倍数座標、他のブロックと重複回避）
	Array<Vec2> brickBlocks = {
		// 前半エリア
		Vec2(5 * 64, 11 * 64),   // X: 320, Y: 704
		Vec2(11 * 64, 9 * 64),   // X: 704, Y: 576
		Vec2(15 * 64, 7 * 64),   // X: 960, Y: 448
		Vec2(20 * 64, 10 * 64),  // X: 1280, Y: 640

		// 中間エリア
		Vec2(24 * 64, 8 * 64),   // X: 1536, Y: 512
		Vec2(29 * 64, 6 * 64),   // X: 1856, Y: 384
		Vec2(34 * 64, 9 * 64),   // X: 2176, Y: 576
		Vec2(38 * 64, 7 * 64),   // X: 2432, Y: 448

		// 後半エリア
		Vec2(42 * 64, 8 * 64),   // X: 2688, Y: 512
		Vec2(46 * 64, 6 * 64),   // X: 2944, Y: 384
		Vec2(51 * 64, 9 * 64),   // X: 3264, Y: 576
		Vec2(56 * 64, 7 * 64),   // X: 3584, Y: 448

		// 終盤エリア
		Vec2(61 * 64, 8 * 64),   // X: 3904, Y: 512
		Vec2(65 * 64, 6 * 64),   // X: 4160, Y: 384
		Vec2(68 * 64, 9 * 64),   // X: 4352, Y: 576
		Vec2(72 * 64, 7 * 64)    // X: 4608, Y: 448
	};

	// ブロックを配置
	for (const auto& pos : coinBlocks) {
		addCoinBlock(pos);
	}

	for (const auto& pos : brickBlocks) {
		addBrickBlock(pos);
	}
}
