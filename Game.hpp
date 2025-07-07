#pragma once
#include <Siv3D.hpp>
#include <memory>
#include "Application.hpp"

class Game
{
private:
	std::unique_ptr<Application> m_application;

public:
	Game();
	~Game();

	// ゲームの初期化
	bool init();

	// ゲームの実行
	void run();

	// ゲームの終了
	void shutdown();
};
