#include "GameScene.hpp"

GameScene::GameScene()
	: m_nextScene(none)
	, m_playerPos(Scene::Center())
	, m_gameTime(0.0)
{
}

void GameScene::init()
{
	// 背景画像の読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");

	// フォントの初期化
	m_gameFont = Font(24);

	// ゲーム状態の初期化
	m_playerPos = Scene::Center();
	m_gameTime = 0.0;
	m_nextScene = none;
}

void GameScene::update()
{
	// ゲーム時間の更新
	m_gameTime += Scene::DeltaTime();

	// プレイヤーの移動
	Vec2 moveVec(0, 0);
	if (KeyLeft.pressed() || KeyA.pressed())
	{
		moveVec.x -= 1;
	}
	if (KeyRight.pressed() || KeyD.pressed())
	{
		moveVec.x += 1;
	}
	if (KeyUp.pressed() || KeyW.pressed())
	{
		moveVec.y -= 1;
	}
	if (KeyDown.pressed() || KeyS.pressed())
	{
		moveVec.y += 1;
	}

	// プレイヤー位置の更新
	if (moveVec.length() > 0)
	{
		moveVec = moveVec.normalized() * 200.0 * Scene::DeltaTime();
		m_playerPos += moveVec;

		// 画面端での制限
		m_playerPos.x = Clamp(m_playerPos.x, 25.0, Scene::Width() - 25.0);
		m_playerPos.y = Clamp(m_playerPos.y, 25.0, Scene::Height() - 25.0);
	}

	// ESCキーでタイトルに戻る
	if (KeyEscape.down())
	{
		m_nextScene = SceneType::Title;
	}
}

void GameScene::draw() const
{
	// 背景の描画
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw();
	}
	else
	{
		// 背景画像が読み込めない場合はグラデーション背景
		Scene::Rect().draw(Arg::top = ColorF(0.2, 0.4, 0.6), Arg::bottom = ColorF(0.1, 0.3, 0.2));
	}

	// プレイヤーの描画
	Circle(m_playerPos, 25).draw(ColorF(1.0, 0.5, 0.0));
	Circle(m_playerPos, 25).drawFrame(3, ColorF(1.0, 1.0, 1.0));

	// UI要素の描画
	// ゲーム時間
	const String timeText = U"Time: {:.1f}s"_fmt(m_gameTime);
	m_gameFont(timeText).draw(10, 10, ColorF(1.0, 1.0, 1.0));

	// 操作説明
	const String controlText = U"WASD/Arrow Keys: Move, ESC: Back to Title";
	m_gameFont(controlText).draw(10, Scene::Height() - 40, ColorF(0.8, 0.8, 0.8));

	// プレイヤー位置
	const String posText = U"Position: ({:.0f}, {:.0f})"_fmt(m_playerPos.x, m_playerPos.y);
	m_gameFont(posText).draw(10, 40, ColorF(1.0, 1.0, 1.0));
}

Optional<SceneType> GameScene::getNextScene() const
{
	return m_nextScene;
}

void GameScene::cleanup()
{
	// リソースのクリーンアップ（必要に応じて）
}
