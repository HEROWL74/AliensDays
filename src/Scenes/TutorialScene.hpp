#pragma once
#include "../Core/SceneBase.hpp"
#include "../Systems/TutorialEvents.hpp"
#include "../UI/TutorialPanel.hpp"
#include "../Enemies/EnemyFactory.hpp"
#include "../Systems/GamepadSystem.hpp"
#include "GameScene.hpp"

class TutorialScene : public GameScene
{
public:
	TutorialScene() : GameScene(StageNumber::Tutorial) {}

	void init() override;
	void update() override;
	void draw() const override;
	void cleanup() override;

	Optional<SceneType> getNextScene() const override;

private:
	enum class Step { Move, Jump, Stomp, Fireball, Done };

	Step m_step = Step::Move;
	int m_subId = 0;
	TutorialPanel m_panel;

	Optional<SceneType> m_nextScene = none;

	void goTo(Step s);
	void spawnFor(Step s);
};
