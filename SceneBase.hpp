#pragma once
#include <Siv3D.hpp>

// シーンの種類
enum class SceneType
{
	Title,
	Game
};

// シーンの基底クラス
class SceneBase
{
public:
	virtual ~SceneBase() = default;

	// シーンの初期化
	virtual void init() = 0;

	// シーンの更新
	virtual void update() = 0;

	// シーンの描画
	virtual void draw() const = 0;

	// 次のシーンを取得
	virtual Optional<SceneType> getNextScene() const = 0;

	// シーンのクリーンアップ
	virtual void cleanup() {}
};
