#pragma once
#include <memory>
#include "SceneBase.hpp"
#include "SceneType.hpp"

class SceneFactory
{
public:
	static std::unique_ptr<SceneBase> create(SceneType type);
};
