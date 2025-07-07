#pragma once
#include <Siv3D.hpp>
#include <memory>
#include "SceneBase.hpp"

class SceneManagers
{
private:
	std::unique_ptr<SceneBase> m_currentScene;
	SceneType m_currentSceneType;

public:
	SceneManagers();
	~SceneManagers();

	// シーンの初期化
	void init(SceneType initialScene);

	// シーンの更新
	void update();

	// シーンの描画
	void draw() const;

	// シーンの切り替え
	void changeScene(SceneType newScene);

	// 現在のシーンタイプを取得
	SceneType getCurrentSceneType() const;

private:
	// シーンのファクトリーメソッド
	std::unique_ptr<SceneBase> createScene(SceneType sceneType);
};
