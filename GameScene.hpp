#pragma once
#include <Siv3D.hpp>
#include "SceneBase.hpp"

class GameScene : public SceneBase
{
private:
	Texture m_backgroundTexture;
	Font m_gameFont;
	Optional<SceneType> m_nextScene;

	// ゲームの状態
	Vec2 m_playerPos;
	double m_gameTime;

public:
	GameScene();
	~GameScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;
};
