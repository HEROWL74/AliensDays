#include "ResultScene.hpp"
#include "GameScene.hpp"

ResultScene::ResultScene()
	: m_selectedButton(0)
	, m_buttonHovered(false)
	, m_animationTimer(0.0)
	, m_starAnimationDelay(0.0)
	, m_showStarAnimation(false)
	, m_blockAnimationTimer(0.0)
	, m_titlePulseTimer(0.0)
	, m_fireworksTimer(0.0)
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

	// パーティクル初期化
	m_particles.clear();
	m_fireworks.clear();
}

void ResultScene::init()
{
	loadTextures();

	// フォントの初期化
	m_titleFont = Font(42, Typeface::Bold);
	m_headerFont = Font(28, Typeface::Bold);
	m_dataFont = Font(22);
	m_buttonFont = Font(20, Typeface::Bold);

	// GameSceneからリザルトデータを取得
	StageNumber clearedStage = GameScene::getNextStageNumber();
	if (static_cast<int>(clearedStage) > 1)
	{
		clearedStage = static_cast<StageNumber>(static_cast<int>(clearedStage) - 1);
	}

	setResultData(
		clearedStage,
		GameScene::getResultPlayerColor(),
		GameScene::getResultStars(),
		3,
		GameScene::getResultCoins(),
		GameScene::getResultTime()
	);

	// UI要素の設定
	setupBlocks();
	setupButtons();
	setupParticles();

	// アニメーション初期化
	m_animationTimer = 0.0;
	m_starAnimationDelay = 1.2;
	m_showStarAnimation = false;
	m_blockAnimationTimer = 0.0;
	m_titlePulseTimer = 0.0;
	m_fireworksTimer = 0.0;
	m_selectedButton = 0;
	m_nextScene = none;

	// 花火エフェクトを準備（3つ星の場合）
	if (m_resultData.collectedStars == 3)
	{
		setupFireworks();
	}
}

void ResultScene::update()
{
	m_animationTimer += Scene::DeltaTime();
	m_blockAnimationTimer += Scene::DeltaTime();
	m_titlePulseTimer += Scene::DeltaTime();
	m_fireworksTimer += Scene::DeltaTime();

	// スターアニメーション開始タイミング
	if (m_animationTimer >= m_starAnimationDelay && !m_showStarAnimation)
	{
		m_showStarAnimation = true;
	}

	updateInput();
	updateAnimations();
	updateParticles();
	updateFireworks();
}

void ResultScene::draw() const
{
	drawBackground();
	drawBackgroundBlocks();
	drawMainPanel();
	drawTitle();
	drawStageInfo();
	drawStarResult();
	drawGameStats();
	drawButtons();
	drawParticles();
	drawFireworks();

	// 操作説明
	const String instructions = U"↑↓: Select  ENTER: Confirm  ESC: Title";
	m_dataFont(instructions).draw(10, Scene::Height() - 30, ColorF(0.9, 0.9, 0.9, 0.8));
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

	setupButtons();
}

void ResultScene::loadTextures()
{
	// 新しいテクスチャの読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");
	m_blockTexture = Texture(U"Sprites/Tiles/block_empty.png");
	m_buttonTexture = Texture(U"UI/PNG/Blue/button_rectangle_depth_flat.png");
	m_starFilledTexture = Texture(U"UI/PNG/Yellow/star.png");
	m_starOutlineTexture = Texture(U"UI/PNG/Yellow/star_outline_depth.png");
	m_coinTexture = Texture(U"Sprites/Tiles/hud_coin.png");
}

void ResultScene::setupBlocks()
{
	m_backgroundBlocks.clear();

	// 背景にランダムなブロックを配置
	for (int i = 0; i < 25; ++i)
	{
		BackgroundBlock block;
		block.position = Vec2(
			Random(-100.0, static_cast<double>(Scene::Width()) + 100.0),
			Random(-100.0, static_cast<double>(Scene::Height()) + 100.0)
		);
		block.rotation = Random(0.0, Math::TwoPi);
		block.scale = Random(0.3, 0.8);
		block.alpha = Random(0.1, 0.3);
		block.rotationSpeed = Random(-1.0, 1.0);
		block.bobSpeed = Random(0.5, 2.0);
		block.bobPhase = Random(0.0, Math::TwoPi);
		m_backgroundBlocks.push_back(block);
	}
}

void ResultScene::setupButtons()
{
	m_buttons.clear();

	const double buttonWidth = 200.0;
	const double buttonHeight = 60.0;
	const double buttonSpacing = 80.0;
	const double startY = Scene::Height() - 180;
	const double totalWidth = buttonWidth * 3 + buttonSpacing * 2;
	const double startX = Scene::Center().x - totalWidth / 2;

	// Next Stage ボタン
	ButtonData nextButton;
	nextButton.text = U"NEXT STAGE";
	nextButton.rect = RectF(startX, startY, buttonWidth, buttonHeight);
	nextButton.action = ButtonAction::NextStage;
	nextButton.enabled = (static_cast<int>(m_resultData.clearedStage) < 6);
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

void ResultScene::setupParticles()
{
	m_particles.clear();

	// 初期パーティクルを生成
	for (int i = 0; i < 30; ++i)
	{
		createParticle(Vec2(Random(0.0, static_cast<double>(Scene::Width())), Random(0.0, static_cast<double>(Scene::Height()))));
	}
}

void ResultScene::setupFireworks()
{
	m_fireworks.clear();

	// 3つ星クリア時の花火を準備
	if (m_resultData.collectedStars == 3)
	{
		for (int i = 0; i < 5; ++i)
		{
			Firework firework;
			firework.position = Vec2(
				Random(100.0, static_cast<double>(Scene::Width()) - 100.0),
				Random(100.0, static_cast<double>(Scene::Height()) - 200.0)
			);
			firework.delay = static_cast<double>(i) * 0.4 + 2.0; // 2秒後から開始
			firework.active = false;
			firework.exploded = false;
			firework.timer = 0.0;
			m_fireworks.push_back(firework);
		}
	}
}

void ResultScene::updateInput()
{
	// 前回の選択を保存
	const int previousSelection = m_selectedButton;

	// キーボード操作
	if (KeyUp.down() || KeyW.down())
	{
		do {
			m_selectedButton = (m_selectedButton - 1 + static_cast<int>(m_buttons.size())) % static_cast<int>(m_buttons.size());
		} while (!m_buttons[m_selectedButton].enabled);

		if (m_selectedButton != previousSelection)
		{
			SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
		}
	}
	if (KeyDown.down() || KeyS.down())
	{
		do {
			m_selectedButton = (m_selectedButton + 1) % static_cast<int>(m_buttons.size());
		} while (!m_buttons[m_selectedButton].enabled);

		if (m_selectedButton != previousSelection)
		{
			SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
		}
	}

	// マウス操作
	const Vec2 mousePos = Cursor::Pos();
	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		if (m_buttons[i].enabled && m_buttons[i].rect.contains(mousePos))
		{
			if (m_selectedButton != static_cast<int>(i))
			{
				m_selectedButton = static_cast<int>(i);
				SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
			}
			break;
		}
	}

	// 決定
	if (KeyEnter.down() || KeySpace.down() ||
		(MouseL.down() && m_buttons[m_selectedButton].rect.contains(mousePos)))
	{
		SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
		executeButton(m_selectedButton);
	}

	// ESCでタイトルに戻る
	if (KeyEscape.down())
	{
		SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
		m_nextScene = SceneType::Title;
	}
}

void ResultScene::updateAnimations()
{
	// 背景ブロックのアニメーション
	for (auto& block : m_backgroundBlocks)
	{
		block.rotation += block.rotationSpeed * Scene::DeltaTime();
		block.bobPhase += block.bobSpeed * Scene::DeltaTime();
	}
}

void ResultScene::updateParticles()
{
	const double deltaTime = Scene::DeltaTime();

	// 既存パーティクルの更新
	for (auto it = m_particles.begin(); it != m_particles.end();)
	{
		auto& particle = *it;

		particle.position += particle.velocity * deltaTime;
		particle.velocity.y += 50.0 * deltaTime; // 重力
		particle.life -= deltaTime;
		particle.rotation += particle.rotationSpeed * deltaTime;

		if (particle.life <= 0.0 || particle.position.y > Scene::Height() + 50)
		{
			it = m_particles.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 新しいパーティクルを追加
	if (Random(0.0, 1.0) < 0.3)
	{
		createParticle(Vec2(Random(0.0, static_cast<double>(Scene::Width())), -20.0));
	}
}

void ResultScene::updateFireworks()
{
	if (m_resultData.collectedStars < 3) return;

	const double deltaTime = Scene::DeltaTime();

	for (auto& firework : m_fireworks)
	{
		firework.timer += deltaTime;

		if (!firework.active && firework.timer >= firework.delay)
		{
			firework.active = true;
			firework.exploded = false;
		}

		if (firework.active && !firework.exploded)
		{
			const double explodeTime = firework.delay + 0.5;
			if (firework.timer >= explodeTime)
			{
				firework.exploded = true;
				createFireworkExplosion(firework.position);
			}
		}
	}
}

void ResultScene::createParticle(const Vec2& position)
{
	Particle particle;
	particle.position = position;
	particle.velocity = Vec2(Random(-50.0, 50.0), Random(-100.0, -20.0));
	particle.life = Random(2.0, 4.0);
	particle.maxLife = particle.life;
	particle.rotation = Random(0.0, Math::TwoPi);
	particle.rotationSpeed = Random(-5.0, 5.0);
	particle.color = ColorF(
		Random(0.7, 1.0),
		Random(0.7, 1.0),
		Random(0.8, 1.0)
	);
	m_particles.push_back(particle);
}

void ResultScene::createFireworkExplosion(const Vec2& position)
{
	// 花火の爆発パーティクルを生成
	for (int i = 0; i < 20; ++i)
	{
		const double angle = Random(0.0, Math::TwoPi);
		const double speed = Random(100.0, 200.0);

		Particle particle;
		particle.position = position;
		particle.velocity = Vec2(std::cos(angle), std::sin(angle)) * speed;
		particle.life = Random(1.5, 3.0);
		particle.maxLife = particle.life;
		particle.rotation = Random(0.0, Math::TwoPi);
		particle.rotationSpeed = Random(-10.0, 10.0);
		particle.color = ColorF(
			Random(0.8, 1.0),
			Random(0.3, 0.8),
			Random(0.2, 0.6)
		);
		m_particles.push_back(particle);
	}
}

void ResultScene::drawBackground() const
{
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw(ColorF(0.4, 0.4, 0.4));
	}
	else
	{
		Scene::Rect().draw(ColorF(0.1, 0.1, 0.2));
	}

	// グラデーションオーバーレイ
	Scene::Rect().draw(
		Arg::top = ColorF(0.0, 0.0, 0.0, 0.3),
		Arg::bottom = ColorF(0.2, 0.1, 0.3, 0.5)
	);
}

void ResultScene::drawBackgroundBlocks() const
{
	if (!m_blockTexture) return;

	for (const auto& block : m_backgroundBlocks)
	{
		const Vec2 bobOffset(0, std::sin(block.bobPhase) * 5.0);
		const Vec2 drawPos = block.position + bobOffset;

		const ColorF blockColor = ColorF(1.0, 1.0, 1.0, block.alpha);
		const double blockSize = 64.0 * block.scale;

		m_blockTexture
			.resized(blockSize, blockSize)
			.rotated(block.rotation)
			.drawAt(drawPos, blockColor);
	}
}

void ResultScene::drawMainPanel() const
{
	if (!m_blockTexture) return;

	// メインパネルをブロックで構築
	const double blockSize = 80.0;
	const int panelWidth = 10;
	const int panelHeight = 8;
	const Vec2 panelStart = Vec2(
		Scene::Center().x - (panelWidth * blockSize) / 2,
		Scene::Center().y - (panelHeight * blockSize) / 2 - 50
	);

	// パネルの枠をブロックで描画
	for (int x = 0; x < panelWidth; ++x)
	{
		for (int y = 0; y < panelHeight; ++y)
		{
			const bool isEdge = (x == 0 || x == panelWidth - 1 || y == 0 || y == panelHeight - 1);

			if (isEdge)
			{
				const Vec2 blockPos = panelStart + Vec2(x * blockSize, y * blockSize);
				const double blockAlpha = 0.8 + std::sin(m_blockAnimationTimer + x + y) * 0.1;
				const ColorF blockColor = ColorF(0.9, 0.9, 0.9, blockAlpha);

				m_blockTexture
					.resized(blockSize, blockSize)
					.drawAt(blockPos + Vec2(blockSize / 2, blockSize / 2), blockColor);
			}
		}
	}

	// パネル内部の背景
	const RectF innerPanel = RectF(
		panelStart.x + blockSize,
		panelStart.y + blockSize,
		(panelWidth - 2) * blockSize,
		(panelHeight - 2) * blockSize
	);
	innerPanel.draw(ColorF(0.05, 0.05, 0.15, 0.9));

	// 内部にもデコレーションブロック
	const int decorBlocks = 8;
	for (int i = 0; i < decorBlocks; ++i)
	{
		const double angle = (i * Math::TwoPi / decorBlocks) + m_blockAnimationTimer * 0.5;
		const double radius = 60.0;
		const Vec2 decorPos = innerPanel.center() + Vec2(std::cos(angle), std::sin(angle)) * radius;
		const double decorAlpha = 0.2 + std::sin(m_blockAnimationTimer * 2.0 + i) * 0.1;

		m_blockTexture
			.resized(32, 32)
			.rotated(angle)
			.drawAt(decorPos, ColorF(0.8, 0.8, 1.0, decorAlpha));
	}
}

void ResultScene::drawTitle() const
{
	const String title = U"STAGE CLEAR!";
	const Vec2 titlePos = Vec2(Scene::Center().x, Scene::Center().y - 200);

	// タイトルの脈動効果
	const double pulse = 1.0 + std::sin(m_titlePulseTimer * 3.0) * 0.15;
	const Font pulseFont = Font(static_cast<int>(42 * pulse), Typeface::Bold);

	// 複数レイヤーでグロー効果
	for (int i = 4; i >= 0; --i)
	{
		const Vec2 offset = Vec2(i * 1.5, i * 1.5);
		const double alpha = (i == 0) ? 1.0 : 0.3;
		ColorF color;

		if (i == 0)
		{
			color = ColorF(1.0, 1.0, 0.8);
		}
		else
		{
			const double hue = std::fmod(m_titlePulseTimer * 0.5 + i * 0.2, 1.0);
			color = HSV(hue * 360.0, 0.6, 1.0, alpha).toColorF();
		}

		pulseFont(title).drawAt(titlePos + offset, color);
	}
}

void ResultScene::drawStageInfo() const
{
	const Vec2 infoPos = Vec2(Scene::Center().x, Scene::Center().y - 120);

	// ステージ情報を枠付きで表示
	const String stageText = U"Stage {}: {}"_fmt(
		static_cast<int>(m_resultData.clearedStage),
		m_resultData.stageName
	);

	const RectF infoRect = RectF(Arg::center = infoPos, 400, 40);
	infoRect.draw(ColorF(0.1, 0.1, 0.2, 0.8));
	infoRect.drawFrame(2.0, ColorF(0.8, 0.8, 0.6));

	m_headerFont(stageText).drawAt(infoPos, ColorF(0.9, 0.9, 0.9));

	// プレイヤー情報
	const String playerText = U"Player: {}"_fmt(m_resultData.playerColorName);
	const Vec2 playerPos = Vec2(infoPos.x, infoPos.y + 50);
	m_dataFont(playerText).drawAt(playerPos, getPlayerColorTint(m_resultData.playerColor));
}

void ResultScene::drawStarResult() const
{
	const Vec2 starSectionPos = Vec2(Scene::Center().x, Scene::Center().y - 40);

	// スターセクションタイトル
	m_headerFont(U"STARS COLLECTED").drawAt(starSectionPos, ColorF(1.0, 1.0, 0.8));

	// 星表示（ブロック風背景付き）
	const double starSize = 80.0;
	const double starSpacing = 120.0;
	const Vec2 starStartPos = Vec2(
		starSectionPos.x - (starSpacing * (m_resultData.totalStars - 1)) / 2,
		starSectionPos.y + 80
	);

	for (int i = 0; i < m_resultData.totalStars; ++i)
	{
		const Vec2 starPos = starStartPos + Vec2(i * starSpacing, 0);
		const bool collected = (i < m_resultData.collectedStars);

		// 星の背景ブロック
		if (m_blockTexture)
		{
			const double bgScale = 1.2 + std::sin(m_animationTimer * 2.0 + i) * 0.1;
			const ColorF bgColor = collected ?
				ColorF(1.0, 1.0, 0.6, 0.4) :
				ColorF(0.3, 0.3, 0.3, 0.4);

			m_blockTexture
				.resized(100 * bgScale, 100 * bgScale)
				.drawAt(starPos, bgColor);
		}

		// スターアニメーション
		double starScale = 1.0;
		if (m_showStarAnimation && collected)
		{
			const double animTime = m_animationTimer - m_starAnimationDelay - (i * 0.5);
			if (animTime > 0)
			{
				starScale = 1.0 + std::sin(animTime * 6.0) * 0.3 * std::exp(-animTime * 1.5);
			}
		}

		// 星描画
		const Texture& starTexture = collected ? m_starFilledTexture : m_starOutlineTexture;
		const ColorF starColor = collected ? getStarColor(i) : ColorF(0.3, 0.3, 0.3);

		if (starTexture)
		{
			const Size scaledSize = Size(
				static_cast<int>(starSize * starScale),
				static_cast<int>(starSize * starScale)
			);
			starTexture.resized(scaledSize).drawAt(starPos, starColor);
		}
		else
		{
			Circle(starPos, starSize * starScale * 0.4).draw(starColor);
		}

		// 収集時のキラキラエフェクト
		if (collected && m_showStarAnimation)
		{
			drawStarGlitter(starPos, i);
		}
	}

	// スターレーティング
	const String ratingText = getStarRating();
	const Vec2 ratingPos = Vec2(starSectionPos.x, starSectionPos.y + 160);

	const RectF ratingRect = RectF(Arg::center = ratingPos, 300, 35);
	ratingRect.draw(ColorF(0.1, 0.1, 0.2, 0.7));
	ratingRect.drawFrame(2.0, ColorF(1.0, 1.0, 0.6));

	m_dataFont(ratingText).drawAt(ratingPos, ColorF(1.0, 1.0, 0.8));
}

void ResultScene::drawGameStats() const
{
	const Vec2 statsPos = Vec2(Scene::Center().x, Scene::Center().y + 120);

	// 統計セクション
	const RectF statsRect = RectF(Arg::center = statsPos, 500, 80);
	statsRect.draw(ColorF(0.05, 0.05, 0.15, 0.8));
	statsRect.drawFrame(3.0, ColorF(0.6, 0.8, 0.6));

	// クリア時間
	const String timeText = U"Clear Time: {}"_fmt(formatTime(m_resultData.clearTime));
	const Vec2 timePos = Vec2(statsPos.x - 120, statsPos.y - 15);
	m_dataFont(timeText).drawAt(timePos, ColorF(0.9, 0.9, 0.9));

	// コイン数（アイコン付き）
	const String coinText = U"Coins: {}"_fmt(m_resultData.collectedCoins);
	const Vec2 coinPos = Vec2(statsPos.x + 120, statsPos.y - 15);

	if (m_coinTexture)
	{
		const Vec2 coinIconPos = Vec2(coinPos.x - 60, coinPos.y);
		m_coinTexture.resized(32, 32).drawAt(coinIconPos);
	}

	m_dataFont(coinText).drawAt(coinPos, ColorF(1.0, 1.0, 0.6));

	// パフォーマンス評価
	const String performanceText = getPerformanceRating();
	const Vec2 perfPos = Vec2(statsPos.x, statsPos.y + 25);
	m_dataFont(performanceText).drawAt(perfPos, ColorF(0.8, 1.0, 0.8));
}

void ResultScene::drawButtons() const
{
	const double time = Scene::Time();

	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		const ButtonData& button = m_buttons[i];
		const bool isSelected = (m_selectedButton == static_cast<int>(i));
		const bool isEnabled = button.enabled;

		// ボタンの色とエフェクト設定
		ColorF buttonColor = isEnabled ? ColorF(0.8, 0.8, 0.8) : ColorF(0.4, 0.4, 0.4);
		ColorF textColor = isEnabled ? ColorF(0.9, 0.9, 0.9) : ColorF(0.5, 0.5, 0.5);
		double buttonScale = 1.0;

		if (isSelected && isEnabled)
		{
			const double glow = 0.8 + 0.2 * std::sin(time * 8.0);
			buttonColor = ColorF(0.6, 0.8, 1.0, glow);
			textColor = ColorF(0.1, 0.1, 0.1);
			buttonScale = 1.05 + std::sin(time * 12.0) * 0.02;
		}

		// ボタン背景にブロック装飾
		if (m_blockTexture && isSelected)
		{
			const Vec2 decorPos1 = button.rect.center() + Vec2(-120, 0);
			const Vec2 decorPos2 = button.rect.center() + Vec2(120, 0);
			const double decorRotation = time * 2.0;

			m_blockTexture
				.resized(40, 40)
				.rotated(decorRotation)
				.drawAt(decorPos1, ColorF(0.8, 0.8, 1.0, 0.6));
			m_blockTexture
				.resized(40, 40)
				.rotated(-decorRotation)
				.drawAt(decorPos2, ColorF(0.8, 0.8, 1.0, 0.6));
		}

		// ボタン描画
		if (m_buttonTexture)
		{
			const Vec2 buttonSize = button.rect.size * buttonScale;

			if (isSelected && isEnabled)
			{
				// グロー効果
				for (int glow = 2; glow >= 0; --glow)
				{
					const double glowScale = buttonScale * (1.0 + glow * 0.05);
					const Vec2 glowSize = buttonSize * glowScale;
					const double glowAlpha = 0.3 + glow * 0.2;

					m_buttonTexture.resized(glowSize).drawAt(button.rect.center(),
						glow == 0 ? buttonColor : ColorF(0.6, 0.8, 1.0, glowAlpha));
				}
			}
			else
			{
				m_buttonTexture.resized(buttonSize).drawAt(button.rect.center(), buttonColor);
			}
		}
		else
		{
			// フォールバック描画
			const RectF buttonRect = RectF(Arg::center = button.rect.center(), button.rect.size * buttonScale);
			buttonRect.draw(ColorF(0.2, 0.2, 0.3));
			buttonRect.drawFrame(2.0, buttonColor);
		}

		// ボタンテキスト
		m_buttonFont(button.text).drawAt(button.rect.center(), textColor);
	}
}

void ResultScene::drawParticles() const
{
	for (const auto& particle : m_particles)
	{
		const double alpha = particle.life / particle.maxLife;
		const ColorF color = ColorF(
			particle.color.r,
			particle.color.g,
			particle.color.b,
			alpha * 0.8
		);

		const double size = 3.0 + std::sin(particle.rotation) * 1.0;
		Circle(particle.position, size).draw(color);

		// キラキラエフェクト
		const double sparkleSize = size * 0.7;
		Line(
			particle.position.x - sparkleSize, particle.position.y,
			particle.position.x + sparkleSize, particle.position.y
		).draw(1.0, color);
		Line(
			particle.position.x, particle.position.y - sparkleSize,
			particle.position.x, particle.position.y + sparkleSize
		).draw(1.0, color);
	}
}

void ResultScene::drawFireworks() const
{
	if (m_resultData.collectedStars < 3) return;

	for (const auto& firework : m_fireworks)
	{
		if (!firework.active) continue;

		if (!firework.exploded)
		{
			// 打ち上げ段階
			const double progress = (firework.timer - firework.delay) / 0.5;
			if (progress >= 0.0 && progress <= 1.0)
			{
				const Vec2 trailStart = firework.position + Vec2(0, 100);
				const Vec2 currentPos = trailStart.lerp(firework.position, progress);

				Circle(currentPos, 8.0).draw(ColorF(1.0, 0.8, 0.2));
				Circle(currentPos, 12.0).draw(ColorF(1.0, 0.6, 0.0, 0.3));
			}
		}
	}
}

void ResultScene::drawStarGlitter(const Vec2& starPos, int starIndex) const
{
	const double time = m_animationTimer - m_starAnimationDelay - (starIndex * 0.5);
	if (time <= 0.0) return;

	// 星の周りにキラキラエフェクト
	for (int i = 0; i < 8; ++i)
	{
		const double angle = i * Math::Pi / 4 + time * 3.0;
		const double distance = 60.0 + std::sin(time * 4.0 + i) * 15.0;
		const Vec2 glitterPos = starPos + Vec2(std::cos(angle), std::sin(angle)) * distance;

		const double glitterAlpha = 0.8 + std::sin(time * 6.0 + i) * 0.2;
		const double glitterSize = 4.0 + std::sin(time * 8.0 + i) * 2.0;

		Circle(glitterPos, glitterSize).draw(ColorF(1.0, 1.0, 0.8, glitterAlpha * 0.7));

		// 十字のキラキラ
		Line(glitterPos.x - glitterSize, glitterPos.y,
			 glitterPos.x + glitterSize, glitterPos.y)
			.draw(2.0, ColorF(1.0, 1.0, 1.0, glitterAlpha));
		Line(glitterPos.x, glitterPos.y - glitterSize,
			 glitterPos.x, glitterPos.y + glitterSize)
			.draw(2.0, ColorF(1.0, 1.0, 1.0, glitterAlpha));
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
		GameScene::setNextStageMode();
		m_nextScene = SceneType::Game;
		break;

	case ButtonAction::Retry:
		GameScene::setRetryMode();
		m_nextScene = SceneType::Game;
		break;

	case ButtonAction::BackToTitle:
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

ColorF ResultScene::getPlayerColorTint(PlayerColor color) const
{
	switch (color)
	{
	case PlayerColor::Green:  return ColorF(0.3, 1.0, 0.3);
	case PlayerColor::Pink:   return ColorF(1.0, 0.5, 0.8);
	case PlayerColor::Purple: return ColorF(0.8, 0.3, 1.0);
	case PlayerColor::Beige:  return ColorF(0.9, 0.8, 0.6);
	case PlayerColor::Yellow: return ColorF(1.0, 1.0, 0.3);
	default: return ColorF(1.0, 1.0, 1.0);
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
	case 0: return U"Keep trying! You can do better!";
	case 1: return U"Good start! Aim for more stars!";
	case 2: return U"Great work! One more star to go!";
	case 3: return U"PERFECT CLEAR! ★★★ AMAZING! ★★★";
	default: return U"Incredible performance!";
	}
}

String ResultScene::getPerformanceRating() const
{
	// 時間とスターの組み合わせで評価
	const double timeScore = m_resultData.clearTime;
	const int starScore = m_resultData.collectedStars;

	if (starScore == 3 && timeScore < 60.0)
	{
		return U"SPEED MASTER! Lightning fast perfect clear!";
	}
	else if (starScore == 3)
	{
		return U"PERFECTIONIST! All stars collected!";
	}
	else if (timeScore < 45.0)
	{
		return U"SPEED RUNNER! Blazing fast completion!";
	}
	else if (m_resultData.collectedCoins >= 50)
	{
		return U"TREASURE HUNTER! Coin collector extraordinaire!";
	}
	else if (starScore >= 2)
	{
		return U"SKILLED PLAYER! Excellent performance!";
	}
	else
	{
		return U"ADVENTURER! Ready for the next challenge!";
	}
}
