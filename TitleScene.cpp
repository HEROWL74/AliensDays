#include "TitleScene.hpp"

TitleScene::TitleScene()
	: m_nextScene(none)
{
}

void TitleScene::init()
{
	// 背景画像の読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");

	// フォントの初期化
	m_titleFont = Font(48, Typeface::Bold);
	m_messageFont = Font(24);

	// 次のシーンをリセット
	m_nextScene = none;
}

void TitleScene::update()
{
	// スペースキーまたはエンターキーでゲームシーンに移行
	if (KeySpace.down() || KeyEnter.down())
	{
		m_nextScene = SceneType::Game;
	}

	// マウスクリックでもゲームシーンに移行
	if (MouseL.down())
	{
		m_nextScene = SceneType::Game;
	}
}

void TitleScene::draw() const
{
	// 背景の描画
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw();
	}
	else
	{
		// 背景画像が読み込めない場合はグラデーション背景
		Scene::Rect().draw(Arg::top = ColorF(0.1, 0.2, 0.4), Arg::bottom = ColorF(0.4, 0.2, 0.1));
	}

	// 半透明オーバーレイ
	Scene::Rect().draw(ColorF(0, 0, 0, 0.3));

	// タイトルテキスト
	const String titleText = U"My Game";
	const Vec2 titlePos = Vec2(Scene::Center().x, Scene::Center().y - 50);
	m_titleFont(titleText).drawAt(titlePos, ColorF(1.0, 1.0, 1.0));

	// メッセージテキスト
	const String messageText = U"Press SPACE or ENTER to start";
	const Vec2 messagePos = Vec2(Scene::Center().x, Scene::Center().y + 50);
	m_messageFont(messageText).drawAt(messagePos, ColorF(0.8, 0.8, 0.8));

	// 点滅効果
	if (std::fmod(Scene::Time() * 2.0, 2.0) < 1.0)
	{
		const String clickText = U"Click to start";
		const Vec2 clickPos = { Scene::Center().x, Scene::Center().y + 100 };
		m_messageFont(clickText).drawAt(clickPos, ColorF{ 1.0, 1.0, 0.0 });
	}

}

Optional<SceneType> TitleScene::getNextScene() const
{
	return m_nextScene;
}

void TitleScene::cleanup()
{
	// リソースのクリーンアップ（必要に応じて）
}
