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

// ★ 新しい統一衝突判定システム用メソッド
Array<RectF> BlockSystem::getCollisionRects() const
{
	Array<RectF> collisionRects;

	for (const auto& block : m_blocks)
	{
		if (block && isBlockSolid(*block))
		{
			RectF blockRect = getBlockRect(*block);
			collisionRects.push_back(blockRect);
		}
	}

	return collisionRects;
}

bool BlockSystem::hasBlockAt(const Vec2& position) const
{
	for (const auto& block : m_blocks)
	{
		if (block && isBlockSolid(*block))
		{
			RectF blockRect = getBlockRect(*block);
			if (blockRect.contains(position))
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

	// 全てのブロックを更新（バウンスアニメーション、ヒット判定など）
	for (auto& block : m_blocks)
	{
		if (block && block->state != BlockState::DESTROYED)
		{
			// バウンスアニメーション更新
			updateBlockAnimation(*block);

			// 下からのヒット判定（ブロック叩き）
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

			// ヒットフラグのリセット条件
			if (block->wasHit && !checkPlayerHitFromBelow(*block, player))
			{
				const Vec2 playerPos = player->getPosition();
				const double distance = block->position.distanceFrom(playerPos);
				if (distance > HIT_DETECTION_SIZE * 1.5)
				{
					block->wasHit = false;
				}
			}
		}
	}

	// 破片更新
	updateFragments();

	// 非アクティブなブロックを削除
	m_blocks.erase(
		std::remove_if(m_blocks.begin(), m_blocks.end(),
			[](const std::unique_ptr<Block>& block) {
				return !block || block->state == BlockState::DESTROYED;
			}),
		m_blocks.end()
	);

	// 非アクティブな破片を削除
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

	for (auto& fragment : m_fragments)
	{
		if (!fragment) continue;

		// 物理更新
		fragment->position += fragment->velocity * deltaTime;
		fragment->velocity.y += FRAGMENT_GRAVITY * deltaTime;

		// 地面との衝突処理
		const double GROUND_LEVEL = 800.0;
		if (fragment->position.y >= GROUND_LEVEL && fragment->velocity.y > 0 && !fragment->bounced)
		{
			fragment->position.y = GROUND_LEVEL;
			fragment->velocity.y *= -0.4;
			fragment->velocity.x *= 0.8;
			fragment->bounced = true;
			fragment->rotationSpeed *= 0.6;
		}

		fragment->velocity.x *= 0.995;
		fragment->rotation += fragment->rotationSpeed * deltaTime;
		fragment->life -= deltaTime;
	}
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
	// 4つの破片を作成（左上、右上、左下、右下）
	for (int i = 0; i < 4; i++)
	{
		// 破片の初期位置（ブロック内の4分割位置）
		Vec2 fragmentPos = blockPos + Vec2(
			(i % 2) * (BLOCK_SIZE / 2) + (BLOCK_SIZE / 4),
			(i / 2) * (BLOCK_SIZE / 2) + (BLOCK_SIZE / 4)
		);

		// より自然な破片の飛び散り方を計算
		double baseVelX = (i % 2 == 0) ? -1.0 : 1.0;  // 左右方向
		double baseVelY = (i / 2 == 0) ? -1.5 : -0.8; // 上下方向（上の破片がより高く飛ぶ）

		// ランダム要素を追加
		double randomFactorX = Random(-0.5, 0.5);
		double randomFactorY = Random(0.0, 0.5);

		Vec2 velocity(
			baseVelX * (150.0 + randomFactorX * 50.0),
			baseVelY * 200.0 - randomFactorY * 100.0
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

	// 上向きに移動している場合のみ判定
	if (playerVel.y >= 0) return false;

	// X軸の重なり判定
	const double PLAYER_WIDTH = 80.0;
	const double COLLISION_MARGIN = 8.0;

	double playerLeft = playerPos.x - PLAYER_WIDTH / 2 + COLLISION_MARGIN;
	double playerRight = playerPos.x + PLAYER_WIDTH / 2 - COLLISION_MARGIN;
	double blockLeft = block.position.x;
	double blockRight = block.position.x + BLOCK_SIZE;

	bool xOverlap = (playerRight > blockLeft && playerLeft < blockRight);
	if (!xOverlap) return false;

	// Y軸の位置関係判定
	const double PLAYER_HEIGHT = 100.0;
	const double HIT_TOLERANCE = 32.0;

	double playerHead = playerPos.y - PLAYER_HEIGHT / 2;
	double blockBottom = block.position.y + BLOCK_SIZE;

	// 現在および次フレームでの衝突をチェック
	double nextPlayerHead = playerHead + playerVel.y * Scene::DeltaTime();

	double currentDistance = std::abs(playerHead - blockBottom);
	double nextDistance = std::abs(nextPlayerHead - blockBottom);

	bool isHittingFromBelow = false;

	if ((currentDistance <= HIT_TOLERANCE && playerHead <= blockBottom + 5.0) ||
		(nextDistance <= HIT_TOLERANCE && nextPlayerHead <= blockBottom + 5.0))
	{
		isHittingFromBelow = true;
	}

	return isHittingFromBelow;
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

	// コインブロックの配置（ステージ全体に分散）
	Array<Vec2> coinBlocks = {
		// 前半エリア (0-1280)
		Vec2(320, 480),   // 初期エリア上空
		Vec2(640, 420),   // 中初期エリア
		Vec2(960, 460),   // 中間エリア
		Vec2(1280, 400),  // 前半終了

		// 中間エリア (1280-2560)
		Vec2(1600, 440),  // 中間開始
		Vec2(1920, 380),  // 中間1
		Vec2(2240, 420),  // 中間2
		Vec2(2560, 360),  // 中間終了

		// 後半エリア (2560-3840)
		Vec2(2880, 400),  // 後半開始
		Vec2(3200, 340),  // 後半1
		Vec2(3520, 380),  // 後半2
		Vec2(3840, 320),  // 後半終了

		// 終盤エリア (3840-5120)
		Vec2(4160, 360),  // 終盤開始
		Vec2(4480, 300),  // 終盤1
		Vec2(4800, 340),  // 終盤2
		Vec2(5120, 280)   // ゴール手前
	};

	// レンガブロックの配置（障害物として機能）
	Array<Vec2> brickBlocks = {
		// 前半エリア (0-1280)
		Vec2(480, 500),   // 初期障害物
		Vec2(800, 440),   // 中初期障害物
		Vec2(1120, 480),  // 前半障害物

		// 中間エリア (1280-2560)
		Vec2(1440, 460),  // 中間障害物1
		Vec2(1760, 400),  // 中間障害物2
		Vec2(2080, 440),  // 中間障害物3
		Vec2(2400, 380),  // 中間障害物4

		// 後半エリア (2560-3840)
		Vec2(2720, 420),  // 後半障害物1
		Vec2(3040, 360),  // 後半障害物2
		Vec2(3360, 400),  // 後半障害物3
		Vec2(3680, 340),  // 後半障害物4

		// 終盤エリア (3840-5120)
		Vec2(4000, 380),  // 終盤障害物1
		Vec2(4320, 320),  // 終盤障害物2
		Vec2(4640, 360),  // 終盤障害物3
		Vec2(4960, 300)   // 最終障害物
	};

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

	// 砂漠ステージの地形を避けた配置
	Array<Vec2> coinBlocks = {
		// 前半エリア (0-1280)
		Vec2(384, 440),   // オアシス近辺
		Vec2(768, 380),   // 砂丘の間隔
		Vec2(1152, 420),  // 前半砂丘

		// 中間エリア (1280-2560)
		Vec2(1536, 360),  // 中間砂丘1
		Vec2(1920, 400),  // 中間砂丘2
		Vec2(2304, 340),  // 中間砂丘3
		Vec2(2688, 380),  // ピラミッド横

		// 後半エリア (2560-3840)
		Vec2(3072, 320),  // 後半砂丘1
		Vec2(3456, 360),  // 後半砂丘2
		Vec2(3840, 300),  // 後半砂丘3

		// 終盤エリア (3840-5120)
		Vec2(4224, 340),  // 砂の谷上空
		Vec2(4608, 280),  // 終盤砂丘1
		Vec2(4992, 320),  // 終盤砂丘2
		Vec2(5376, 260)   // 最終砂漠
	};

	Array<Vec2> brickBlocks = {
		// 前半エリア (0-1280)
		Vec2(512, 460),   // 砂丘障害1
		Vec2(896, 400),   // 砂丘障害2
		Vec2(1280, 440),  // 前半砂丘障害

		// 中間エリア (1280-2560)
		Vec2(1664, 380),  // 中間砂丘障害1
		Vec2(2048, 420),  // 中間砂丘障害2
		Vec2(2432, 360),  // ピラミッド近辺

		// 後半エリア (2560-3840)
		Vec2(2816, 400),  // 後半砂丘障害1
		Vec2(3200, 340),  // 後半砂丘障害2
		Vec2(3584, 380),  // 砂の道

		// 終盤エリア (3840-5120)
		Vec2(3968, 360),  // 終盤砂丘障害1
		Vec2(4352, 300),  // 終盤砂丘障害2
		Vec2(4736, 340),  // 砂丘間
		Vec2(5120, 280)   // 最終砂漠障害
	};

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

	// 魔法ステージの浮遊島を避けた魔法的配置
	Array<Vec2> coinBlocks = {
		// 前半エリア (0-1280)
		Vec2(448, 380),   // 魔法の島近辺
		Vec2(896, 320),   // 魔法の空上空
		Vec2(1344, 360),  // 前半魔法エリア

		// 中間エリア (1280-2560)
		Vec2(1792, 300),  // 中間魔法エリア1
		Vec2(2240, 340),  // 中間魔法エリア2
		Vec2(2688, 280),  // 中間魔法エリア3

		// 後半エリア (2560-3840)
		Vec2(3136, 320),  // 後半魔法エリア1
		Vec2(3584, 260),  // 高い魔法島上空
		Vec2(4032, 300),  // 後半魔法エリア2

		// 終盤エリア (3840-5120)
		Vec2(4480, 240),  // 魔法の道上空
		Vec2(4928, 280),  // 終盤魔法エリア1
		Vec2(5376, 220),  // 最終魔法エリア
		Vec2(5824, 260)   // ゴール前魔法空間
	};

	Array<Vec2> brickBlocks = {
		// 前半エリア (0-1280)
		Vec2(576, 400),   // 魔法障害1
		Vec2(1024, 340),  // 魔法障害2
		Vec2(1472, 380),  // 前半魔法障害

		// 中間エリア (1280-2560)
		Vec2(1920, 320),  // 中間魔法障害1
		Vec2(2368, 360),  // 中間魔法障害2
		Vec2(2816, 300),  // 魔法の城砦

		// 後半エリア (2560-3840)
		Vec2(3264, 340),  // 後半魔法障害1
		Vec2(3712, 280),  // 高度魔法障害
		Vec2(4160, 320),  // 後半魔法障害2

		// 終盤エリア (3840-5120)
		Vec2(4608, 260),  // 魔法の道障害
		Vec2(5056, 300),  // 終盤魔法障害
		Vec2(5504, 240),  // 最終魔法城砦
		Vec2(5952, 280)   // 最終魔法障害
	};

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

	// 雪山の地形を避けた配置
	Array<Vec2> coinBlocks = {
		// 前半エリア (0-1280)
		Vec2(384, 420),   // 山麓エリア
		Vec2(768, 360),   // 雪の足場近辺
		Vec2(1152, 400),  // 前半雪原

		// 中間エリア (1280-2560)
		Vec2(1536, 340),  // 中間雪原1
		Vec2(1920, 380),  // 中間雪原2
		Vec2(2304, 320),  // 氷の足場上空
		Vec2(2688, 360),  // 山頂エリア

		// 後半エリア (2560-3840)
		Vec2(3072, 300),  // 後半山頂1
		Vec2(3456, 340),  // 後半山頂2
		Vec2(3840, 280),  // 氷河エリア

		// 終盤エリア (3840-5120)
		Vec2(4224, 320),  // 終盤氷河1
		Vec2(4608, 260),  // 雪原エリア
		Vec2(4992, 300),  // 終盤雪原
		Vec2(5376, 240)   // 最終山頂
	};

	Array<Vec2> brickBlocks = {
		// 前半エリア (0-1280)
		Vec2(512, 440),   // 雪山障害1
		Vec2(896, 380),   // 雪山障害2
		Vec2(1280, 420),  // 前半氷の道

		// 中間エリア (1280-2560)
		Vec2(1664, 360),  // 中間氷の道1
		Vec2(2048, 400),  // 中間氷の道2
		Vec2(2432, 340),  // 氷の道

		// 後半エリア (2560-3840)
		Vec2(2816, 380),  // 後半氷の道1
		Vec2(3200, 320),  // 雪だまり近辺
		Vec2(3584, 360),  // 後半氷の道2

		// 終盤エリア (3840-5120)
		Vec2(3968, 300),  // 終盤雪山1
		Vec2(4352, 340),  // 雪原障害
		Vec2(4736, 280),  // 終盤雪山2
		Vec2(5120, 320)   // 最終雪原障害
	};

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

	// 石ステージの地形を避けた配置
	Array<Vec2> coinBlocks = {
		// 前半エリア (0-1280)
		Vec2(320, 460),   // 石の間隔
		Vec2(640, 400),   // 岩場の上空
		Vec2(960, 440),   // 前半岩場
		Vec2(1280, 380),  // 前半石の道

		// 中間エリア (1280-2560)
		Vec2(1600, 420),  // 中間岩場1
		Vec2(1920, 360),  // 中間岩場2
		Vec2(2240, 400),  // 石の道の上
		Vec2(2560, 340),  // 中間石の道

		// 後半エリア (2560-3840)
		Vec2(2880, 380),  // 後半岩場1
		Vec2(3200, 320),  // 高い岩場の上
		Vec2(3520, 360),  // 後半岩場2
		Vec2(3840, 300),  // 石の谷の上

		// 終盤エリア (3840-5120)
		Vec2(4160, 340),  // 終盤岩場1
		Vec2(4480, 280),  // 終盤の岩場
		Vec2(4800, 320),  // 終盤岩場2
		Vec2(5120, 260)   // 最終岩場
	};

	Array<Vec2> brickBlocks = {
		// 前半エリア (0-1280)
		Vec2(448, 480),   // 初期岩場
		Vec2(768, 420),   // 前半岩場障害
		Vec2(1088, 460),  // 前半石の道障害

		// 中間エリア (1280-2560)
		Vec2(1408, 440),  // 中間岩場障害1
		Vec2(1728, 380),  // 中間岩場障害2
		Vec2(2048, 420),  // 中間岩場
		Vec2(2368, 360),  // 石の道障害

		// 後半エリア (2560-3840)
		Vec2(2688, 400),  // 後半岩場障害1
		Vec2(3008, 340),  // 高岩場
		Vec2(3328, 380),  // 後半岩場障害2
		Vec2(3648, 320),  // 石の道障害

		// 終盤エリア (3840-5120)
		Vec2(3968, 360),  // 終盤岩場障害1
		Vec2(4288, 300),  // 終盤岩場
		Vec2(4608, 340),  // 終盤岩場障害2
		Vec2(4928, 280)   // 最終岩場障害
	};

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

	// 地下ステージの洞窟を避けた配置
	Array<Vec2> coinBlocks = {
		// 前半エリア (0-1280)
		Vec2(384, 440),   // 洞窟入口
		Vec2(768, 380),   // 地下通路間隔
		Vec2(1152, 420),  // 前半地下通路

		// 中間エリア (1280-2560)
		Vec2(1536, 360),  // 中間地下通路1
		Vec2(1920, 400),  // 中間地下通路2
		Vec2(2304, 340),  // 深い洞窟
		Vec2(2688, 380),  // 中間深洞窟

		// 後半エリア (2560-3840)
		Vec2(3072, 320),  // 後半深洞窟1
		Vec2(3456, 360),  // 地下の隠し部屋
		Vec2(3840, 300),  // 後半深洞窟2

		// 終盤エリア (3840-5120)
		Vec2(4224, 340),  // 終盤隠し部屋1
		Vec2(4608, 280),  // 洞窟の奥
		Vec2(4992, 320),  // 終盤隠し部屋2
		Vec2(5376, 260)   // 地下の宝
	};

	Array<Vec2> brickBlocks = {
		// 前半エリア (0-1280)
		Vec2(512, 460),   // 洞窟障害1
		Vec2(896, 400),   // 地下通路障害
		Vec2(1280, 440),  // 前半洞窟障害

		// 中間エリア (1280-2560)
		Vec2(1664, 380),  // 中間地下通路障害1
		Vec2(2048, 420),  // 中間地下通路障害2
		Vec2(2432, 360),  // 地下通路障害

		// 後半エリア (2560-3840)
		Vec2(2816, 400),  // 後半地下通路障害1
		Vec2(3200, 340),  // 深洞窟障害
		Vec2(3584, 380),  // 後半地下通路障害2

		// 終盤エリア (3840-5120)
		Vec2(3968, 320),  // 終盤洞窟障害1
		Vec2(4352, 360),  // 隠し部屋障害
		Vec2(4736, 300),  // 洞窟奥障害
		Vec2(5120, 340)   // 最終地下障害
	};

	for (const auto& pos : coinBlocks) {
		addCoinBlock(pos);
	}

	for (const auto& pos : brickBlocks) {
		addBrickBlock(pos);
	}
}
