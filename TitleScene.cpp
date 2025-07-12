#include "TitleScene.hpp"

TitleScene::TitleScene()
	: m_nextScene(none)
	, m_selectedButton(0)
	, m_buttonHoverTimer(0.0)
{
}

void TitleScene::init()
{
	// 背景画像の読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");
	//ゲームロゴ読み込み
	m_gameLogoTexture = Texture(U"Sprites/gameLogo.png");

	// ボタン画像の読み込み
	m_buttonTexture = Texture(U"UI/PNG/Yellow/button_rectangle_depth_gradient.png");

	// フォントの初期化
	m_titleFont = Font(48, Typeface::Bold);
	m_messageFont = Font(24);
	m_buttonFont = Font(20, Typeface::Bold);

	// ボタンの設定
	setupButtons();

	// 次のシーンをリセット
	m_nextScene = none;
	m_selectedButton = 0;
	m_buttonHoverTimer = 0.0;
}

void TitleScene::update()
{
	// ボタンホバータイマー更新
	m_buttonHoverTimer += Scene::DeltaTime();

	// キーボード操作
	if (KeyUp.down() || KeyW.down())
	{
		m_selectedButton = (m_selectedButton - 1 + static_cast<int>(m_buttons.size())) % static_cast<int>(m_buttons.size());
		m_buttonHoverTimer = 0.0;
	}
	if (KeyDown.down() || KeyS.down())
	{
		m_selectedButton = (m_selectedButton + 1) % static_cast<int>(m_buttons.size());
		m_buttonHoverTimer = 0.0;
	}

	// マウス操作
	const Vec2 mousePos = Cursor::Pos();
	bool mouseHoverDetected = false;

	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		if (m_buttons[i].rect.contains(mousePos))
		{
			if (m_selectedButton != static_cast<int>(i))
			{
				m_selectedButton = static_cast<int>(i);
				m_buttonHoverTimer = 0.0;
			}
			mouseHoverDetected = true;
			break;
		}
	}

	// ボタンの実行
	if (KeyEnter.down() || KeySpace.down() ||
		(MouseL.down() && mouseHoverDetected))
	{
		executeButton(m_selectedButton);
	}

	// 従来のクイックスタート（互換性のため）
	if (KeyG.down())  // Gキーでゲーム直行
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

	// ロゴの描画
	if (m_gameLogoTexture)
	{
		drawAnimatedLogo();
	}

	// 背景にパーティクル風エフェクト
	drawBackgroundParticles();

	// ボタンの描画
	drawButtons();

	// 操作説明
	drawControlInstructions();
}

Optional<SceneType> TitleScene::getNextScene() const
{
	return m_nextScene;
}

void TitleScene::cleanup()
{
	// リソースのクリーンアップ（必要に応じて）
}

void TitleScene::setupButtons()
{
	const double buttonWidth = 280.0;  // ボタン幅を大きく
	const double buttonHeight = 80.0;  // ボタン高さを大きく
	const double buttonSpacing = 100.0;
	const Vec2 startPos = Vec2(Scene::Center().x - buttonWidth / 2, Scene::Center().y + 120);

	// ボタンデータの設定
	m_buttons.clear();

	ButtonData startButton;
	startButton.text = U"START";
	startButton.rect = RectF(startPos.x, startPos.y, buttonWidth, buttonHeight);
	startButton.action = ButtonAction::Start;
	m_buttons.push_back(startButton);

	ButtonData optionButton;
	optionButton.text = U"OPTION";
	optionButton.rect = RectF(startPos.x, startPos.y + buttonSpacing, buttonWidth, buttonHeight);
	optionButton.action = ButtonAction::Option;
	m_buttons.push_back(optionButton);

	ButtonData creditsButton;
	creditsButton.text = U"CREDITS";
	creditsButton.rect = RectF(startPos.x, startPos.y + buttonSpacing * 2, buttonWidth, buttonHeight);
	creditsButton.action = ButtonAction::Credits;
	m_buttons.push_back(creditsButton);

	ButtonData exitButton;
	exitButton.text = U"EXIT";
	exitButton.rect = RectF(startPos.x, startPos.y + buttonSpacing * 3, buttonWidth, buttonHeight);
	exitButton.action = ButtonAction::Exit;
	m_buttons.push_back(exitButton);
}

void TitleScene::drawAnimatedLogo() const
{
	const double time = Scene::Time();

	// 複数のアニメーション効果を組み合わせ
	const double bounce = std::sin(time * 1.5) * 8.0;           // バウンス
	const double scale = 1.8 + std::sin(time * 2.0) * 0.05;     // スケール変化（大きく）
	const double rotation = std::sin(time * 0.8) * 0.05;        // 軽い回転
	const double sway = std::cos(time * 1.2) * 3.0;             // 左右の揺れ

	// ロゴの位置計算（ボタンのためにもう少し上に）
	const Vec2 logoPos = Vec2(Scene::Center().x + sway, Scene::Center().y - 180 + bounce);

	// 光る効果のための複数描画（グロー効果）
	for (int i = 3; i >= 0; --i)
	{
		const double glowScale = scale * (1.0 + i * 0.02);
		const double glowAlpha = 0.15 + i * 0.05;
		const ColorF glowColor = ColorF(1.0, 1.0, 0.8, glowAlpha);

		// グロー効果
		if (i > 0)
		{
			m_gameLogoTexture.scaled(glowScale).rotated(rotation).drawAt(logoPos, glowColor);
		}
		else
		{
			// メインロゴ
			m_gameLogoTexture.scaled(scale).rotated(rotation).drawAt(logoPos);
		}
	}

	// キラキラエフェクト
	for (int i = 0; i < 5; ++i)
	{
		const double sparkleTime = time + i * 1.2;
		const double sparkleAlpha = (std::sin(sparkleTime * 3.0) + 1.0) * 0.3;

		if (sparkleAlpha > 0.3)
		{
			const Vec2 sparkleOffset = Vec2(
				std::cos(sparkleTime + i) * 80.0,
				std::sin(sparkleTime * 1.5 + i) * 50.0
			);
			const Vec2 sparklePos = logoPos + sparkleOffset;

			// 星型のキラキラ
			const double sparkleSize = 3.0 + std::sin(sparkleTime * 4.0) * 2.0;

			// 星の描画（十字型）
			Line(sparklePos.x - sparkleSize, sparklePos.y,
				 sparklePos.x + sparkleSize, sparklePos.y)
				.draw(2.0, ColorF(1.0, 1.0, 0.8, sparkleAlpha));
			Line(sparklePos.x, sparklePos.y - sparkleSize,
				 sparklePos.x, sparklePos.y + sparkleSize)
				.draw(2.0, ColorF(1.0, 1.0, 0.8, sparkleAlpha));
		}
	}
}

void TitleScene::drawButtons() const
{
	const double time = Scene::Time();

	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		const ButtonData& button = m_buttons[i];
		const bool isSelected = (m_selectedButton == static_cast<int>(i));

		// ボタンのアニメーション効果
		double buttonScale = 1.0;
		double buttonAlpha = 0.8;
		ColorF buttonColor = ColorF(1.0, 1.0, 1.0);

		if (isSelected)
		{
			// 選択されたボタンの効果（拡大エフェクト復活）
			buttonScale = 1.1 + std::sin(time * 8.0 + m_buttonHoverTimer * 10.0) * 0.05;
			buttonAlpha = 1.0;

			// 選択されたボタンは光る
			const double glowIntensity = 0.5 + 0.5 * std::sin(time * 6.0);
			buttonColor = ColorF(1.0, 1.0, 0.8 + glowIntensity * 0.2);
		}
		// 選択されていないボタンは通常サイズ（1.0）のまま

		// ボタン背景の描画
		const Vec2 buttonCenter = button.rect.center();
		const Vec2 buttonSize = button.rect.size * buttonScale;

		if (m_buttonTexture)
		{
			// ボタン画像を使用
			if (isSelected)
			{
				// グロー効果
				for (int glow = 2; glow >= 0; --glow)
				{
					const double glowScale = buttonScale * (1.0 + glow * 0.08);
					const double glowAlpha = buttonAlpha * (0.3 + glow * 0.1);
					const ColorF glowColor = ColorF(1.0, 1.0, 0.6, glowAlpha);

					m_buttonTexture.resized(buttonSize * glowScale).drawAt(buttonCenter,
						glow == 0 ? buttonColor : glowColor);
				}
			}
			else
			{
				m_buttonTexture.resized(buttonSize).drawAt(buttonCenter,
					ColorF(buttonColor.r, buttonColor.g, buttonColor.b, buttonAlpha));
			}
		}
		else
		{
			// フォールバック：矩形で描画
			RectF buttonRect = RectF(Arg::center = buttonCenter, buttonSize);

			if (isSelected)
			{
				// グロー効果
				buttonRect.drawFrame(4.0, ColorF(1.0, 1.0, 0.6, 0.8));
				buttonRect.draw(ColorF(0.3, 0.3, 0.1, 0.9));
			}
			else
			{
				buttonRect.drawFrame(2.0, ColorF(0.6, 0.6, 0.6, buttonAlpha));
				buttonRect.draw(ColorF(0.2, 0.2, 0.2, buttonAlpha));
			}
		}

		// ボタンテキストの描画（フォントサイズを大きく）
		const Font largeButtonFont = Font(26, Typeface::Bold);  // ボタンフォントを大きく
		ColorF textColor = isSelected ? ColorF(0.1, 0.1, 0.1) : ColorF(0.9, 0.9, 0.9);

		// 選択されたボタンのテキストにも軽い効果
		if (isSelected)
		{
			// 影効果
			largeButtonFont(button.text).drawAt(buttonCenter + Vec2(2, 2), ColorF(0.0, 0.0, 0.0, 0.5));
			// メインテキスト
			largeButtonFont(button.text).drawAt(buttonCenter, textColor);
		}
		else
		{
			// 通常のテキスト
			largeButtonFont(button.text).drawAt(buttonCenter + Vec2(1, 1), ColorF(0.0, 0.0, 0.0, 0.3));
			largeButtonFont(button.text).drawAt(buttonCenter, textColor);
		}
	}
}

void TitleScene::drawBackgroundParticles() const
{
	const double time = Scene::Time();

	// 背景にパーティクル風エフェクト
	for (int i = 0; i < 15; ++i)
	{
		const double particleTime = time + i * 0.5;
		const double x = 100.0 + (i * 120.0) + std::sin(particleTime * 0.8) * 50.0;
		const double y = 100.0 + std::sin(particleTime * 1.2 + i) * Scene::Height() * 0.8;
		const double size = 1.5 + std::sin(particleTime * 3.0) * 1.0;
		const double alpha = (std::sin(particleTime * 2.0) + 1.0) * 0.2;

		if (alpha > 0.05)
		{
			Circle(x, y, size).draw(ColorF(0.8, 0.9, 1.0, alpha));
		}
	}
}

void TitleScene::drawControlInstructions() const
{
	const String instructions = U"↑↓ or W/S: Select  ENTER/SPACE: Confirm  G: Quick Start";
	const Vec2 instructionsPos = Vec2(10, Scene::Height() - 30);

	m_messageFont(instructions).draw(instructionsPos + Vec2(1, 1), ColorF(0.0, 0.0, 0.0, 0.5));
	m_messageFont(instructions).draw(instructionsPos, ColorF(0.7, 0.7, 0.7, 0.8));
}

void TitleScene::executeButton(int buttonIndex)
{
	if (buttonIndex < 0 || buttonIndex >= static_cast<int>(m_buttons.size()))
		return;

	const ButtonAction action = m_buttons[buttonIndex].action;

	switch (action)
	{
	case ButtonAction::Start:
		m_nextScene = SceneType::CharacterSelect;  // キャラクター選択画面に遷移
		break;

	case ButtonAction::Option:
		m_nextScene = SceneType::Option;  // オプションシーンに遷移
		break;

	case ButtonAction::Credits:
		m_nextScene = SceneType::Credit;  // クレジットシーンに遷移
		break;

	case ButtonAction::Exit:
		// ゲーム終了
		System::Exit();
		break;
	}
}
