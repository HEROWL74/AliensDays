#pragma once
#include <Siv3D.hpp>
#include "SceneBase.hpp"

class TitleScene : public SceneBase
{
private:
	Texture m_backgroundTexture;
	Font m_titleFont;
	Font m_messageFont;
	Optional<SceneType> m_nextScene;

public:
	TitleScene();
	~TitleScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;
};
