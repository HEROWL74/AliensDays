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
	// ★ カメラのスクロール処理を1ブロック基準で調整
	const double screenCenterX = Scene::Width() / 2.0;
	const double targetCameraX = playerPosition.x - screenCenterX;

	// ステージの境界でクランプ（1ブロック基準）
	const double maxCameraX = m_stagePixelWidth - Scene::Width();

	if (m_stagePixelWidth > Scene::Width())
	{
		m_cameraOffset.x = Math::Clamp(targetCameraX, 0.0, maxCameraX);
	}
	else
	{
		m_cameraOffset.x = 0.0;
	}

	// Y軸のカメラ追従は無効（横スクロールのみ）
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

	// ★ ゴールフラグの当たり判定を1ブロック基準で設定
	const RectF goalRect(
		m_goalPosition.x - BLOCK_SIZE / 2,
		m_goalPosition.y - BLOCK_SIZE / 2,
		BLOCK_SIZE,
		BLOCK_SIZE
	);

	return playerRect.intersects(goalRect);
}
void Stage::generateStageLayout()
{
	m_blocks.clear();

	// ステージ番号に応じて個別のレイアウトを生成
	switch (m_stageNumber)
	{
	case StageNumber::Stage1:
		generateGrassStageLayout();
		break;
	case StageNumber::Stage2:
		generateSandStageLayout();
		break;
	case StageNumber::Stage3:
		generatePurpleStageLayout();
		break;
	case StageNumber::Stage4:
		generateSnowStageLayout();
		break;
	case StageNumber::Stage5:
		generateStoneStageLayout();
		break;
	case StageNumber::Stage6:
		generateDirtStageLayout();
		break;
	default:
		generateGrassStageLayout(); // フォールバック
		break;
	}
}

void Stage::generateGrassStageLayout()
{
	// Stage1: 草原ステージ - 横スクロールアクションの基本を学ぶ
	const int groundLevel = 13; // Y座標832 (13 * 64)

	// 1. メインの地面（画面下部）
	createGroundSection(0, groundLevel, STAGE_WIDTH, 4);

	// 2. 空中プラットフォーム配置
	// ブロックシステムとの重複を完全回避、ジャンプアクションを楽しめる配置

	// === チュートリアルエリア (0-1000px) ===
	// 基本的なジャンプを学ぶ
	createAirPlatform(5, 11, 3);   // X: 320, Y: 704 - 低い安全な足場
	createAirPlatform(10, 10, 2);  // X: 640, Y: 640 - 少し高い

	// === 第1エリア (1000-2000px) - 段階的な上昇 ===
	createAirPlatform(16, 11, 2);  // X: 1024, Y: 704
	createAirPlatform(20, 9, 3);   // X: 1280, Y: 576 - ジャンプ必要
	createAirPlatform(25, 8, 2);   // X: 1600, Y: 512
	createAirPlatform(29, 7, 2);   // X: 1856, Y: 448 - 高い位置

	// === 第2エリア (2000-3000px) - 谷間と山 ===
	createAirPlatform(34, 10, 3);  // X: 2176, Y: 640 - 低い位置に戻る
	createAirPlatform(39, 6, 2);   // X: 2496, Y: 384 - 大ジャンプ必要
	createAirPlatform(43, 8, 2);   // X: 2752, Y: 512 - 中間高度
	createAirPlatform(47, 5, 1);   // X: 3008, Y: 320 - 狭い高所

	// === 第3エリア (3000-4000px) - リズミカルな配置 ===
	createAirPlatform(51, 9, 2);   // X: 3264, Y: 576
	createAirPlatform(55, 7, 2);   // X: 3520, Y: 448
	createAirPlatform(58, 10, 3);  // X: 3712, Y: 640 - 安全地帯
	createAirPlatform(62, 6, 2);   // X: 3968, Y: 384

	// === 最終エリア (4000-5000px) - 総合チャレンジ ===
	createAirPlatform(66, 8, 2);   // X: 4224, Y: 512
	createAirPlatform(69, 4, 1);   // X: 4416, Y: 256 - 最高点
	createAirPlatform(72, 7, 2);   // X: 4608, Y: 448
	createAirPlatform(75, 10, 3);  // X: 4800, Y: 640 - ゴール前安全地帯

	// ゴールフラグ（最終プラットフォーム上）
	const Vec2 goalPosition = Vec2(77 * BLOCK_SIZE, 10 * BLOCK_SIZE - 32);
	addGoalFlag(goalPosition);
}

void Stage::generateSandStageLayout()
{
	// Stage2: 砂漠ステージ - 砂丘の起伏と長距離ジャンプ
	const int groundLevel = 13;
	createGroundSection(0, groundLevel, STAGE_WIDTH, 4);

	// === オアシスエリア (0-1000px) ===
	createAirPlatform(4, 11, 4);   // X: 256, Y: 704 - 大きな砂丘
	createAirPlatform(11, 10, 2);  // X: 704, Y: 640

	// === 砂丘エリア (1000-2000px) - 起伏のある地形 ===
	createAirPlatform(17, 12, 3);  // X: 1088, Y: 768 - 地面近く
	createAirPlatform(22, 9, 2);   // X: 1408, Y: 576
	createAirPlatform(26, 7, 2);   // X: 1664, Y: 448
	createAirPlatform(30, 8, 3);   // X: 1920, Y: 512

	// === ピラミッドエリア (2000-3000px) - 階段状の配置 ===
	createAirPlatform(35, 11, 2);  // X: 2240, Y: 704 - 基底
	createAirPlatform(38, 9, 2);   // X: 2432, Y: 576
	createAirPlatform(41, 7, 2);   // X: 2624, Y: 448
	createAirPlatform(44, 5, 2);   // X: 2816, Y: 320 - 頂上
	createAirPlatform(47, 6, 2);   // X: 3008, Y: 384 - 下降開始

	// === 流砂エリア (3000-4000px) - 不規則な配置 ===
	createAirPlatform(52, 10, 2);  // X: 3328, Y: 640
	createAirPlatform(56, 8, 1);   // X: 3584, Y: 512 - 狭い足場
	createAirPlatform(59, 11, 2);  // X: 3776, Y: 704
	createAirPlatform(63, 6, 2);   // X: 4032, Y: 384

	// === 砂嵐エリア (4000-5000px) - 視界不良を想定した配置 ===
	createAirPlatform(67, 9, 2);   // X: 4288, Y: 576
	createAirPlatform(70, 7, 1);   // X: 4480, Y: 448 - 小さい足場
	createAirPlatform(73, 10, 2);  // X: 4672, Y: 640
	createAirPlatform(76, 8, 3);   // X: 4864, Y: 512 - ゴール前

	const Vec2 goalPosition = Vec2(79 * BLOCK_SIZE, 8 * BLOCK_SIZE - 32);
	addGoalFlag(goalPosition);
}

void Stage::generatePurpleStageLayout()
{
	// Stage3: 魔法の森 - 浮遊する魔法の足場、垂直移動重視
	const int groundLevel = 13;
	createGroundSection(0, groundLevel, STAGE_WIDTH, 4);

	// === 森の入口 (0-1000px) ===
	createAirPlatform(3, 10, 2);   // X: 192, Y: 640
	createAirPlatform(7, 8, 2);    // X: 448, Y: 512
	createAirPlatform(11, 6, 2);   // X: 704, Y: 384 - 上昇開始

	// === 魔法の木 (1000-2000px) - 螺旋上昇 ===
	createAirPlatform(15, 11, 2);  // X: 960, Y: 704
	createAirPlatform(19, 8, 2);   // X: 1216, Y: 512
	createAirPlatform(23, 5, 2);   // X: 1472, Y: 320 - 高い
	createAirPlatform(27, 7, 2);   // X: 1728, Y: 448
	createAirPlatform(31, 4, 1);   // X: 1984, Y: 256 - 最高点

	// === 浮遊島エリア (2000-3000px) - 離れた足場 ===
	createAirPlatform(36, 9, 2);   // X: 2304, Y: 576
	createAirPlatform(40, 6, 1);   // X: 2560, Y: 384 - 小さい
	createAirPlatform(44, 8, 2);   // X: 2816, Y: 512
	createAirPlatform(48, 3, 2);   // X: 3072, Y: 192 - 非常に高い

	// === 古代遺跡エリア (3000-4000px) - 複雑な配置 ===
	createAirPlatform(53, 10, 3);  // X: 3392, Y: 640
	createAirPlatform(57, 7, 2);   // X: 3648, Y: 448
	createAirPlatform(60, 5, 1);   // X: 3840, Y: 320
	createAirPlatform(63, 9, 2);   // X: 4032, Y: 576

	// === 魔法の城 (4000-5000px) - 城壁を模した配置 ===
	createAirPlatform(67, 11, 2);  // X: 4288, Y: 704 - 城門
	createAirPlatform(70, 8, 2);   // X: 4480, Y: 512
	createAirPlatform(73, 6, 2);   // X: 4672, Y: 384 - 城壁上
	createAirPlatform(76, 9, 3);   // X: 4864, Y: 576 - 玉座の間

	const Vec2 goalPosition = Vec2(79 * BLOCK_SIZE, 9 * BLOCK_SIZE - 32);
	addGoalFlag(goalPosition);
}

void Stage::generateSnowStageLayout()
{
	// Stage4: 雪山ステージ - 滑りやすい足場と急峻な地形
	const int groundLevel = 13;
	createGroundSection(0, groundLevel, STAGE_WIDTH, 4);

	// === 山麓 (0-1000px) - 緩やかな上り ===
	createAirPlatform(6, 11, 3);   // X: 384, Y: 704
	createAirPlatform(11, 10, 2);  // X: 704, Y: 640
	createAirPlatform(15, 9, 2);   // X: 960, Y: 576

	// === 氷河エリア (1000-2000px) - 滑る足場 ===
	createAirPlatform(19, 11, 4);  // X: 1216, Y: 704 - 長い氷
	createAirPlatform(25, 8, 2);   // X: 1600, Y: 512
	createAirPlatform(29, 6, 1);   // X: 1856, Y: 384 - 小さい氷

	// === 登山道 (2000-3000px) - ジグザグ上昇 ===
	createAirPlatform(33, 10, 2);  // X: 2112, Y: 640
	createAirPlatform(37, 7, 2);   // X: 2368, Y: 448
	createAirPlatform(41, 5, 2);   // X: 2624, Y: 320
	createAirPlatform(45, 3, 1);   // X: 2880, Y: 192 - 頂上付近

	// === 吹雪エリア (3000-4000px) - 不規則で危険 ===
	createAirPlatform(49, 8, 2);   // X: 3136, Y: 512
	createAirPlatform(53, 6, 1);   // X: 3392, Y: 384
	createAirPlatform(56, 10, 2);  // X: 3584, Y: 640 - 急降下
	createAirPlatform(60, 4, 2);   // X: 3840, Y: 256 - 再上昇

	// === 山頂 (4000-5000px) - 最終チャレンジ ===
	createAirPlatform(64, 7, 2);   // X: 4096, Y: 448
	createAirPlatform(68, 5, 1);   // X: 4352, Y: 320 - 狭い
	createAirPlatform(71, 9, 2);   // X: 4544, Y: 576
	createAirPlatform(74, 3, 2);   // X: 4736, Y: 192 - 最高点
	createAirPlatform(77, 8, 3);   // X: 4928, Y: 512 - ゴール前

	const Vec2 goalPosition = Vec2(80 * BLOCK_SIZE, 8 * BLOCK_SIZE - 32);
	addGoalFlag(goalPosition);
}

void Stage::generateStoneStageLayout()
{
	// Stage5: 古代遺跡 - 石造建築物と精密なジャンプ
	const int groundLevel = 13;
	createGroundSection(0, groundLevel, STAGE_WIDTH, 4);

	// === 遺跡入口 (0-1000px) - 崩れた石段 ===
	createAirPlatform(5, 11, 2);   // X: 320, Y: 704
	createAirPlatform(9, 9, 3);    // X: 576, Y: 576
	createAirPlatform(14, 7, 2);   // X: 896, Y: 448

	// === 大広間 (1000-2000px) - 柱の配置 ===
	createAirPlatform(18, 10, 1);  // X: 1152, Y: 640 - 柱1
	createAirPlatform(21, 8, 1);   // X: 1344, Y: 512 - 柱2
	createAirPlatform(24, 6, 1);   // X: 1536, Y: 384 - 柱3
	createAirPlatform(27, 4, 1);   // X: 1728, Y: 256 - 柱4
	createAirPlatform(30, 5, 2);   // X: 1920, Y: 320 - 横梁

	// === 王の間 (2000-3000px) - 対称的な配置 ===
	createAirPlatform(34, 9, 2);   // X: 2176, Y: 576
	createAirPlatform(38, 6, 3);   // X: 2432, Y: 384 - 玉座
	createAirPlatform(42, 9, 2);   // X: 2688, Y: 576
	createAirPlatform(46, 7, 2);   // X: 2944, Y: 448

	// === 宝物庫への道 (3000-4000px) - 罠を意識 ===
	createAirPlatform(50, 11, 1);  // X: 3200, Y: 704 - 落とし穴後
	createAirPlatform(53, 8, 2);   // X: 3392, Y: 512
	createAirPlatform(57, 5, 1);   // X: 3648, Y: 320 - 狭い
	createAirPlatform(60, 10, 2);  // X: 3840, Y: 640

	// === 宝物庫 (4000-5000px) - 複雑な最終エリア ===
	createAirPlatform(64, 7, 2);   // X: 4096, Y: 448
	createAirPlatform(67, 4, 1);   // X: 4288, Y: 256 - 宝の台座
	createAirPlatform(70, 6, 2);   // X: 4480, Y: 384
	createAirPlatform(73, 9, 2);   // X: 4672, Y: 576
	createAirPlatform(76, 5, 1);   // X: 4864, Y: 320
	createAirPlatform(78, 8, 3);   // X: 4992, Y: 512 - 脱出口

	const Vec2 goalPosition = Vec2(81 * BLOCK_SIZE, 8 * BLOCK_SIZE - 32);
	addGoalFlag(goalPosition);
}

void Stage::generateDirtStageLayout()
{
	// Stage6: 地下洞窟（最終ステージ） - 複雑な洞窟構造と総合チャレンジ
	const int groundLevel = 13;
	createGroundSection(0, groundLevel, STAGE_WIDTH, 4);

	// === 洞窟入口 (0-1000px) - 下降する地形 ===
	createAirPlatform(4, 10, 3);   // X: 256, Y: 640
	createAirPlatform(9, 11, 2);   // X: 576, Y: 704
	createAirPlatform(13, 12, 2);  // X: 832, Y: 768 - 低い

	// === 鍾乳洞 (1000-2000px) - 上下の足場 ===
	createAirPlatform(17, 9, 2);   // X: 1088, Y: 576
	createAirPlatform(20, 5, 2);   // X: 1280, Y: 320 - 天井近く
	createAirPlatform(23, 11, 3);  // X: 1472, Y: 704 - 床近く
	createAirPlatform(27, 7, 2);   // X: 1728, Y: 448
	createAirPlatform(30, 3, 1);   // X: 1920, Y: 192 - 最上部

	// === 地底湖 (2000-3000px) - 離れた足場 ===
	createAirPlatform(35, 10, 1);  // X: 2240, Y: 640 - 飛び石1
	createAirPlatform(38, 9, 1);   // X: 2432, Y: 576 - 飛び石2
	createAirPlatform(41, 10, 1);  // X: 2624, Y: 640 - 飛び石3
	createAirPlatform(44, 8, 2);   // X: 2816, Y: 512 - 中州
	createAirPlatform(48, 11, 2);  // X: 3072, Y: 704 - 対岸

	// === 溶岩洞 (3000-4000px) - 危険エリア ===
	createAirPlatform(52, 9, 1);   // X: 3328, Y: 576 - 小さい
	createAirPlatform(55, 6, 1);   // X: 3520, Y: 384
	createAirPlatform(58, 8, 1);   // X: 3712, Y: 512
	createAirPlatform(61, 4, 2);   // X: 3904, Y: 256 - 安全地帯

	// === 最深部 (4000-5000px) - 最終試練 ===
	createAirPlatform(65, 10, 2);  // X: 4160, Y: 640
	createAirPlatform(68, 7, 1);   // X: 4352, Y: 448 - 狭い
	createAirPlatform(70, 5, 1);   // X: 4480, Y: 320
	createAirPlatform(72, 3, 1);   // X: 4608, Y: 192 - 最高難度
	createAirPlatform(74, 6, 2);   // X: 4736, Y: 384
	createAirPlatform(77, 9, 3);  // X: 4928, Y: 576 - 出口への道

	// 最終ゴール（洞窟の出口）
	const Vec2 goalPosition = Vec2(80 * BLOCK_SIZE, 9 * BLOCK_SIZE - 32);
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
	// ★ 1ブロック基準での正確な衝突判定
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
	// ★ 全ての固体ブロックの矩形を64x64基準で返す
	Array<RectF> collisionRects;
	collisionRects.reserve(m_blocks.size()); // パフォーマンス向上

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
	// ★ カメラオフセットを考慮した正確な座標変換
	return worldPos - m_cameraOffset;
}

Vec2 Stage::screenToWorldPosition(const Vec2& screenPos) const
{
	// ★ スクリーン座標からワールド座標への変換
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

				// ★ グリッド座標とワールド座標を表示
				const Point gridPos = worldToGridPosition(block.position);
				Font(12)(U"({},{})"_fmt(gridPos.x, gridPos.y))
					.draw(screenPos + Vec2(4, 4), ColorF(1.0, 1.0, 0.0));
			}
		}
	}

	// ★ デバッグ情報をテキストで表示（64x64基準）
	Font debugFont(16);
	const String debugInfo = U"Block Size: {}px | Grid: {}x{} | Ground Level: Block {}"_fmt(
		BLOCK_SIZE,
		STAGE_WIDTH,
		STAGE_HEIGHT,
		STAGE_HEIGHT - 4
	);
	debugFont(debugInfo).draw(10, Scene::Height() - 60, ColorF(1.0, 1.0, 0.0));

	const String blockInfo = U"Total Blocks: {} | Stage Width: {}px"_fmt(
		m_blocks.size(),
		m_stagePixelWidth
	);
	debugFont(blockInfo).draw(10, Scene::Height() - 40, ColorF(1.0, 1.0, 0.0));

	// ★ プレイヤー位置に対するグリッド座標表示
	if (m_cameraOffset.x > 0)
	{
		const double playerWorldX = m_cameraOffset.x + Scene::Width() / 2.0;
		const int playerGridX = static_cast<int>(playerWorldX / BLOCK_SIZE);
		const String playerGridInfo = U"Player Grid X: {} (World X: {:.0f})"_fmt(
			playerGridX, playerWorldX
		);
		debugFont(playerGridInfo).draw(10, Scene::Height() - 20, ColorF(0.8, 1.0, 0.8));
	}
#endif
}

// ★ 新しいメソッド: 指定位置にプレイヤーが入れるかチェック
bool Stage::canPlayerFitAt(const Vec2& position) const
{
	const double halfSize = BLOCK_SIZE / 2.0;
	const RectF playerRect(
		position.x - halfSize,
		position.y - halfSize,
		BLOCK_SIZE,
		BLOCK_SIZE
	);

	return !checkCollision(playerRect);
}

// 指定グリッド座標が空いているかチェック
bool Stage::isGridPositionFree(int gridX, int gridY) const
{
	return !isBlockSolid(gridX, gridY);
}

// プレイヤーサイズでの隙間チェック
bool Stage::hasOneBlockGap(int gridX, int gridY) const
{
	// 指定位置とその上の1ブロックが空いているかチェック
	return isGridPositionFree(gridX, gridY) && isGridPositionFree(gridX, gridY - 1);
}

//地面レベルの取得（グリッド座標）
int Stage::getGroundLevel() const
{
	return STAGE_HEIGHT - 4; // 13ブロック目（0ベースで12）
}

//ステージの境界チェック
bool Stage::isWithinStageBounds(const Vec2& position) const
{
	return position.x >= 0 && position.x <= m_stagePixelWidth &&
		position.y >= 0 && position.y <= STAGE_HEIGHT * BLOCK_SIZE;
}

bool Stage::isBlockSolid(int gridX, int gridY) const{

	// ★ グリッド座標での正確なブロック判定
	if (gridX < 0 || gridX >= STAGE_WIDTH || gridY < 0 || gridY >= STAGE_HEIGHT)
	{
		return false; // 範囲外は非固体
	}

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

	// ★ 1ブロック基準での地面検索
	// 上から下に向かって最初に見つかるソリッドブロックの上面を返す
	for (int gridY = 0; gridY < STAGE_HEIGHT; ++gridY)
	{
		if (isBlockSolid(gridX, gridY))
		{
			// ブロックの上面の位置を返す
			return Vec2(x, gridY * BLOCK_SIZE);
		}
	}

	// ソリッドブロックが見つからない場合は基本地面位置
	const int defaultGroundLevel = STAGE_HEIGHT - 4; // 13ブロック目
	return Vec2(x, defaultGroundLevel * BLOCK_SIZE);
}

Vec2 Stage::gridToWorldPosition(int gridX, int gridY) const
{
	return Vec2(gridX * BLOCK_SIZE, gridY * BLOCK_SIZE);
}

Point Stage::worldToGridPosition(const Vec2& worldPos) const
{
	// ★ ワールド座標からグリッド座標への変換（64x64基準）
	return Point(
		static_cast<int>(Math::Floor(worldPos.x / BLOCK_SIZE)),
		static_cast<int>(Math::Floor(worldPos.y / BLOCK_SIZE))
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
