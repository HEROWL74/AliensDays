#include "Application.hpp"

Application::Application()
	: m_sceneManager(nullptr)
	, m_isRunning(false)
{
}

Application::~Application()
{
	shutdown();
}

bool Application::init()
{
	// ウィンドウの設定
	Window::SetTitle(U"Siv3D Game");
	Window::Resize(1920, 1080);

	// シーンマネージャーの初期化
	m_sceneManager = std::make_unique<SceneManagers>();
	m_sceneManager->init(SceneType::Title);

	m_isRunning = true;
	return true;
}

void Application::run()
{
	if (!m_isRunning)
	{
		return;
	}

	while (System::Update())
	{
		update();
		draw();

		// ウィンドウが閉じられたら終了
		if (!m_isRunning)
		{
			break;
		}
	}
}

void Application::shutdown()
{
	m_isRunning = false;
	m_sceneManager.reset();
}

bool Application::isRunning() const
{
	return m_isRunning;
}

void Application::update()
{
	if (!m_sceneManager)
	{
		return;
	}

	m_sceneManager->update();
}

void Application::draw()
{
	if (!m_sceneManager)
	{
		return;
	}

	m_sceneManager->draw();
}
