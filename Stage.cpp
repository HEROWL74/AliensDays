#include "Stage.hpp"

// ステージ設定の静的配列
const Array<Stage::StageConfig> Stage::s_stageConfigs = {
	{ TerrainType::Grass, U"Green Plains", ColorF(0.4, 0.7, 0.9), ColorF(0.6, 0.8, 1.0), U"緑豊かな平原" },
	{ TerrainType::Sand, U"Desert Oasis", ColorF(0.9, 0.7, 0.4), ColorF(1.0, 0.9, 0.6), U"砂漠のオアシス" },
	{ TerrainType::Purple, U"Mystic Forest", ColorF(0.3, 0.2, 0.5), ColorF(0.5, 0.3, 0.7), U"神秘の森" },
	{ TerrainType::Snow, U"Frozen Peaks", ColorF(0.7, 0.8, 0.9), ColorF(0.8, 0.9, 1.0), U"凍てつく山頂" },
	{ TerrainType::Stone, U"Ancient Ruins", ColorF(0.4, 0.4, 0.5), ColorF(0.6, 0.6, 0.7), U"古代遺跡" },
	{ TerrainType::Dirt, U"Underground Cave", ColorF(0.2, 0.2, 0.3), ColorF(0.3, 0.3, 0.4), U"地下洞窟" }
};

Stage::Stage()
	: m_stageNumber(StageNumber::Stage1)
	, m_terrainType(TerrainType::Grass)
	, m_stageName(U"Default Stage")
	, m_backgroundColor(ColorF(0.4, 0.7, 0.9))
	, m_skyColor(ColorF(0.6, 0.8, 1.0))
	, m_cameraOffset(Vec2::Zero())
	, m_stagePixelWidth(STAGE_WIDTH* BLOCK_SIZE)
	, m_hasGoal(false)
	, m_goalAnimationTimer(0.0)
{
}

Stage::Stage(StageNumber stageNumber)
{
	init(stageNumber);
}

void Stage::init(StageNumber stageNumber)
{
	m_stageNumber = stageNumber;

	// ステージ設定を取得
	const int stageIndex = static_cast<int>(stageNumber) - 1;
	if (stageIndex >= 0 && stageIndex < static_cast<int>(s_stageConfigs.size()))
	{
		const auto& config = s_stageConfigs[stageIndex];
		m_terrainType = config.terrain;
		m_stageName = config.name;
		m_backgroundColor = config.backgroundColor;
		m_skyColor = config.skyColor;
	}

	// カメラとステージサイズの初期化
	m_cameraOffset = Vec2::Zero();
	m_stagePixelWidth = STAGE_WIDTH * BLOCK_SIZE;
	m_hasGoal = false;
	m_goalAnimationTimer = 0.0;

	// テクスチャ読み込み
	loadTerrainTextures();
	loadGoalTextures();

	// ステージレイアウト生成
	generateStageLayout();
}

void Stage::update(const Vec2& playerPosition)
{
	// カメラのスクロール処理
	const double screenCenterX = Scene::Width() / 2.0;
	const double targetCameraX = playerPosition.x - screenCenterX;

	// ステージの境界でクランプ
	const double maxCameraX = m_stagePixelWidth - Scene::Width();

	if (m_stagePixelWidth > Scene::Width())
	{
		m_cameraOffset.x = Math::Clamp(targetCameraX, 0.0, maxCameraX);
	}
	else
	{
		m_cameraOffset.x = 0.0;
	}

	m_cameraOffset.y = 0.0;

	// ゴールアニメーションタイマー更新
	m_goalAnimationTimer += Scene::DeltaTime();
}

void Stage::loadTerrainTextures()
{
	m_terrainTextures.clear();

	const String terrainStr = getTerrainString(m_terrainType);
	const String basePath = U"Sprites/Tiles/";

	// 各ブロックタイプのテクスチャを読み込み
	Array<std::pair<BlockType, String>> blockTypes = {
		{ BlockType::Bottom, U"bottom" },
		{ BlockType::BottomLeft, U"bottom_left" },
		{ BlockType::BottomRight, U"bottom_right" },
		{ BlockType::Center, U"center" },
		{ BlockType::Left, U"left" },
		{ BlockType::Right, U"right" },
		{ BlockType::Top, U"top" },
		{ BlockType::TopLeft, U"top_left" },
		{ BlockType::TopRight, U"top_right" }
	};

	for (const auto& [blockType, blockStr] : blockTypes)
	{
		const String filename = U"terrain_{}_block_{}.png"_fmt(terrainStr, blockStr);
		const String filepath = basePath + filename;
		const String key = buildTextureKey(m_terrainType, blockType);

		Texture texture(filepath);
		if (texture)
		{
			m_terrainTextures[key] = texture;
		}
		else
		{
			Print << U"Failed to load terrain texture: " << filepath;
		}
	}

	// 空中プラットフォーム用のシンプルブロックも読み込み
	const String simpleBlockFilename = U"terrain_{}_block.png"_fmt(terrainStr);
	const String simpleBlockFilepath = basePath + simpleBlockFilename;
	const String simpleBlockKey = U"{}_simple"_fmt(terrainStr);

	Texture simpleBlockTexture(simpleBlockFilepath);
	if (simpleBlockTexture)
	{
		m_terrainTextures[simpleBlockKey] = simpleBlockTexture;
	}
	else
	{
		Print << U"Failed to load simple block texture: " << simpleBlockFilepath;
	}
}

void Stage::loadGoalTextures()
{
	const String basePath = U"Sprites/Tiles/";

	m_goalFlagA = Texture(basePath + U"flag_blue_a.png");
	m_goalFlagB = Texture(basePath + U"flag_blue_b.png");

	if (!m_goalFlagA)
	{
		Print << U"Failed to load goal flag A texture";
	}
	if (!m_goalFlagB)
	{
		Print << U"Failed to load goal flag B texture";
	}
}

void Stage::addGoalFlag(const Vec2& position)
{
	m_goalPosition = position;
	m_hasGoal = true;

	// ゴールフラグ用のブロックを追加（非固体）
	StageBlock goalBlock;
	goalBlock.terrain = m_terrainType;
	goalBlock.blockType = BlockType::Empty;
	goalBlock.position = position;
	goalBlock.isSolid = false;
	goalBlock.isGoal = true;
	m_blocks.push_back(goalBlock);
}

bool Stage::checkGoalCollision(const RectF& playerRect) const
{
	if (!m_hasGoal) return false;

	// ゴールフラグの当たり判定矩形
	const RectF goalRect(m_goalPosition.x - 32, m_goalPosition.y - 32, 64, 64);

	return playerRect.intersects(goalRect);
}

void Stage::generateStageLayout()
{
	m_blocks.clear();

	// 全ステージ共通の統一レイアウトを使用
	// テクスチャのみがステージごとに変わる
	generateUnifiedStageLayout();
}

void Stage::generateUnifiedStageLayout()
{
	// ★ 修正: 地面をより適切な位置に配置
	// Scene::Height() = 600の場合、地面は下から4ブロック分（約256ピクセル）上に配置
	const int groundLevel = STAGE_HEIGHT - 4; // 約13 (17-4)、ピクセル座標では832

	// 1. メインの地面（画面下部）
	createGroundSection(0, groundLevel, STAGE_WIDTH, 4);

	// 2. 左側エリアの空中プラットフォーム
	createAirPlatform(8, groundLevel - 4, 4);   // 地面から4ブロック上
	createAirPlatform(5, groundLevel - 7, 3);   // 地面から7ブロック上

	// 3. 中央エリアの空中プラットフォーム群
	createAirPlatform(20, groundLevel - 3, 6);  // 地面から3ブロック上
	createAirPlatform(18, groundLevel - 6, 3);  // 地面から6ブロック上
	createAirPlatform(23, groundLevel - 9, 2);  // 地面から9ブロック上

	// 4. 中央-右側の段階的空中プラットフォーム
	createAirPlatform(35, groundLevel - 2, 4);  // 地面から2ブロック上
	createAirPlatform(42, groundLevel - 4, 3);  // 地面から4ブロック上
	createAirPlatform(48, groundLevel - 6, 2);  // 地面から6ブロック上

	// 5. 右側エリアの空中プラットフォーム群
	createAirPlatform(58, groundLevel - 3, 5);  // 地面から3ブロック上
	createAirPlatform(55, groundLevel - 7, 3);  // 地面から7ブロック上
	createAirPlatform(59, groundLevel - 10, 2); // 地面から10ブロック上

	// 6. 最終エリアの空中プラットフォーム
	createAirPlatform(70, groundLevel - 5, 4);  // 地面から5ブロック上
	createAirPlatform(68, groundLevel - 8, 2);  // 地面から8ブロック上

	// 7. ゴールフラグを地面上に配置
	const Vec2 goalPosition = Vec2((STAGE_WIDTH - 5) * BLOCK_SIZE, groundLevel * BLOCK_SIZE - 32);
	addGoalFlag(goalPosition);
}

// 新しいメソッド：空中プラットフォーム専用（terrain_○○_block.pngを使用）
void Stage::createAirPlatform(int startX, int y, int width)
{
	for (int i = 0; i < width; ++i)
	{
		StageBlock block;
		block.terrain = m_terrainType;
		block.blockType = BlockType::Simple; // 新しいBlockType
		block.position = gridToWorldPosition(startX + i, y);
		block.isSolid = true;
		block.isGoal = false;
		m_blocks.push_back(block);
	}
}

void Stage::draw() const
{
	drawBackground();
	drawBlocks();
	drawGoalFlag();
	drawCollisionDebug();
}

void Stage::drawBackground() const
{
	// 空のグラデーション
	Scene::Rect().draw(Arg::top = m_skyColor, Arg::bottom = m_backgroundColor);

	// ステージ名表示（右上）
	Font titleFont(24, Typeface::Bold);
	const String stageTitle = U"Stage {} - {}"_fmt(static_cast<int>(m_stageNumber), m_stageName);
	titleFont(stageTitle).draw(Arg::topRight(Scene::Width() - 10, 10), ColorF(1.0, 1.0, 1.0, 0.8));
}

void Stage::drawBlocks() const
{
	for (const auto& block : m_blocks)
	{
		drawBlock(block);
	}
}

void Stage::drawGoalFlag() const
{
	if (!m_hasGoal) return;

	const Vec2 screenPos = worldToScreenPosition(m_goalPosition);

	// 画面内にある場合のみ描画
	if (screenPos.x >= -64 && screenPos.x <= Scene::Width() + 64)
	{
		// アニメーション（flag_aとflag_bを交互に表示）
		const double animationSpeed = 0.5;
		const bool useVariantA = std::fmod(m_goalAnimationTimer, animationSpeed * 2) < animationSpeed;

		const Texture& currentTexture = useVariantA ? m_goalFlagA : m_goalFlagB;

		if (currentTexture)
		{
			// 軽く揺れるアニメーション
			const double sway = std::sin(m_goalAnimationTimer * 2.0) * 2.0;
			const Vec2 animatedPos = screenPos + Vec2(sway, 0);

			currentTexture.drawAt(animatedPos);

			// 光る効果
			const double glowAlpha = (std::sin(m_goalAnimationTimer * 4.0) + 1.0) * 0.3;
			Circle(animatedPos, 40).draw(ColorF(0.0, 0.5, 1.0, glowAlpha));
		}
		else
		{
			// フォールバック：青い旗型
			const Vec2 flagBase = screenPos;
			const Vec2 flagTop = flagBase + Vec2(0, -30);
			const Vec2 flagRight = flagBase + Vec2(20, -15);

			Triangle(flagTop, flagBase, flagRight).draw(ColorF(0.0, 0.5, 1.0));
			Line(flagBase, flagTop).draw(3.0, ColorF(0.8, 0.8, 0.8));
		}
	}
}

void Stage::drawBlock(const StageBlock& block) const
{
	if (block.blockType == BlockType::Empty) return;

	const Vec2 screenPos = worldToScreenPosition(block.position);

	// 画面外カリング（最適化）
	if (screenPos.x < -BLOCK_SIZE || screenPos.x > Scene::Width() + BLOCK_SIZE) return;

	// Simpleブロック（空中プラットフォーム）の特別処理
	if (block.blockType == BlockType::Simple)
	{
		const String simpleBlockKey = U"{}_simple"_fmt(getTerrainString(block.terrain));
		if (m_terrainTextures.contains(simpleBlockKey))
		{
			m_terrainTextures.at(simpleBlockKey).draw(screenPos);
			return;
		}
	}

	// 通常のブロック描画
	const Texture texture = getBlockTexture(block.terrain, block.blockType);
	if (texture)
	{
		texture.draw(screenPos);
	}
	else
	{
		// フォールバック：テクスチャがない場合は色付きの矩形
		const ColorF fallbackColor = [&]() {
			switch (block.terrain)
			{
			case TerrainType::Grass:  return ColorF(0.3, 0.7, 0.3);
			case TerrainType::Sand:   return ColorF(0.9, 0.8, 0.5);
			case TerrainType::Purple: return ColorF(0.6, 0.3, 0.8);
			case TerrainType::Snow:   return ColorF(0.9, 0.9, 1.0);
			case TerrainType::Stone:  return ColorF(0.5, 0.5, 0.6);
			case TerrainType::Dirt:   return ColorF(0.6, 0.4, 0.2);
			default: return ColorF(0.5, 0.5, 0.5);
			}
			}();

		RectF(screenPos, BLOCK_SIZE, BLOCK_SIZE).draw(fallbackColor);
	}
}

bool Stage::checkCollision(const RectF& rect) const
{
	// プレイヤーの矩形と各ブロックの矩形でintersects判定
	for (const auto& block : m_blocks)
	{
		if (block.isSolid && block.blockType != BlockType::Empty)
		{
			const RectF blockRect(block.position, BLOCK_SIZE, BLOCK_SIZE);
			if (rect.intersects(blockRect))
			{
				return true;
			}
		}
	}
	return false;
}

Array<RectF> Stage::getCollisionRects() const
{
	// 全ての固体ブロックの矩形を返す（Siv3D流の当たり判定用）
	Array<RectF> collisionRects;
	for (const auto& block : m_blocks)
	{
		if (block.isSolid && block.blockType != BlockType::Empty)
		{
			collisionRects.push_back(RectF(block.position, BLOCK_SIZE, BLOCK_SIZE));
		}
	}
	return collisionRects;
}

Vec2 Stage::worldToScreenPosition(const Vec2& worldPos) const
{
	return worldPos - m_cameraOffset;
}

Vec2 Stage::screenToWorldPosition(const Vec2& screenPos) const
{
	return screenPos + m_cameraOffset;
}

void Stage::drawCollisionDebug() const
{
	// デバッグ用：見えない当たり判定矩形を半透明で描画
#ifdef _DEBUG
	for (const auto& block : m_blocks)
	{
		if (block.isSolid && block.blockType != BlockType::Empty)
		{
			const Vec2 screenPos = worldToScreenPosition(block.position);

			// 画面内にあるもののみ描画
			if (screenPos.x >= -BLOCK_SIZE && screenPos.x <= Scene::Width())
			{
				const RectF blockRect(screenPos, BLOCK_SIZE, BLOCK_SIZE);
				blockRect.drawFrame(2.0, ColorF(1.0, 0.0, 0.0, 0.5));
			}
		}
	}

	// ★ 追加: デバッグ情報をテキストで表示
	Font debugFont(16);
	const String debugInfo = U"Ground Level: {} (pixel: {})"_fmt(
		STAGE_HEIGHT - 4,
		(STAGE_HEIGHT - 4) * BLOCK_SIZE
	);
	debugFont(debugInfo).draw(10, Scene::Height() - 60, ColorF(1.0, 1.0, 0.0));

	const String blockInfo = U"Total Blocks: {} | Block Size: {}px"_fmt(
		m_blocks.size(),
		BLOCK_SIZE
	);
	debugFont(blockInfo).draw(10, Scene::Height() - 40, ColorF(1.0, 1.0, 0.0));
#endif
}

bool Stage::isBlockSolid(int gridX, int gridY) const
{
	for (const auto& block : m_blocks)
	{
		const Point blockGrid = worldToGridPosition(block.position);
		if (blockGrid.x == gridX && blockGrid.y == gridY)
		{
			return block.isSolid;
		}
	}
	return false;
}

Vec2 Stage::getGroundPosition(double x) const
{
	const int gridX = static_cast<int>(x / BLOCK_SIZE);

	// 上から下に向かって最初に見つかるソリッドブロックの上面を返す
	for (int gridY = 0; gridY < STAGE_HEIGHT; ++gridY)
	{
		if (isBlockSolid(gridX, gridY))
		{
			return Vec2(x, gridY * BLOCK_SIZE);
		}
	}

	// ★ 修正: ソリッドブロックが見つからない場合は画面下端ではなく、想定される地面位置
	const int groundLevel = STAGE_HEIGHT - 4;
	return Vec2(x, groundLevel * BLOCK_SIZE);
}

Vec2 Stage::gridToWorldPosition(int gridX, int gridY) const
{
	return Vec2(gridX * BLOCK_SIZE, gridY * BLOCK_SIZE);
}

Point Stage::worldToGridPosition(const Vec2& worldPos) const
{
	return Point(
		static_cast<int>(worldPos.x / BLOCK_SIZE),
		static_cast<int>(worldPos.y / BLOCK_SIZE)
	);
}

String Stage::getTerrainString(TerrainType terrain) const
{
	switch (terrain)
	{
	case TerrainType::Dirt:   return U"dirt";
	case TerrainType::Grass:  return U"grass";
	case TerrainType::Purple: return U"purple";
	case TerrainType::Sand:   return U"sand";
	case TerrainType::Snow:   return U"snow";
	case TerrainType::Stone:  return U"stone";
	default: return U"grass";
	}
}

String Stage::getBlockTypeString(BlockType blockType) const
{
	switch (blockType)
	{
	case BlockType::Bottom:      return U"bottom";
	case BlockType::BottomLeft:  return U"bottom_left";
	case BlockType::BottomRight: return U"bottom_right";
	case BlockType::Center:      return U"center";
	case BlockType::Left:        return U"left";
	case BlockType::Right:       return U"right";
	case BlockType::Top:         return U"top";
	case BlockType::TopLeft:     return U"top_left";
	case BlockType::TopRight:    return U"top_right";
	case BlockType::Simple:      return U"block";
	default: return U"center";
	}
}

TerrainType Stage::getStageTerrainType(StageNumber stage)
{
	const int index = static_cast<int>(stage) - 1;
	if (index >= 0 && index < static_cast<int>(s_stageConfigs.size()))
	{
		return s_stageConfigs[index].terrain;
	}
	return TerrainType::Grass;
}

String Stage::getStageName(StageNumber stage)
{
	const int index = static_cast<int>(stage) - 1;
	if (index >= 0 && index < static_cast<int>(s_stageConfigs.size()))
	{
		return s_stageConfigs[index].name;
	}
	return U"Unknown Stage";
}

ColorF Stage::getStageBackgroundColor(StageNumber stage)
{
	const int index = static_cast<int>(stage) - 1;
	if (index >= 0 && index < static_cast<int>(s_stageConfigs.size()))
	{
		return s_stageConfigs[index].backgroundColor;
	}
	return ColorF(0.4, 0.7, 0.9);
}

// ===== 新しいタイル生成システム =====

void Stage::setBlock(int gridX, int gridY, BlockType blockType, bool isSolid)
{
	if (gridX >= 0 && gridX < STAGE_WIDTH && gridY >= 0 && gridY < STAGE_HEIGHT)
	{
		StageBlock block;
		block.terrain = m_terrainType;
		block.blockType = blockType;
		block.position = gridToWorldPosition(gridX, gridY);
		block.isSolid = isSolid;
		block.isGoal = false;
		m_blocks.push_back(block);
	}
}

void Stage::createGroundSection(int startX, int startY, int width, int height)
{
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const int currentX = startX + x;
			const int currentY = startY + y;

			BlockType blockType = determineGroundBlockType(x, y, width, height);
			setBlock(currentX, currentY, blockType);
		}
	}
}

BlockType Stage::determineGroundBlockType(int localX, int localY, int totalWidth, int totalHeight) const
{
	// 一番上の行（top系）
	if (localY == 0)
	{
		if (localX == 0) return BlockType::TopLeft;
		else if (localX == totalWidth - 1) return BlockType::TopRight;
		else return BlockType::Top;
	}
	// 一番下の行（bottom系）
	else if (localY == totalHeight - 1)
	{
		if (localX == 0) return BlockType::BottomLeft;
		else if (localX == totalWidth - 1) return BlockType::BottomRight;
		else return BlockType::Bottom;
	}
	// 中間の行（center系）
	else
	{
		if (localX == 0) return BlockType::Left;
		else if (localX == totalWidth - 1) return BlockType::Right;
		else return BlockType::Center;
	}
}

void Stage::createSinglePlatform(int x, int y)
{
	// 単独プラットフォームの場合は特別処理
	setBlock(x, y, BlockType::Top);
}

void Stage::createHorizontalPlatform(int startX, int y, int width)
{
	for (int i = 0; i < width; ++i)
	{
		BlockType blockType;
		if (width == 1)
		{
			// 単独ブロック
			blockType = BlockType::Top;
		}
		else if (i == 0)
		{
			// 左端
			blockType = BlockType::TopLeft;
		}
		else if (i == width - 1)
		{
			// 右端
			blockType = BlockType::TopRight;
		}
		else
		{
			// 中央
			blockType = BlockType::Top;
		}

		setBlock(startX + i, y, blockType);
	}
}

String Stage::buildTextureKey(TerrainType terrain, BlockType blockType) const
{
	return U"{}_{}"_fmt(getTerrainString(terrain), getBlockTypeString(blockType));
}

Texture Stage::getBlockTexture(TerrainType terrain, BlockType blockType) const
{
	const String key = buildTextureKey(terrain, blockType);
	if (m_terrainTextures.contains(key))
	{
		return m_terrainTextures.at(key);
	}
	return Texture{};
}
