#pragma once
#include <Siv3D.hpp>

// 地形の種類
enum class TerrainType
{
	Dirt,      // 土
	Grass,     // 草原
	Purple,    // 紫（魔法の森など）
	Sand,      // 砂漠
	Snow,      // 雪原
	Stone      // 石（toneファイル名に対応）
};

// ブロックの種類
enum class BlockType
{
	Bottom,
	BottomLeft,
	BottomRight,
	Center,
	Left,
	Right,
	Top,
	TopLeft,
	TopRight,
	Simple,
	Empty  // 何もない空間
};

// ステージ番号
enum class StageNumber
{
	Tutorial = 0,
	Stage1,
	Stage2,
	Stage3,
	Stage4,
	Stage5,
	Stage6
};

// ステージブロック情報
struct StageBlock
{
	TerrainType terrain;
	BlockType blockType;
	Vec2 position;
	bool isSolid;  // 衝突判定があるかどうか
	bool isGoal;   // ゴールフラグかどうか

	StageBlock() : terrain(TerrainType::Grass), blockType(BlockType::Empty),
		position(Vec2::Zero()), isSolid(false), isGoal(false) {
	}
};

class Stage
{
private:
	// ステージ情報
	StageNumber m_stageNumber;
	TerrainType m_terrainType;
	String m_stageName;
	ColorF m_backgroundColor;
	ColorF m_skyColor;

	// ブロック関連
	Array<StageBlock> m_blocks;
	HashTable<String, Texture> m_terrainTextures;
	static constexpr int BLOCK_SIZE = 64;  // ブロック1つのサイズ
	static constexpr int STAGE_WIDTH = 80;  // ステージの幅（ブロック数）
	static constexpr int STAGE_HEIGHT = 17; // ステージの高さ（ブロック数）

	// カメラ・スクロール関連
	Vec2 m_cameraOffset;
	double m_stagePixelWidth;  // ステージの実際の幅（ピクセル）

	// ステージ設定
	struct StageConfig
	{
		TerrainType terrain;
		String name;
		ColorF backgroundColor;
		ColorF skyColor;
		String description;
	};

	static const Array<StageConfig> s_stageConfigs;

	// ゴール関連
	Vec2 m_goalPosition;
	bool m_hasGoal;
	Texture m_goalFlagA;
	Texture m_goalFlagB;
	double m_goalAnimationTimer;

public:
	Stage();
	Stage(StageNumber stageNumber);
	~Stage() = default;

	// 初期化・終了
	void init(StageNumber stageNumber);
	void loadTerrainTextures();
	void generateStageLayout();

	void createAirPlatform(int startX, int y, int width);

	// 更新・描画
	void update(const Vec2& playerPosition);  // カメラ更新用
	void draw() const;
	void drawBackground() const;
	void drawBlocks() const;
	void drawBlock(const StageBlock& block) const;

	// ステージ情報取得
	StageNumber getStageNumber() const { return m_stageNumber; }
	TerrainType getTerrainType() const { return m_terrainType; }
	String getStageName() const { return m_stageName; }
	ColorF getBackgroundColor() const { return m_backgroundColor; }
	Vec2 getCameraOffset() const { return m_cameraOffset; }

	// 衝突判定
	bool checkCollision(const RectF& rect) const;
	bool isWithinStageBounds(const Vec2& position) const;
	bool isBlockSolid(int gridX, int gridY) const;
	Vec2 getGroundPosition(double x) const;  // 指定X座標での地面位置を取得
	Array<RectF> getCollisionRects() const;
	Vec2 screenToWorldPosition(const Vec2& screenPos) const;
	Vec2 worldToScreenPosition(const Vec2& worldPos) const;
	void drawCollisionDebug() const;         // デバッグ用衝突判定描画

	bool canPlayerFitAt(const Vec2& position) const;

	bool isGridPositionFree(int gridX, int gridY) const;

	bool hasOneBlockGap(int gridX, int gridY) const;

	int getGroundLevel() const;

	// ユーティリティ
	Vec2 gridToWorldPosition(int gridX, int gridY) const;
	Point worldToGridPosition(const Vec2& worldPos) const;
	String getTerrainString(TerrainType terrain) const;
	String getBlockTypeString(BlockType blockType) const;

	// 静的メソッド
	static TerrainType getStageTerrainType(StageNumber stage);
	static String getStageName(StageNumber stage);
	static ColorF getStageBackgroundColor(StageNumber stage);

	// ゴール関連メソッド
	void addGoalFlag(const Vec2& position);
	bool checkGoalCollision(const RectF& playerRect) const;
	Vec2 getGoalPosition() const { return m_goalPosition; }
	bool hasGoal() const { return m_hasGoal; }

private:
	// ステージ別レイアウト生成メソッド
	void generateGrassStageLayout();    // ステージ1: 草原
	void generateSandStageLayout();     // ステージ2: 砂漠
	void generatePurpleStageLayout();   // ステージ3: 魔法の森
	void generateSnowStageLayout();     // ステージ4: 雪山
	void generateStoneStageLayout();    // ステージ5: 古代遺跡
	void generateDirtStageLayout();     // ステージ6: 地下洞窟

	// ブロック配置ヘルパー（新システム）
	void setBlock(int gridX, int gridY, BlockType blockType, bool isSolid = true);
	void createGroundSection(int startX, int startY, int width, int height);
	void createSinglePlatform(int x, int y);
	void createHorizontalPlatform(int startX, int y, int width);
	BlockType determineGroundBlockType(int localX, int localY, int totalWidth, int totalHeight) const;

	// テクスチャ関連
	String buildTextureKey(TerrainType terrain, BlockType blockType) const;
	Texture getBlockTexture(TerrainType terrain, BlockType blockType) const;

	// ゴール描画
	void drawGoalFlag() const;
	void loadGoalTextures();
};
