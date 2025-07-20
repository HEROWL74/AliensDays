#pragma once
#include <Siv3D.hpp>
#include <memory>

// 前方宣言
class Player;
enum class StageNumber;

class BlockSystem {
public:
	// ブロックの種類
	enum class BlockType {
		COIN_BLOCK,     // コインブロック
		BRICK_BLOCK     // レンガブロック
	};

	// ブロックの状態
	enum class BlockState {
		ACTIVE,         // アクティブ状態
		EMPTY,          // 空状態（コインブロック用）
		DESTROYED       // 破壊された状態（レンガブロック用）
	};

	// 破片の構造体
	struct BlockFragment {
		Vec2 position;              // 位置
		Vec2 velocity;              // 速度
		double rotation;            // 回転角度
		double rotationSpeed;       // 回転速度
		double life;                // 生存期間
		double maxLife;             // 最大生存期間
		Texture texture;            // テクスチャ
		bool bounced;               // 地面にバウンスしたかのフラグ

		BlockFragment(const Vec2& pos, const Vec2& vel, const Texture& tex)
			: position(pos), velocity(vel), texture(tex)
			, rotation(0.0), rotationSpeed(Random(-5.0, 5.0))
			, life(FRAGMENT_LIFE), maxLife(FRAGMENT_LIFE), bounced(false) {
		}
	};

	// 個別のブロック
	struct Block {
		Vec2 position;                  // 位置
		BlockType type;                 // ブロックの種類
		BlockState state;               // ブロックの状態
		Texture texture;                // 現在のテクスチャ
		double bounceAnimation;         // バウンスアニメーション用
		double bounceTimer;             // バウンスタイマー
		bool wasHit;                    // ヒットされたかのフラグ

		Block(const Vec2& pos, BlockType blockType)
			: position(pos), type(blockType), state(BlockState::ACTIVE)
			, bounceAnimation(0.0), bounceTimer(0.0), wasHit(false) {
		}
	};

	BlockSystem();
	~BlockSystem() = default;

	void init();
	void update(Player* player);
	void draw(const Vec2& cameraOffset) const;

	// ブロック管理
	void addCoinBlock(const Vec2& position);
	void addBrickBlock(const Vec2& position);
	void clearAllBlocks();

	// 新しい統一衝突判定システム用メソッド
	Array<RectF> getCollisionRects() const;
	bool hasBlockAt(const Vec2& position) const;
	void handlePlayerInteraction(Player* player);

	// プレイヤーとの当たり判定（レガシー - 段階的に削除予定）
	bool checkCollision(const RectF& playerRect) const;
	void handlePlayerCollision(Player* player);

	// ヒット判定関連
	bool checkPlayerHitFromBelow(const Block& block, Player* player) const;

	// ステージ別のブロック配置
	void generateBlocksForStage(StageNumber stageNumber);

	// 獲得したコイン数を取得
	int getCoinsFromBlocks() const { return m_coinsFromBlocks; }
	void resetCoinCount() { m_coinsFromBlocks = 0; }

	// デバッグ用
	int getTotalBlockCount() const { return static_cast<int>(m_blocks.size()); }
	int getActiveBlockCount() const;

private:
	// テクスチャ
	Texture m_coinBlockActiveTexture;   // アクティブなコインブロック
	Texture m_coinBlockEmptyTexture;    // 空のコインブロック
	Texture m_brickBlockTexture;        // レンガブロック
	Array<Texture> m_brickFragmentTextures; // レンガの破片テクスチャ

	// ブロック管理
	Array<std::unique_ptr<Block>> m_blocks;
	Array<std::unique_ptr<BlockFragment>> m_fragments;
	int m_coinsFromBlocks;

	// 物理・演出定数
	static constexpr double BLOCK_SIZE = 64.0;          // ブロックサイズ
	static constexpr double HIT_DETECTION_SIZE = 80.0;  // 当たり判定サイズ
	static constexpr double BOUNCE_HEIGHT = 8.0;        // バウンス高さ
	static constexpr double BOUNCE_DURATION = 0.3;      // バウンス期間
	static constexpr double FRAGMENT_GRAVITY = 600.0;   // 破片の重力
	static constexpr double FRAGMENT_LIFE = 2.0;        // 破片の生存期間

	// ヘルパー関数
	void loadTextures();
	void createBrickFragmentTextures();

	// 新しい統一システム用の内部メソッド
	void updateBlockInteractions(Player* player);
	void updateBlockAnimation(Block& block);

	// 既存の内部メソッド（簡素化）
	void updateFragments();
	void handleCoinBlockHit(Block& block);
	void handleBrickBlockHit(Block& block);
	void createBrickFragments(const Vec2& blockPos);
	void drawBlock(const Block& block, const Vec2& cameraOffset) const;
	void drawFragments(const Vec2& cameraOffset) const;

	// 衝突判定ヘルパー（レガシー - 段階的に削除予定）
	bool checkAABBCollision(const RectF& rect1, const RectF& rect2) const;
	void resolveCollision(Player* player, const Block& block);
	bool isBlockSolid(const Block& block) const;
	RectF getBlockRect(const Block& block) const;

	// レガシー着地判定（削除予定）
	bool checkPlayerLandingOnBlocks(const RectF& playerRect) const;
	void handleBlockLanding(Player* player);
	double findNearestBlockTop(const Vec2& playerPos) const;

	// ステージ別生成メソッド
	void generateBlocksForGrassStage();
	void generateBlocksForSandStage();
	void generateBlocksForPurpleStage();
	void generateBlocksForSnowStage();
	void generateBlocksForStoneStage();
	void generateBlocksForDirtStage();
};
