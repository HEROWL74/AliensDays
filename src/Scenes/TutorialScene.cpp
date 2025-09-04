#include "TutorialScene.hpp"
#include "../Core/SceneFactory.hpp"

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
	// ★ GameSceneの初期化を先に実行
	GameScene::init();

	// チュートリアル固有の初期化
	goTo(Step::Move);

	// ★ イベント購読（通知->進行）
	m_subId = TutorialSubject::instance().subscribe(
		[this](TutorialEvent ev, const Vec2&) {
			switch (m_step) {
			case Step::Move:
				if (ev == TutorialEvent::MoveLeftRight) { goTo(Step::Jump); }
				break;
			case Step::Jump:
				if (ev == TutorialEvent::Jump) { goTo(Step::Stomp); }
				break;
			case Step::Stomp:
				if (ev == TutorialEvent::StompEnemy) { goTo(Step::Fireball); }
				break;
			case Step::Fireball:
				if (ev == TutorialEvent::FireballKill) { goTo(Step::Done); }
				break;
			default: break;
			}
		}
	);
}

void TutorialScene::update()
{
	// ★ GameSceneの更新処理を実行
	GameScene::update();

	// チュートリアルパネルの更新
	m_panel.update();

	// ★ 完了時の処理
	if (m_step == Step::Done && (KeyEnter.down() || KeySpace.down())) {
		m_nextScene = SceneType::Title;
	}
}

void TutorialScene::draw() const
{
	// ★ GameSceneの描画を実行
	GameScene::draw();

	// チュートリアルパネルを最前面に描画
	m_panel.draw();

	// 完了メッセージ
	if (m_step == Step::Done) {
		const RectF msgBox(Scene::Center().x - 200, Scene::Center().y + 120, 400, 60);
		msgBox.draw(ColorF(0.0, 0.0, 0.0, 0.7));
		msgBox.drawFrame(2, ColorF(1.0, 1.0, 0.6));

		Font(20, Typeface::Bold)(U"ENTER または SPACE でタイトルへ")
			.drawAt(msgBox.center(), ColorF(1.0, 1.0, 0.8));
	}
}

Optional<SceneType> TutorialScene::getNextScene() const
{
	// m_nextScene が設定されていればそれを返す、なければGameSceneの結果を返す
	if (m_nextScene) return m_nextScene;
	return GameScene::getNextScene();
}

void TutorialScene::cleanup()
{
	// ★ イベント購読解除
	TutorialSubject::instance().unsubscribe(m_subId);

	// GameSceneのクリーンアップ
	GameScene::cleanup();
}

void TutorialScene::goTo(Step s)
{
	m_step = s;
	switch (m_step) {
	case Step::Move:
		m_panel.show(U"← → または A/D で左右に動いてみよう！");
		break;
	case Step::Jump:
		m_panel.show(U"SPACE または W でジャンプしてみよう！");
		break;
	case Step::Stomp:
		m_panel.show(U"スライムを上から踏んでみよう！");
		break;
	case Step::Fireball:
		m_panel.show(U"F でファイアボール！敵を倒してみよう！");
		break;
	case Step::Done:
		m_panel.show(U"チュートリアル完了！よくできました！");
		break;
	}
	spawnFor(m_step);
}

void TutorialScene::spawnFor(Step s)
{
	switch (s) {
	case Step::Stomp: {
		//踏みやすい位置にスライムを配置
		addEnemy(spawnEnemy(U"NormalSlime", Vec2{ 500, 768 }));
		break;
	}
	case Step::Fireball: {
		//ファイアボールで倒しやすい位置にBeeを配置
		addEnemy(spawnEnemy(U"Bee", Vec2{ 700, 600 }));
		break;
	}
	default: break;
	}
}
