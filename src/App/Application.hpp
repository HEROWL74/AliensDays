#pragma once
#include <Siv3D.hpp>
#include <memory>
#include "../Core/SceneManagers.hpp"

class Application
{
private:
	std::unique_ptr<SceneManagers> m_sceneManager;
	bool m_isRunning;

public:
	Application();
	~Application();

	// アプリケーションの初期化
	bool init();

	// アプリケーションの実行
	void run();

	// アプリケーションの終了
	void shutdown();

	// 実行中かどうか
	bool isRunning() const;

private:
	// フレーム処理
	void update();
	void draw();
};
