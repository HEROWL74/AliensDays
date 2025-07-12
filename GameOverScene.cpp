#include "GameOverScene.hpp"
#include "GameScene.hpp"

GameOverScene::GameOverScene()
	: m_selectedButton(0)
	, m_animationTimer(0.0)
	, m_fadeAlpha(1.0)
	, m_currentStage(StageNumber::Stage1)
	, m_playerColor(PlayerColor::Green)
	, m_nextScene(none)
{
}

void GameOverScene::init()
{
	loadTextures();

	// フォントの初期化
	m_titleFont = Font(48, Typeface::Bold);
	m_messageFont = Font(24);
	m_buttonFont = Font(20, Typeface::Bold);

	// GameSceneからデータを取得
	m_currentStage = GameScene::getNextStageNumber();
	if (static_cast<int>(m_currentStage) > 1)
	{
		m_currentStage = static_cast<StageNumber>(static_cast<int>(m_currentStage) - 1);
	}
	m_playerColor = GameScene::getResultPlayerColor();

	// UI要素の設定
	setupButtons();

	// 初期化
	m_animationTimer = 0.0;
	m_fadeAlpha = 1.0;
	m_selectedButton = 0;
	m_nextScene = none;
}

void GameOverScene::update()
{
	m_animationTimer += Scene::DeltaTime();

	updateAnimations();
	updateInput();
}

void GameOverScene::draw() const
{
	drawBackground();
	drawParticleEffects();
	drawTitle();
	drawStageInfo();
	drawButtons();

	// フェードイン効果
	if (m_fadeAlpha > 0.0)
	{
		Scene::Rect().draw(ColorF(0.0, 0.0, 0.0, m_fadeAlpha));
	}

	// 操作説明
	const String instructions = U"↑↓: Select  ENTER: Confirm  ESC: Title";
	m_messageFont(instructions).draw(10, Scene::Height() - 30, ColorF(0.8, 0.8, 0.8, 0.7));
}

Optional<SceneType> GameOverScene::getNextScene() const
{
	return m_nextScene;
}

void GameOverScene::cleanup()
{
	// 必要に応じてクリーンアップ
}

void GameOverScene::setGameOverData(StageNumber stage, PlayerColor playerColor)
{
	m_currentStage = stage;
	m_playerColor = playerColor;
}

void GameOverScene::loadTextures()
{
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");
	m_buttonTexture = Texture(U"UI/PNG/Yellow/button_rectangle_depth_gradient.png");
}

void GameOverScene::setupButtons()
{
	m_buttons.clear();

	const double buttonWidth = 200.0;
	const double buttonHeight = 60.0;
	const double buttonSpacing = 80.0;
	const double startY = Scene::Center().y + 100;

	// Retry ボタン
	ButtonData retryButton;
	retryButton.text = U"RETRY";
	retryButton.rect = RectF(
		Scene::Center().x - buttonWidth / 2,
		startY,
		buttonWidth,
		buttonHeight
	);
	retryButton.action = ButtonAction::Retry;
	m_buttons.push_back(retryButton);

	// Character Select ボタン
	ButtonData charSelectButton;
	charSelectButton.text = U"CHANGE CHARACTER";
	charSelectButton.rect = RectF(
		Scene::Center().x - buttonWidth / 2,
		startY + buttonSpacing,
		buttonWidth,
		buttonHeight
	);
	charSelectButton.action = ButtonAction::CharacterSelect;
	m_buttons.push_back(charSelectButton);

	// Title ボタン
	ButtonData titleButton;
	titleButton.text = U"BACK TO TITLE";
	titleButton.rect = RectF(
		Scene::Center().x - buttonWidth / 2,
		startY + buttonSpacing * 2,
		buttonWidth,
		buttonHeight
	);
	titleButton.action = ButtonAction::Title;
	m_buttons.push_back(titleButton);
}

void GameOverScene::updateInput()
{
	// キーボード操作
	if (KeyUp.down() || KeyW.down())
	{
		m_selectedButton = (m_selectedButton - 1 + static_cast<int>(m_buttons.size())) % static_cast<int>(m_buttons.size());
	}
	if (KeyDown.down() || KeyS.down())
	{
		m_selectedButton = (m_selectedButton + 1) % static_cast<int>(m_buttons.size());
	}

	// マウス操作
	const Vec2 mousePos = Cursor::Pos();
	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		if (m_buttons[i].rect.contains(mousePos))
		{
			m_selectedButton = static_cast<int>(i);
			break;
		}
	}

	// 決定
	if (KeyEnter.down() || KeySpace.down() ||
		(MouseL.down() && m_buttons[m_selectedButton].rect.contains(mousePos)))
	{
		executeButton(m_selectedButton);
	}

	// ESCでタイトルに戻る
	if (KeyEscape.down())
	{
		GameScene::clearResultData();
		m_nextScene = SceneType::Title;
	}
}

void GameOverScene::updateAnimations()
{
	// フェードイン効果
	if (m_fadeAlpha > 0.0)
	{
		m_fadeAlpha = Math::Max(0.0, m_fadeAlpha - Scene::DeltaTime() * 2.0);
	}
}

void GameOverScene::drawBackground() const
{
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw(ColorF(0.3, 0.3, 0.3));
	}
	else
	{
		Scene::Rect().draw(ColorF(0.1, 0.1, 0.1));
	}

	// 暗いオーバーレイ
	Scene::Rect().draw(ColorF(0.0, 0.0, 0.0, 0.6));
}

void GameOverScene::drawTitle() const
{
	const String title = U"GAME OVER";
	const Vec2 titlePos = Vec2(Scene::Center().x, Scene::Center().y - 150);

	// タイトルの効果（赤っぽい色で）
	for (int i = 2; i >= 0; --i)
	{
		const Vec2 offset = Vec2(i * 2, i * 2);
		const double alpha = (i == 0) ? 1.0 : 0.4;
		const ColorF color = (i == 0) ? ColorF(1.0, 0.3, 0.3) : ColorF(0.8, 0.2, 0.2, alpha);

		m_titleFont(title).drawAt(titlePos + offset, color);
	}

	// 脈動効果
	const double pulse = 0.8 + 0.2 * std::sin(m_animationTimer * 3.0);
	const double pulseSize = 52.0 * pulse;
	Font(static_cast<int>(pulseSize), Typeface::Bold)(title).drawAt(titlePos, ColorF(1.0, 0.4, 0.4, 0.3));
}

void GameOverScene::drawStageInfo() const
{
	const Vec2 infoPos = Vec2(Scene::Center().x, Scene::Center().y - 50);

	// ステージ情報
	const String stageText = U"Stage {}: {}"_fmt(
		static_cast<int>(m_currentStage),
		Stage::getStageName(m_currentStage)
	);
	m_messageFont(stageText).drawAt(infoPos, ColorF(0.9, 0.9, 0.9));

	// プレイヤー情報
	const String playerText = U"Player: {}"_fmt(getPlayerColorName(m_playerColor));
	m_messageFont(playerText).drawAt(infoPos + Vec2(0, 35), ColorF(0.8, 0.8, 0.8));
}

void GameOverScene::drawButtons() const
{
	const double time = Scene::Time();

	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		const ButtonData& button = m_buttons[i];
		const bool isSelected = (m_selectedButton == static_cast<int>(i));

		// ボタンの色設定
		ColorF buttonColor = isSelected ? ColorF(1.0, 1.0, 0.8) : ColorF(0.6, 0.6, 0.6);
		ColorF textColor = isSelected ? ColorF(0.1, 0.1, 0.1) : ColorF(0.9, 0.9, 0.9);

		if (isSelected)
		{
			const double glow = 0.8 + 0.2 * std::sin(time * 6.0);
			buttonColor = ColorF(1.0, 1.0, glow);
		}

		// ボタン描画
		if (m_buttonTexture)
		{
			if (isSelected)
			{
				// グロー効果
				m_buttonTexture.resized(button.rect.size * 1.05).drawAt(button.rect.center(), ColorF(1.0, 1.0, 0.6, 0.3));
			}
			m_buttonTexture.resized(button.rect.size).drawAt(button.rect.center(), buttonColor);
		}
		else
		{
			button.rect.draw(ColorF(0.3, 0.3, 0.3));
			button.rect.drawFrame(2.0, buttonColor);
		}

		// テキスト描画
		m_buttonFont(button.text).drawAt(button.rect.center(), textColor);
	}
}

void GameOverScene::drawParticleEffects() const
{
	const double time = m_animationTimer;

	// 暗いパーティクル効果
	for (int i = 0; i < 15; ++i)
	{
		const double particleTime = time + i * 0.4;
		const double x = std::fmod(i * 127.0, Scene::Width());
		const double y = std::fmod(particleTime * 15.0 + i * 150.0, Scene::Height() + 50.0) - 25.0;
		const double alpha = (std::sin(particleTime * 1.5) + 1.0) * 0.15;
		const double size = 1.0 + std::sin(particleTime * 3.0) * 0.5;

		if (alpha > 0.05)
		{
			Circle(x, y, size).draw(ColorF(0.5, 0.2, 0.2, alpha));
		}
	}
}

void GameOverScene::executeButton(int buttonIndex)
{
	if (buttonIndex < 0 || buttonIndex >= static_cast<int>(m_buttons.size()))
		return;

	const ButtonAction action = m_buttons[buttonIndex].action;

	switch (action)
	{
	case ButtonAction::Retry:
		// リトライモードを設定
		GameScene::setRetryMode();
		m_nextScene = SceneType::Game;
		break;

	case ButtonAction::CharacterSelect:
		// キャラクター選択画面に戻る
		GameScene::clearResultData();
		m_nextScene = SceneType::CharacterSelect;
		break;

	case ButtonAction::Title:
		// タイトル画面に戻る
		GameScene::clearResultData();
		m_nextScene = SceneType::Title;
		break;
	}
}

String GameOverScene::getPlayerColorName(PlayerColor color) const
{
	switch (color)
	{
	case PlayerColor::Green:  return U"Green";
	case PlayerColor::Pink:   return U"Pink";
	case PlayerColor::Purple: return U"Purple";
	case PlayerColor::Beige:  return U"Beige";
	case PlayerColor::Yellow: return U"Yellow";
	default: return U"Unknown";
	}
}
