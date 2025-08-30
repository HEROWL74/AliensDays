#pragma once
#include "../Core/SceneBase.hpp"

class TutorialScene : public SceneBase
{
public:
	void init() override;
	void update() override;
	void draw() const override;

	Optional<SceneType> getNextScene() const override;
};
