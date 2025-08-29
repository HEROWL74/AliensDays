#pragma once
#include <Siv3D.hpp>

// 前方宣言
class Player;
class Stage;
class BlockSystem;

// 衝突タイプ
enum class CollisionType
{
	None,
	Ground,     // 地面への着地
	Ceiling,    // 天井への衝突
	LeftWall,   // 左の壁への衝突
	RightWall   // 右の壁への衝突
};

class CollisionSystem
{
public:
	// 衝突結果構造体
	struct CollisionResult
	{
		bool hasCollision;
		CollisionType collisionType;
		Vec2 correctedPosition;
		Vec2 correctedVelocity;

		CollisionResult()
			: hasCollision(false), collisionType(CollisionType::None)
			, correctedPosition(Vec2::Zero()), correctedVelocity(Vec2::Zero())
		{
		}
	};

	CollisionSystem() = default;
	~CollisionSystem() = default;

	// ★ メイン衝突判定メソッド（64x64ブロック基準）
	void resolvePlayerCollisions(Player* player, Stage* stage, BlockSystem* blockSystem);

	// ★ 1ブロック基準の衝突解決
	CollisionResult resolveBlockCollision(const Vec2& currentPos, const Vec2& nextPos,
										 const Vec2& velocity, const RectF& blockRect);

	// ★ 統合衝突矩形取得
	Array<RectF> getUnifiedCollisionRects(Stage* stage, BlockSystem* blockSystem) const;

	// ★ 精密な接地判定（1ブロック基準）
	bool checkPreciseGroundContact(const Vec2& playerPos, const Array<RectF>& terrainRects) const;

	// ★ プレイヤーが隙間に入れるかチェック
	bool canPlayerFitInGap(const Vec2& position, const Array<RectF>& terrainRects) const;

	// スライディング時の特別判定
	void checkSlidingBlockCollision(Player* player, const Vec2& playerPos, const Vec2& playerSize,
								   const Array<RectF>& blockRects, BlockSystem* blockSystem) const;

	// ユーティリティメソッド
	RectF getPlayerRect(const Vec2& position, const Vec2& size = Vec2::Zero()) const;
	Vec2 findNearestGroundPosition(const Vec2& position, const Array<RectF>& terrainRects) const;

	// レガシー互換メソッド（段階的に削除予定）
	bool checkImprovedGroundContact(const Vec2& playerPos, const Vec2& playerSize,
								   const Array<RectF>& terrainRects) const;
	bool checkGroundContact(const Vec2& playerPos, const Vec2& playerSize,
						   const Array<RectF>& terrainRects) const;

private:
	// 定数（64x64ブロック基準）
	static constexpr double COLLISION_EPSILON = 0.1;
	static constexpr double SEPARATION_OFFSET = 1.0;

#ifdef _DEBUG
	// デバッグ用メソッド
	void debugCollisionInfo(const Vec2& position, const Vec2& velocity,
						   bool grounded, size_t blockCount) const;
#endif
};
