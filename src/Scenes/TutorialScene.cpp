#include "TutorialScene.hpp"
#include "../Core/SceneManagers.hpp"

namespace {
	const bool registered = [] {
		SceneFactory::registerScene(SceneType::Tutorial, [] {
			return std::make_unique<TutorialScene>();
		});
		return true;
		}();
}

void TutorialScene::init()
{
	Print << U"TutorialScene::init()";
}

void TutorialScene::update()
{
	if (KeyEnter.down())
	{
		requestSceneChange(SceneType::Title);
	}
}

void TutorialScene::draw() const
{
	Scene::SetBackground(ColorF(0.1, 0.1, 0.2));
	FontAsset(U"Title")(U"チュートリアルシーン").drawAt(Scene::CenterF());
	FontAsset(U"Menu")(U"Enterキーでタイトルへ戻る").drawAt(Scene::CenterF() + Vec2{ 0, 50 });
}

Optional<SceneType> TutorialScene::getNextScene() const
{
	return m_requestedSceneChange;
}
