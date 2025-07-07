#include "SceneManagers.hpp"
#include "TitleScene.hpp"
#include "GameScene.hpp"

SceneManagers::SceneManagers()
	: m_currentScene(nullptr)
	, m_currentSceneType(SceneType::Title)
{
}

SceneManagers::~SceneManagers()
{
	if (m_currentScene)
	{
		m_currentScene->cleanup();
	}
}

void SceneManagers::init(SceneType initialScene)
{
	m_currentSceneType = initialScene;
	m_currentScene = createScene(initialScene);
	if (m_currentScene)
	{
		m_currentScene->init();
	}
}

void SceneManagers::update()
{
	if (!m_currentScene)
		return;

	m_currentScene->update();

	// シーンの切り替えをチェック
	if (auto nextScene = m_currentScene->getNextScene())
	{
		changeScene(nextScene.value());
	}
}

void SceneManagers::draw() const
{
	if (m_currentScene)
	{
		m_currentScene->draw();
	}
}

void SceneManagers::changeScene(SceneType newScene)
{
	if (m_currentScene)
	{
		m_currentScene->cleanup();
	}

	m_currentSceneType = newScene;
	m_currentScene = createScene(newScene);

	if (m_currentScene)
	{
		m_currentScene->init();
	}
}

SceneType SceneManagers::getCurrentSceneType() const
{
	return m_currentSceneType;
}

std::unique_ptr<SceneBase> SceneManagers::createScene(SceneType sceneType)
{
	switch (sceneType)
	{
	case SceneType::Title:
		return std::make_unique<TitleScene>();
	case SceneType::Game:
		return std::make_unique<GameScene>();
	default:
		return nullptr;
	}
}
