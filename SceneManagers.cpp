#include "SceneManagers.hpp"
#include "SplashScene.hpp"           // スプラッシュシーンのインクルードを追加
#include "TitleScene.hpp"
#include "CharacterSelectScene.hpp"  // キャラクターセレクトシーンのインクルードを追加
#include "OptionScene.hpp"           // オプションシーンのインクルードを追加
#include "CreditScene.hpp"           // クレジットシーンのインクルードを追加
#include "GameScene.hpp"
#include "GameOverScene.hpp"         // ゲームオーバーシーンのインクルード追加
#include "ResultScene.hpp"           // リザルトシーンのインクルード追加

SceneManagers::SceneManagers()
	: m_currentScene(nullptr)
	, m_currentSceneType(SceneType::Splash)  // 初期シーンをスプラッシュに変更
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

	if (m_fadeState == FadeState::None)
	{
		m_currentScene->update();

		if (auto next = m_currentScene->getNextScene())
		{
			m_nextSceneType = next.value();
			m_fadeState = FadeState::FadeOut;
			m_fadeTimer = 0.0;
		}
	}
	else if (m_fadeState == FadeState::FadeOut)
	{
		m_fadeTimer += Scene::DeltaTime();
		if (m_fadeTimer >= 1.0)
		{
			changeScene(m_nextSceneType);
			m_fadeState = FadeState::FadeIn;
			m_fadeTimer = 0.0;
		}
	}
	else if (m_fadeState == FadeState::FadeIn)
	{
		m_fadeTimer += Scene::DeltaTime();
		if (m_fadeTimer >= 1.0)
		{
			m_fadeState = FadeState::None;
			m_fadeTimer = 0.0;
		}
	}
}

void SceneManagers::draw() const
{
	if (m_currentScene)
	{
		m_currentScene->draw();
	}

	if (m_fadeState != FadeState::None)
	{
		double alpha = (m_fadeState == FadeState::FadeOut)
			? m_fadeTimer / 1.0
			: 1.0 - m_fadeTimer / 1.0;

		Rect(Scene::Size()).draw(ColorF(0.0, alpha));
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
	case SceneType::Splash:            // スプラッシュシーンのケースを追加
		return std::make_unique<SplashScene>();
	case SceneType::Title:
		return std::make_unique<TitleScene>();
	case SceneType::CharacterSelect:   // キャラクターセレクトシーンのケースを追加
		return std::make_unique<CharacterSelectScene>();
	case SceneType::Option:            // オプションシーンのケースを追加
		return std::make_unique<OptionScene>();
	case SceneType::Credit:            // クレジットシーンのケースを追加
		return std::make_unique<CreditScene>();
	case SceneType::Game:
		return std::make_unique<GameScene>();
	case SceneType::GameOver:          // ゲームオーバーシーンのケース追加
		return std::make_unique<GameOverScene>();
	case SceneType::Result:            // リザルトシーンのケース追加
		return std::make_unique<ResultScene>();
	default:
		return nullptr;
	}
}
