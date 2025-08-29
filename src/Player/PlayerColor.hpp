#pragma once
#include <Siv3D.hpp>

// プレイヤーカラー列挙型
enum class PlayerColor
{
	Green,   // バランス型：標準的な能力
	Pink,    // スピード型：移動速度が速い、ジャンプ力は普通
	Purple,  // ジャンプ型：ジャンプ力が高い、移動速度は普通
	Beige,   // タフネス型：ライフが多い、移動とジャンプは少し遅い
	Yellow   // クイック型：素早い動作、低ライフ
};

// プレイヤー特性構造体
struct PlayerStats
{
	double moveSpeed;      // 移動速度倍率
	double jumpPower;      // ジャンプ力倍率
	int maxLife;          // 最大ライフ
	double invincibleTime; // 無敵時間倍率
	String description;   // キャラクターの説明
	String trait;         // 特性名
};

// プレイヤー色ごとの特性を取得
inline PlayerStats getPlayerStats(PlayerColor color)
{
	switch (color)
	{
	case PlayerColor::Green:
		return {
			1.0,    // moveSpeed: 標準
			1.0,    // jumpPower: 標準
			6,      // maxLife: 3ハート (標準)
			1.0,    // invincibleTime: 標準
			U"バランスの取れた標準キャラクター",
			U"バランス型"
		};

	case PlayerColor::Pink:
		return {
			1.3,    // moveSpeed: 30%速い
			1.0,    // jumpPower: 標準
			4,      // maxLife: 2ハート (少ない)
			0.8,    // invincibleTime: 少し短い
			U"素早い移動で敵を翻弄する",
			U"スピード型"
		};

	case PlayerColor::Purple:
		return {
			0.9,    // moveSpeed: 少し遅い
			1.4,    // jumpPower: 40%高い
			6,      // maxLife: 3ハート (標準)
			1.0,    // invincibleTime: 標準
			U"高いジャンプで難所を突破する",
			U"ジャンプ型"
		};

	case PlayerColor::Beige:
		return {
			0.8,    // moveSpeed: 20%遅い
			0.9,    // jumpPower: 少し低い
			8,      // maxLife: 4ハート (多い)
			1.3,    // invincibleTime: 30%長い
			U"高い耐久力で安全にゴールを目指す",
			U"タフネス型"
		};

	case PlayerColor::Yellow:
		return {
			1.2,    // moveSpeed: 20%速い
			1.1,    // jumpPower: 少し高い
			2,      // maxLife: 1ハート (最少)
			0.7,    // invincibleTime: 短い
			U"軽やかな動きだが打たれ弱い",
			U"クイック型"
		};

	default:
		return getPlayerStats(PlayerColor::Green);
	}
}

// プレイヤー色の名前を取得
inline String getPlayerColorName(PlayerColor color)
{
	switch (color)
	{
	case PlayerColor::Green:  return U"Green";
	case PlayerColor::Pink:   return U"Pink";
	case PlayerColor::Purple: return U"Purple";
	case PlayerColor::Beige:  return U"Beige";
	case PlayerColor::Yellow: return U"Yellow";
	default: return U"Unknown";
	}
}

// プレイヤー色のテーマカラーを取得
inline ColorF getPlayerThemeColor(PlayerColor color)
{
	switch (color)
	{
	case PlayerColor::Green:  return ColorF(0.3, 1.0, 0.3);
	case PlayerColor::Pink:   return ColorF(1.0, 0.5, 0.8);
	case PlayerColor::Purple: return ColorF(0.8, 0.3, 1.0);
	case PlayerColor::Beige:  return ColorF(0.9, 0.8, 0.6);
	case PlayerColor::Yellow: return ColorF(1.0, 1.0, 0.3);
	default: return ColorF(1.0, 1.0, 1.0);
	}
}
