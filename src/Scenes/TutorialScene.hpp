#pragma once
#include "../Core/SceneBase.hpp"
#include "../Systems/TutorialEvents.hpp"
#include "../UI/TutorialPanel.hpp"
#include "../Enemies/EnemyFactory.hpp"
#include "GameScene.hpp"

class TutorialScene : public GameScene
{
public:
	TutorialScene() : GameScene(StageNumber::Tutorial) {}

	void init() override;
	void update() override;
	void draw() const override;
	void cleanup() override; // ★ 追加

	Optional<SceneType> getNextScene() const override;

private:
	enum class Step { Move, Jump, Stomp, Fireball, Done };

	Step m_step = Step::Move;
	int m_subId = 0;
	TutorialPanel m_panel;

	Optional<SceneType> m_nextScene = none; // ★ 修正: メンバー変数として追加

	void goTo(Step s);
	void spawnFor(Step s);
};
