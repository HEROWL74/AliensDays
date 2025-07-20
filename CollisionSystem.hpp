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

// 衝突情報
struct CollisionInfo
{
	bool hasCollision;
	CollisionType type;
	Vec2 contactPoint;
	Vec2 normal;
	double penetration;

	CollisionInfo()
		: hasCollision(false), type(CollisionType::None)
		, contactPoint(Vec2::Zero()), normal(Vec2::Zero()), penetration(0.0)
	{
	}

	CollisionInfo(CollisionType t, const Vec2& point, const Vec2& n, double p)
		: hasCollision(true), type(t), contactPoint(point), normal(n), penetration(p)
	{
	}
};

// 移動体の情報
struct MovingBody
{
	Vec2 position;
	Vec2 velocity;
	Vec2 size;          // 幅と高さ
	bool isGrounded;

	MovingBody(const Vec2& pos, const Vec2& vel, const Vec2& sz, bool grounded = false)
		: position(pos), velocity(vel), size(sz), isGrounded(grounded)
	{
	}

	RectF getBounds() const
	{
		return RectF(position.x - size.x / 2, position.y - size.y / 2, size.x, size.y);
	}

	RectF getNextFrameBounds(double deltaTime) const
	{
		Vec2 nextPos = position + velocity * deltaTime;
		return RectF(nextPos.x - size.x / 2, nextPos.y - size.y / 2, size.x, size.y);
	}
};

class CollisionSystem
{
public:
	CollisionSystem() = default;
	~CollisionSystem() = default;

	// メイン衝突判定メソッド
	void resolvePlayerCollisions(Player* player, Stage* stage, BlockSystem* blockSystem);

	// 個別衝突判定メソッド
	CollisionInfo checkAABBCollision(const RectF& rectA, const RectF& rectB) const;
	Array<CollisionInfo> checkCollisionsWithTerrain(const MovingBody& body, const Array<RectF>& terrainRects) const;

	// 衝突解決メソッド
	void resolveCollision(MovingBody& body, const CollisionInfo& collision) const;
	void resolveMultipleCollisions(MovingBody& body, const Array<CollisionInfo>& collisions) const;

	// ユーティリティメソッド
	bool isGrounded(const MovingBody& body, const Array<RectF>& terrainRects) const;
	Vec2 findNearestGroundPosition(const Vec2& position, const Array<RectF>& terrainRects) const;

private:
	// 内部ヘルパーメソッド
	CollisionInfo getCollisionInfo(const RectF& moving, const RectF& stationary) const;
	double calculatePenetration(const RectF& rectA, const RectF& rectB, CollisionType type) const;
	Vec2 calculateContactPoint(const RectF& rectA, const RectF& rectB) const;
	Vec2 calculateCollisionNormal(CollisionType type) const;

	CollisionInfo refineCollisionWithMovement(const CollisionInfo& collision, const Vec2& velocity, const RectF& prevRect, const RectF& terrainRect) const;

	CollisionInfo refineCollisionWithVelocity(const CollisionInfo& collision, const Vec2& velocity, const RectF& currentRect, const RectF& terrainRect) const;

	// 定数
	static constexpr double GROUND_TOLERANCE = 10.0;
	static constexpr double COLLISION_EPSILON = 0.1;
	static constexpr double SEPARATION_OFFSET = 1.0;
};
