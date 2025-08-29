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

std::unordered_map<SceneType, SceneFactory::CreatorFunc>& SceneFactory::getRegistry()
{
	static std::unordered_map<SceneType, CreatorFunc> registry;
	return registry;
}

void SceneFactory::registerScene(SceneType type, CreatorFunc creator)
{
	getRegistry()[type] = std::move(creator);
}

std::unique_ptr<SceneBase> SceneFactory::create(SceneType type)
{
	auto& registry = getRegistry();
	if (auto it = registry.find(type); it != registry.end())
	{
		return it->second(); //登録したらインスタンス生成
	}
	throw std::runtime_error("Unknown SceneType");
}
