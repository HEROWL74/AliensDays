//src/Core/SceneFactory.cpp
#include "SceneFactory.hpp"
#include "../Scenes/TitleScene.hpp"
#include "../Scenes/GameScene.hpp"
#include "../Scenes/CharacterSelectScene.hpp"
#include "../Scenes/OptionScene.hpp"
#include "../Scenes/CreditScene.hpp"
#include "../Scenes/GameOverScene.hpp"
#include "../Scenes/ResultScene.hpp"
#include "../Scenes/SplashScene.hpp"

std::unique_ptr<SceneBase> SceneFactory::create(SceneType type)
{
	switch (type)
	{
	case SceneType::Title:
		return std::make_unique<TitleScene>();
	case SceneType::Game:
		return std::make_unique<GameScene>();
	case SceneType::CharacterSelect:
		return std::make_unique<CharacterSelectScene>();
	case SceneType::Option:
		return std::make_unique<OptionScene>();
	case SceneType::Credit:
		return std::make_unique<CreditScene>();
	case SceneType::GameOver:
		return std::make_unique<GameOverScene>();
	case SceneType::Result:
		return std::make_unique<ResultScene>();
	case SceneType::Splash:
		return std::make_unique<SplashScene>();
	default:
		return nullptr;
	}
}
