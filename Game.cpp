#include "Game.hpp"

Game::Game()
	: m_application(nullptr)
{
}

Game::~Game()
{
	shutdown();
}

bool Game::init()
{
	// アプリケーションの作成と初期化
	m_application = std::make_unique<Application>();
	return m_application->init();
}

void Game::run()
{
	if (!m_application)
	{
		return;
	}

	m_application->run();
}

void Game::shutdown()
{
	if (m_application)
	{
		m_application->shutdown();
		m_application.reset();
	}
}
