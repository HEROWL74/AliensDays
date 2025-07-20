#pragma once
#include <Siv3D.hpp>
#include <memory>
#include "SceneBase.hpp"

class SceneManagers
{
private:
	enum class FadeState
	{
		None,
		FadeOut,
		FadeIn
	};

	double m_fadeTimer = 0.0;
	FadeState m_fadeState = FadeState::None;
	SceneType m_nextSceneType;

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

	// 現在のシーンを取得
	SceneBase* getCurrentScene() const { return m_currentScene.get(); }


private:
	// シーンのファクトリーメソッド
	std::unique_ptr<SceneBase> createScene(SceneType sceneType);
};
