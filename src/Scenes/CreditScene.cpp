#include "CreditScene.hpp"
#include "../Sound/SoundManager.hpp"

CreditScene::CreditScene()
	: m_scrollOffset(0.0)
	, m_scrollSpeed(30.0)
	, m_totalHeight(0.0)
	, m_autoScroll(true)
	, m_backButtonHovered(false)
	, m_animationTimer(0.0)
	, m_nextScene(none)
{
}

void CreditScene::init()
{
	// テクスチャの読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");
	m_buttonTexture = Texture(U"UI/PNG/Yellow/button_rectangle_depth_gradient.png");

	// フォントの初期化
	m_titleFont = Font(48, Typeface::Bold);
	m_categoryFont = Font(28, Typeface::Bold);
	m_nameFont = Font(20);
	m_buttonFont = Font(20, Typeface::Bold);

	// タイトルBGMが再生されていない場合は開始
	SoundManager& soundManager = SoundManager::GetInstance();
	if (!soundManager.isBGMPlaying(SoundManager::SoundType::BGM_TITLE))
	{
		soundManager.playBGM(SoundManager::SoundType::BGM_TITLE);
	}

	// クレジット情報とボタンの設定
	setupCredits();
	setupButton();
	calculateTotalHeight();

	// 初期状態
	m_scrollOffset = Scene::Height();  // 画面下からスタート
	m_autoScroll = true;
	m_backButtonHovered = false;
	m_animationTimer = 0.0;
	m_nextScene = none;
}

void CreditScene::update()
{
	m_animationTimer += Scene::DeltaTime();

	updateInput();
	updateScroll();
}

void CreditScene::draw() const
{
	drawBackground();
	drawParticleEffects();
	drawCredits();
	drawBackButton();

	// 操作説明
	const String instructions = U"↑↓: Manual Scroll  SPACE: Toggle Auto-scroll  ESC/Click BACK: Return";
	m_nameFont(instructions).draw(10, Scene::Height() - 30, ColorF(0.8, 0.8, 0.8, 0.7));
}

Optional<SceneType> CreditScene::getNextScene() const
{
	return m_nextScene;
}

void CreditScene::cleanup()
{
	// BGMは停止しない（タイトルBGMを継続）
	// 必要に応じてクリーンアップ
}

void CreditScene::setupCredits()
{
	m_credits.clear();

	// ゲーム情報
	CreditEntry gameInfo;
	gameInfo.category = U"Game";
	gameInfo.names = { U"Alien's Days" };
	gameInfo.categoryColor = ColorF(1.0, 1.0, 0.8);
	gameInfo.nameColor = ColorF(0.9, 0.9, 0.9);
	m_credits.push_back(gameInfo);

	// 開発者
	CreditEntry development;
	development.category = U"DEVELOPMENT";
	development.names = {
		U"Lead Developer: HerowlGames",
		U"Programming: Hiroaki Togawa",
		U"Game Design: Hiroaki Togawa"
	};
	development.categoryColor = ColorF(0.8, 1.0, 0.8);
	development.nameColor = ColorF(0.9, 0.9, 0.9);
	m_credits.push_back(development);

	// エンジン・ツール
	CreditEntry engine;
	engine.category = U"ENGINE & TOOLS";
	engine.names = {
		U"Siv3D Framework",
		U"OpenSiv3D Community",
		U"C++ Standard Library"
	};
	engine.categoryColor = ColorF(0.8, 0.8, 1.0);
	engine.nameColor = ColorF(0.9, 0.9, 0.9);
	m_credits.push_back(engine);

	// アート・デザイン
	CreditEntry art;
	art.category = U"ART & DESIGN";
	art.names = {
		U"UI Assets: Kenney.nl",
		U"Background Art: Kenney.nl",
		U"Icon Design: ChatGPT"
	};
	art.categoryColor = ColorF(1.0, 0.8, 0.8);
	art.nameColor = ColorF(0.9, 0.9, 0.9);
	m_credits.push_back(art);

	// 音楽・効果音
	CreditEntry audio;
	audio.category = U"AUDIO";
	audio.names = {
		U"Sound Effects: Kenney.nl & Sprigin",
		U"Background Music: Kenney.nl"
	};
	audio.categoryColor = ColorF(1.0, 0.8, 1.0);
	audio.nameColor = ColorF(0.9, 0.9, 0.9);
	m_credits.push_back(audio);

	// 特別感謝
	CreditEntry thanks;
	thanks.category = U"SPECIAL THANKS";
	thanks.names = {
		U"Siv3D Development Team",
		U"OpenSource Community",
		U"Beta Testers"
	};
	thanks.categoryColor = ColorF(1.0, 1.0, 0.6);
	thanks.nameColor = ColorF(1.0, 1.0, 0.8);
	m_credits.push_back(thanks);

	// 終了メッセージ
	CreditEntry ending;
	ending.category = U"THANK YOU FOR PLAYING!";
	ending.names = { U"" };  // 空の名前で、カテゴリのみ表示
	ending.categoryColor = ColorF(1.0, 0.8, 0.6);
	ending.nameColor = ColorF(0.9, 0.9, 0.9);
	m_credits.push_back(ending);
}

void CreditScene::setupButton()
{
	const double buttonWidth = 120.0;
	const double buttonHeight = 50.0;

	m_backButtonRect = RectF(
		Scene::Width() - buttonWidth - 20,
		20,
		buttonWidth,
		buttonHeight
	);
}

void CreditScene::calculateTotalHeight()
{
	m_totalHeight = 100.0;  // 上部余白

	for (const auto& credit : m_credits)
	{
		m_totalHeight += getEntryHeight(credit);
	}

	m_totalHeight += 200.0;  // 下部余白
}

void CreditScene::updateInput()
{
	// マウスホバー検出
	const Vec2 mousePos = Cursor::Pos();
	m_backButtonHovered = m_backButtonRect.contains(mousePos);

	// 手動スクロール
	if (KeyUp.pressed() || KeyW.pressed())
	{
		m_scrollOffset += 100.0 * Scene::DeltaTime();
		m_autoScroll = false;
	}
	if (KeyDown.pressed() || KeyS.pressed())
	{
		m_scrollOffset -= 100.0 * Scene::DeltaTime();
		m_autoScroll = false;
	}

	// オートスクロール切り替え
	if (KeySpace.down())
	{
		m_autoScroll = !m_autoScroll;
	}

	// 戻る操作
	if (KeyEscape.down() || (MouseL.down() && m_backButtonHovered))
	{
		// SE再生
		SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
		m_nextScene = SceneType::Title;
	}

	// マウスホイールでスクロール
	const double wheel = Mouse::Wheel();
	if (wheel != 0.0)
	{
		m_scrollOffset += wheel * 50.0;
		m_autoScroll = false;
	}
}

void CreditScene::updateScroll()
{
	if (m_autoScroll)
	{
		m_scrollOffset -= m_scrollSpeed * Scene::DeltaTime();

		// スクロールが終了したら最初に戻る
		if (m_scrollOffset < -m_totalHeight)
		{
			m_scrollOffset = Scene::Height();
		}
	}

	// 手動スクロール時の範囲制限
	if (!m_autoScroll)
	{
		m_scrollOffset = Math::Clamp(m_scrollOffset, -m_totalHeight + 100.0, Scene::Height());
	}
}

void CreditScene::drawBackground() const
{
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw(ColorF(0.5, 0.5, 0.5, 0.8));
	}
	else
	{
		Scene::Rect().draw(ColorF(0.1, 0.1, 0.2));
	}

	// グラデーションオーバーレイ
	Scene::Rect().draw(Arg::top = ColorF(0.1, 0.1, 0.3, 0.3), Arg::bottom = ColorF(0.3, 0.1, 0.3, 0.3));
}

void CreditScene::drawCredits() const
{
	double currentY = m_scrollOffset;

	// タイトル
	const String title = U"CREDITS";
	const Vec2 titlePos = getDrawPosition(currentY);
	if (isVisible(currentY, 60))
	{
		// タイトルの光る効果
		for (int i = 2; i >= 0; --i)
		{
			const Vec2 offset = Vec2(i * 2, i * 2);
			const double alpha = (i == 0) ? 1.0 : 0.4;
			const ColorF color = (i == 0) ? ColorF(1.0, 1.0, 0.8) : ColorF(0.8, 0.8, 1.0, alpha);

			m_titleFont(title).drawAt(titlePos + offset, color);
		}
	}
	currentY -= 100.0;

	// クレジット項目
	for (const auto& credit : m_credits)
	{
		const double entryHeight = getEntryHeight(credit);

		if (isVisible(currentY, entryHeight))
		{
			// カテゴリ描画
			const Vec2 categoryPos = getDrawPosition(currentY);

			// カテゴリの光る効果
			const double categoryGlow = 0.7 + 0.3 * std::sin(m_animationTimer * 2.0);
			ColorF glowColor = credit.categoryColor;
			glowColor.a = categoryGlow;

			m_categoryFont(credit.category).drawAt(categoryPos + Vec2(2, 2), ColorF(0.0, 0.0, 0.0, 0.5));
			m_categoryFont(credit.category).drawAt(categoryPos, glowColor);

			currentY -= 50.0;

			// 名前リスト描画
			for (const auto& name : credit.names)
			{
				if (!name.isEmpty())
				{
					const Vec2 namePos = getDrawPosition(currentY);

					// 名前の描画（フェードイン効果）
					const double fadeAlpha = Math::Clamp(1.0 - std::abs(namePos.y - Scene::Center().y) / 200.0, 0.3, 1.0);
					ColorF nameColor = credit.nameColor;
					nameColor.a = fadeAlpha;

					m_nameFont(name).drawAt(namePos + Vec2(1, 1), ColorF(0.0, 0.0, 0.0, 0.3));
					m_nameFont(name).drawAt(namePos, nameColor);

					currentY -= 35.0;
				}
			}
		}
		else
		{
			currentY -= entryHeight;
		}

		currentY -= 20.0;  // セクション間の余白
	}
}

void CreditScene::drawBackButton() const
{
	// ボタンの描画
	ColorF buttonColor = m_backButtonHovered ? ColorF(1.0, 1.0, 0.8) : ColorF(0.8, 0.8, 0.8);

	if (m_buttonTexture)
	{
		if (m_backButtonHovered)
		{
			// ホバー時のグロー効果
			m_buttonTexture.resized(m_backButtonRect.size * 1.05).drawAt(m_backButtonRect.center(), ColorF(1.0, 1.0, 0.6, 0.3));
		}
		m_buttonTexture.resized(m_backButtonRect.size).drawAt(m_backButtonRect.center(), buttonColor);
	}
	else
	{
		m_backButtonRect.draw(ColorF(0.3, 0.3, 0.3));
		m_backButtonRect.drawFrame(2.0, buttonColor);
	}

	// ボタンテキスト
	ColorF textColor = m_backButtonHovered ? ColorF(0.1, 0.1, 0.1) : ColorF(0.9, 0.9, 0.9);
	m_buttonFont(U"BACK").drawAt(m_backButtonRect.center(), textColor);
}

void CreditScene::drawParticleEffects() const
{
	const double time = m_animationTimer;

	// 背景のキラキラエフェクト
	for (int i = 0; i < 20; ++i)
	{
		const double particleTime = time + i * 0.3;
		const double x = std::fmod(i * 97.0, Scene::Width());
		const double y = std::fmod(particleTime * 30.0 + i * 200.0, Scene::Height() + 100.0) - 50.0;
		const double alpha = (std::sin(particleTime * 3.0) + 1.0) * 0.3;
		const double size = 1.0 + std::sin(particleTime * 4.0) * 0.5;

		if (alpha > 0.1)
		{
			Circle(x, y, size).draw(ColorF(1.0, 1.0, 0.8, alpha));

			// 星型効果
			const double starSize = size * 2.0;
			Line(x - starSize, y, x + starSize, y).draw(1.0, ColorF(1.0, 1.0, 0.8, alpha * 0.5));
			Line(x, y - starSize, x, y + starSize).draw(1.0, ColorF(1.0, 1.0, 0.8, alpha * 0.5));
		}
	}
}

double CreditScene::getEntryHeight(const CreditEntry& entry) const
{
	double height = 50.0;  // カテゴリの高さ
	height += entry.names.size() * 35.0;  // 名前リストの高さ
	return height;
}

Vec2 CreditScene::getDrawPosition(double baseY) const
{
	return Vec2(Scene::Center().x, baseY);
}

bool CreditScene::isVisible(double y, double height) const
{
	return (y + height >= -50.0 && y <= Scene::Height() + 50.0);
}
