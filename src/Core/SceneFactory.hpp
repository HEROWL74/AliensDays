#pragma once
#include <memory>
#include "SceneBase.hpp"
#include <functional>
#include <unordered_map>

class SceneFactory
{
public:
	using CreatorFunc = std::function<std::unique_ptr<SceneBase>()>;

	static void registerScene(SceneType type, CreatorFunc creator);
	static std::unique_ptr<SceneBase> create(SceneType type);

private:
	static std::unordered_map<SceneType, CreatorFunc>& getRegistry();
};
