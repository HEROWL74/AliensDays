#include "ResultScene.hpp"
#include "GameScene.hpp"

ResultScene::ResultScene()
	: m_selectedButton(0)
	, m_buttonHovered(false)
	, m_animationTimer(0.0)
	, m_starAnimationDelay(0.0)
	, m_showStarAnimation(false)
	, m_nextScene(none)
{
	// デフォルトリザルトデータ
	m_resultData.clearedStage = StageNumber::Stage1;
	m_resultData.playerColor = PlayerColor::Green;
	m_resultData.collectedStars = 0;
	m_resultData.totalStars = 3;
	m_resultData.collectedCoins = 0;
	m_resultData.clearTime = 0.0;
	m_resultData.stageName = U"Unknown Stage";
	m_resultData.playerColorName = U"Green";
}

void ResultScene::init()
{
	loadTextures();

	// フォントの初期化
	m_titleFont = Font(36, Typeface::Bold);
	m_headerFont = Font(24, Typeface::Bold);
	m_dataFont = Font(20);
	m_buttonFont = Font(18, Typeface::Bold);

	// GameSceneからリザルトデータを取得
	StageNumber clearedStage = GameScene::getNextStageNumber();
	if (static_cast<int>(clearedStage) > 1)
	{
		// 前のステージをクリアした場合
		clearedStage = static_cast<StageNumber>(static_cast<int>(clearedStage) - 1);
	}

	setResultData(
		clearedStage,
		GameScene::getResultPlayerColor(),
		GameScene::getResultStars(),
		3, // 総スター数は常に3
		GameScene::getResultCoins(),
		GameScene::getResultTime()
	);

	// UI要素の設定
	setupPanel();
	setupButtons();

	// アニメーション初期化
	m_animationTimer = 0.0;
	m_starAnimationDelay = 1.0;
	m_showStarAnimation = false;
	m_selectedButton = 0;
	m_nextScene = none;
}

void ResultScene::update()
{
	m_animationTimer += Scene::DeltaTime();

	// スターアニメーション開始タイミング
	if (m_animationTimer >= m_starAnimationDelay && !m_showStarAnimation)
	{
		m_showStarAnimation = true;
	}

	updateInput();
	updateAnimations();
}

void ResultScene::draw() const
{
	drawBackground();
	drawParticleEffects();
	drawPanel();
	drawTitle();
	drawStageInfo();
	drawStarResult();
	drawGameStats();
	drawButtons();

	// 操作説明
	const String instructions = U"↑↓: Select  ENTER: Confirm  ESC: Title";
	m_dataFont(instructions).draw(10, Scene::Height() - 30, ColorF(0.8, 0.8, 0.8, 0.7));
}

Optional<SceneType> ResultScene::getNextScene() const
{
	return m_nextScene;
}

void ResultScene::cleanup()
{
	// 必要に応じてクリーンアップ
}

void ResultScene::setResultData(StageNumber stage, PlayerColor playerColor,
								int stars, int totalStars, int coins, double time)
{
	m_resultData.clearedStage = stage;
	m_resultData.playerColor = playerColor;
	m_resultData.collectedStars = stars;
	m_resultData.totalStars = totalStars;
	m_resultData.collectedCoins = coins;
	m_resultData.clearTime = time;
	m_resultData.stageName = Stage::getStageName(stage);
	m_resultData.playerColorName = getPlayerColorName(playerColor);

	// ボタンの有効/無効を設定
	setupButtons();
}

void ResultScene::loadTextures()
{
	// テクスチャの読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");
	m_buttonTexture = Texture(U"UI/PNG/Yellow/button_rectangle_depth_gradient.png");
	m_starFilledTexture = Texture(U"UI/PNG/Yellow/star.png");
	m_starOutlineTexture = Texture(U"UI/PNG/Yellow/star_outline_depth.png");
	m_coinTexture = Texture(U"Sprites/Tiles/hud_coin.png");
}

void ResultScene::setupPanel()
{
	const double panelWidth = 700.0;
	const double panelHeight = 600.0;
	m_panelRect = RectF(
		Scene::Center().x - panelWidth / 2,
		Scene::Center().y - panelHeight / 2,
		panelWidth,
		panelHeight
	);
}

void ResultScene::setupButtons()
{
	m_buttons.clear();

	const double buttonWidth = 180.0;
	const double buttonHeight = 50.0;
	const double buttonSpacing = 60.0;
	const double startY = m_panelRect.y + m_panelRect.h - 80;
	const double startX = m_panelRect.center().x - (buttonWidth * 1.5 + buttonSpacing);

	// Next Stage ボタン
	ButtonData nextButton;
	nextButton.text = U"NEXT STAGE";
	nextButton.rect = RectF(startX, startY, buttonWidth, buttonHeight);
	nextButton.action = ButtonAction::NextStage;
	nextButton.enabled = (static_cast<int>(m_resultData.clearedStage) < 6); // Stage6まで
	m_buttons.push_back(nextButton);

	// Retry ボタン
	ButtonData retryButton;
	retryButton.text = U"RETRY";
	retryButton.rect = RectF(startX + buttonWidth + buttonSpacing, startY, buttonWidth, buttonHeight);
	retryButton.action = ButtonAction::Retry;
	retryButton.enabled = true;
	m_buttons.push_back(retryButton);

	// Back to Title ボタン
	ButtonData titleButton;
	titleButton.text = U"TITLE";
	titleButton.rect = RectF(startX + (buttonWidth + buttonSpacing) * 2, startY, buttonWidth, buttonHeight);
	titleButton.action = ButtonAction::BackToTitle;
	titleButton.enabled = true;
	m_buttons.push_back(titleButton);
}

void ResultScene::updateInput()
{
	// キーボード操作
	if (KeyUp.down() || KeyW.down())
	{
		do {
			m_selectedButton = (m_selectedButton - 1 + static_cast<int>(m_buttons.size())) % static_cast<int>(m_buttons.size());
		} while (!m_buttons[m_selectedButton].enabled);
	}
	if (KeyDown.down() || KeyS.down())
	{
		do {
			m_selectedButton = (m_selectedButton + 1) % static_cast<int>(m_buttons.size());
		} while (!m_buttons[m_selectedButton].enabled);
	}

	// マウス操作
	const Vec2 mousePos = Cursor::Pos();
	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		if (m_buttons[i].enabled && m_buttons[i].rect.contains(mousePos))
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
		m_nextScene = SceneType::Title;
	}
}

void ResultScene::updateAnimations()
{
	// アニメーション更新処理
}

void ResultScene::drawBackground() const
{
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw(ColorF(0.6, 0.6, 0.6));
	}
	else
	{
		Scene::Rect().draw(ColorF(0.1, 0.1, 0.2));
	}

	// オーバーレイ
	Scene::Rect().draw(ColorF(0.0, 0.0, 0.0, 0.4));
}

void ResultScene::drawPanel() const
{
	// パネル背景
	m_panelRect.draw(ColorF(0.05, 0.05, 0.15, 0.95));
	m_panelRect.drawFrame(4.0, ColorF(0.8, 0.8, 0.6));

	// パネル内側の装飾
	const RectF innerRect = m_panelRect.stretched(-8);
	innerRect.drawFrame(2.0, ColorF(0.6, 0.6, 0.4, 0.3));
}

void ResultScene::drawTitle() const
{
	const String title = U"STAGE CLEAR!";
	const Vec2 titlePos = Vec2(m_panelRect.center().x, m_panelRect.y + 40);

	// タイトルの光る効果
	for (int i = 2; i >= 0; --i)
	{
		const Vec2 offset = Vec2(i * 1.5, i * 1.5);
		const double alpha = (i == 0) ? 1.0 : 0.4;
		const ColorF color = (i == 0) ? ColorF(1.0, 1.0, 0.6) : ColorF(0.6, 1.0, 0.6, alpha);

		m_titleFont(title).drawAt(titlePos + offset, color);
	}
}

void ResultScene::drawStageInfo() const
{
	const double startY = m_panelRect.y + 90;

	// ステージ情報
	const String stageText = U"Stage {}: {}"_fmt(
		static_cast<int>(m_resultData.clearedStage),
		m_resultData.stageName
	);
	m_headerFont(stageText).drawAt(Vec2(m_panelRect.center().x, startY), ColorF(0.9, 0.9, 0.9));

	// プレイヤー情報
	const String playerText = U"Player: {}"_fmt(m_resultData.playerColorName);
	m_dataFont(playerText).drawAt(Vec2(m_panelRect.center().x, startY + 35), ColorF(0.8, 0.8, 0.8));
}

void ResultScene::drawStarResult() const
{
	const double startY = m_panelRect.y + 180;
	const Vec2 basePos = Vec2(m_panelRect.center().x, startY);

	// スターセクションタイトル
	m_headerFont(U"STARS COLLECTED").drawAt(basePos, ColorF(1.0, 1.0, 0.8));

	// スター表示
	const double starSize = 60.0;
	const double starSpacing = 80.0;
	const Vec2 starStartPos = Vec2(
		basePos.x - (starSpacing * (m_resultData.totalStars - 1)) / 2,
		basePos.y + 50
	);

	for (int i = 0; i < m_resultData.totalStars; ++i)
	{
		const Vec2 starPos = starStartPos + Vec2(i * starSpacing, 0);
		const bool collected = (i < m_resultData.collectedStars);

		// スターアニメーション
		double starScale = 1.0;
		double starAlpha = 1.0;
		if (m_showStarAnimation && collected)
		{
			const double animTime = m_animationTimer - m_starAnimationDelay - (i * 0.3);
			if (animTime > 0)
			{
				starScale = 1.0 + std::sin(animTime * 8.0) * 0.2 * std::exp(-animTime * 2.0);
			}
		}

		// スター描画
		const Texture& starTexture = collected ? m_starFilledTexture : m_starOutlineTexture;
		const ColorF starColor = collected ? getStarColor(i) : ColorF(0.3, 0.3, 0.3);

		if (starTexture)
		{
			const Size scaledSize = Size(static_cast<int>(starSize * starScale), static_cast<int>(starSize * starScale));
			starTexture.resized(scaledSize).drawAt(starPos, ColorF(starColor.r, starColor.g, starColor.b, starAlpha));
		}
		else
		{
			Circle(starPos, starSize * starScale * 0.4).draw(starColor);
		}
	}

	// スターレーティング
	const String ratingText = getStarRating();
	const Vec2 ratingPos = Vec2(basePos.x, basePos.y + 120);
	m_dataFont(ratingText).drawAt(ratingPos, ColorF(1.0, 1.0, 0.8));
}

void ResultScene::drawGameStats() const
{
	const double startY = m_panelRect.y + 370;
	const Vec2 basePos = Vec2(m_panelRect.center().x, startY);

	// 統計セクションタイトル
	m_headerFont(U"GAME STATS").drawAt(basePos, ColorF(0.8, 1.0, 0.8));

	// クリア時間
	const String timeText = U"Clear Time: {}"_fmt(formatTime(m_resultData.clearTime));
	m_dataFont(timeText).drawAt(Vec2(basePos.x, basePos.y + 35), ColorF(0.9, 0.9, 0.9));

	// コイン数
	const String coinText = U"Coins Collected: {}"_fmt(m_resultData.collectedCoins);
	m_dataFont(coinText).drawAt(Vec2(basePos.x, basePos.y + 60), ColorF(0.9, 0.9, 0.9));

	// コインアイコン
	if (m_coinTexture)
	{
		const Vec2 coinPos = Vec2(basePos.x - 80, basePos.y + 60);
		m_coinTexture.resized(24, 24).drawAt(coinPos);
	}
}

void ResultScene::drawButtons() const
{
	const double time = Scene::Time();

	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		const ButtonData& button = m_buttons[i];
		const bool isSelected = (m_selectedButton == static_cast<int>(i));
		const bool isEnabled = button.enabled;

		// ボタンの色設定
		ColorF buttonColor = isEnabled ? ColorF(0.8, 0.8, 0.8) : ColorF(0.4, 0.4, 0.4);
		ColorF textColor = isEnabled ? ColorF(0.9, 0.9, 0.9) : ColorF(0.5, 0.5, 0.5);

		if (isSelected && isEnabled)
		{
			const double glow = 0.8 + 0.2 * std::sin(time * 6.0);
			buttonColor = ColorF(1.0, 1.0, glow);
			textColor = ColorF(0.1, 0.1, 0.1);
		}

		// ボタン描画
		if (m_buttonTexture)
		{
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

void ResultScene::drawParticleEffects() const
{
	const double time = Scene::Time();

	// 背景のキラキラエフェクト
	for (int i = 0; i < 20; ++i)
	{
		const double particleTime = time + i * 0.4;
		const double x = std::fmod(i * 127.0, Scene::Width());
		const double y = std::fmod(particleTime * 20.0 + i * 150.0, Scene::Height() + 50.0) - 25.0;
		const double alpha = (std::sin(particleTime * 2.0) + 1.0) * 0.2;
		const double size = 1.0 + std::sin(particleTime * 4.0) * 0.5;

		if (alpha > 0.1)
		{
			Circle(x, y, size).draw(ColorF(1.0, 1.0, 0.8, alpha));
		}
	}
}

void ResultScene::executeButton(int buttonIndex)
{
	if (buttonIndex < 0 || buttonIndex >= static_cast<int>(m_buttons.size()))
		return;

	if (!m_buttons[buttonIndex].enabled)
		return;

	const ButtonAction action = m_buttons[buttonIndex].action;

	switch (action)
	{
	case ButtonAction::NextStage:
		// 次のステージモードを設定
		GameScene::setNextStageMode();
		m_nextScene = SceneType::Game;
		break;

	case ButtonAction::Retry:
		// リトライモードを設定
		GameScene::setRetryMode();
		m_nextScene = SceneType::Game;
		break;

	case ButtonAction::BackToTitle:
		// GameSceneのデータをクリア
		GameScene::clearResultData();
		m_nextScene = SceneType::Title;
		break;
	}
}

String ResultScene::getPlayerColorName(PlayerColor color) const
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

String ResultScene::formatTime(double seconds) const
{
	const int minutes = static_cast<int>(seconds) / 60;
	const int secs = static_cast<int>(seconds) % 60;
	const int centisecs = static_cast<int>((seconds - static_cast<int>(seconds)) * 100);

	return U"{:02d}:{:02d}.{:02d}"_fmt(minutes, secs, centisecs);
}

ColorF ResultScene::getStarColor(int starIndex) const
{
	switch (starIndex)
	{
	case 0: return ColorF(1.0, 1.0, 0.4);  // 金色
	case 1: return ColorF(0.9, 0.9, 1.0);  // 銀色
	case 2: return ColorF(1.0, 0.7, 0.4);  // 銅色
	default: return ColorF(1.0, 1.0, 0.6);
	}
}

String ResultScene::getStarRating() const
{
	switch (m_resultData.collectedStars)
	{
	case 0: return U"Try harder next time!";
	case 1: return U"Good job!";
	case 2: return U"Great work!";
	case 3: return U"PERFECT! ★★★";
	default: return U"Amazing!";
	}
}
