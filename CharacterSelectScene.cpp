#include "CharacterSelectScene.hpp"

// 静的変数の定義
PlayerColor CharacterSelectScene::s_selectedPlayerColor = PlayerColor::Green;

CharacterSelectScene::CharacterSelectScene()
	: m_selectedCharacter(0)
	, m_selectionTimer(0.0)
	, m_selectButtonHovered(false)
	, m_backButtonHovered(false)
	, m_animationTimer(0.0)
	, m_nextScene(none)
{
}

void CharacterSelectScene::init()
{
	// テクスチャの読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");
	m_buttonTexture = Texture(U"UI/PNG/Yellow/button_rectangle_depth_gradient.png");

	loadPlayerTextures();

	// フォントの初期化
	m_titleFont = Font(36, Typeface::Bold);
	m_labelFont = Font(24);
	m_buttonFont = Font(20, Typeface::Bold);

	// UI要素の設定
	setupCharacters();
	setupButtons();

	// 初期状態
	m_selectedCharacter = 0;
	m_selectionTimer = 0.0;
	m_selectButtonHovered = false;
	m_backButtonHovered = false;
	m_animationTimer = 0.0;
	m_nextScene = none;

	// スパークルエフェクトの初期化
	m_sparklePositions.clear();
	m_sparkleTimers.clear();
}

void CharacterSelectScene::update()
{
	m_animationTimer += Scene::DeltaTime();
	m_selectionTimer += Scene::DeltaTime();

	updateInput();
	updateAnimations();
}

void CharacterSelectScene::draw() const
{
	drawBackground();
	drawTitle();

	// 五角形のトレースパスを先に描画
	drawPentagonTracePath();

	drawCharacters();
	drawSparkles();
	drawButtons();
	drawInstructions();
}

Optional<SceneType> CharacterSelectScene::getNextScene() const
{
	return m_nextScene;
}

void CharacterSelectScene::cleanup()
{
	// 必要に応じてクリーンアップ
}

PlayerColor CharacterSelectScene::getSelectedPlayerColor()
{
	return s_selectedPlayerColor;
}

void CharacterSelectScene::setSelectedPlayerColor(PlayerColor color)
{
	s_selectedPlayerColor = color;
}

void CharacterSelectScene::loadPlayerTextures()
{
	m_playerTextures.clear();

	// プレイヤーテクスチャの読み込み
	m_playerTextures.push_back(Texture(U"Sprites/Tiles/hud_player_green.png"));
	m_playerTextures.push_back(Texture(U"Sprites/Tiles/hud_player_pink.png"));
	m_playerTextures.push_back(Texture(U"Sprites/Tiles/hud_player_purple.png"));
	m_playerTextures.push_back(Texture(U"Sprites/Tiles/hud_player_beige.png"));
	m_playerTextures.push_back(Texture(U"Sprites/Tiles/hud_player_yellow.png"));
}

void CharacterSelectScene::setupCharacters()
{
	m_characters.clear();

	const double characterSize = 120.0;

	// 各キャラクターの設定
	Array<PlayerColor> colors = {
		PlayerColor::Green,
		PlayerColor::Pink,
		PlayerColor::Purple,
		PlayerColor::Beige,
		PlayerColor::Yellow
	};

	// 五角形の中心と半径
	const Vec2 pentagonCenter = Scene::Center();
	const double pentagonRadius = 200.0;  // 大きな五角形の半径

	for (size_t i = 0; i < colors.size(); ++i)
	{
		CharacterData character;
		character.color = colors[i];
		character.name = getColorName(colors[i]);

		if (i < m_playerTextures.size())
		{
			character.texture = m_playerTextures[i];
		}

		// 五角形の頂点位置を計算（上向きから開始）
		const double angle = (i * Math::TwoPi / 5.0) - Math::HalfPi;  // -90度から開始（上向き）
		character.displayPos = pentagonCenter + Vec2(
			std::cos(angle) * pentagonRadius,
			std::sin(angle) * pentagonRadius
		);

		character.rect = RectF(
			character.displayPos.x - characterSize / 2,
			character.displayPos.y - characterSize / 2,
			characterSize,
			characterSize
		);

		m_characters.push_back(character);
	}
}

void CharacterSelectScene::setupButtons()
{
	const double buttonWidth = 140.0;
	const double buttonHeight = 50.0;
	const double buttonY = Scene::Center().y + 150;

	// SELECTボタン
	m_selectButtonRect = RectF(
		Scene::Center().x - buttonWidth - 10,
		buttonY +100,
		buttonWidth,
		buttonHeight
	);

	// BACKボタン
	m_backButtonRect = RectF(
		Scene::Center().x + 10,
		buttonY +100,
		buttonWidth,
		buttonHeight
	);
}

void CharacterSelectScene::updateInput()
{
	// マウスホバー検出
	const Vec2 mousePos = Cursor::Pos();
	m_selectButtonHovered = m_selectButtonRect.contains(mousePos);
	m_backButtonHovered = m_backButtonRect.contains(mousePos);

	// キャラクター選択（キーボード）
	if (KeyLeft.down() || KeyA.down())
	{
		m_selectedCharacter = (m_selectedCharacter - 1 + static_cast<int>(m_characters.size())) % static_cast<int>(m_characters.size());
		m_selectionTimer = 0.0;
		createSparkleEffect(m_characters[m_selectedCharacter].displayPos);
	}
	if (KeyRight.down() || KeyD.down())
	{
		m_selectedCharacter = (m_selectedCharacter + 1) % static_cast<int>(m_characters.size());
		m_selectionTimer = 0.0;
		createSparkleEffect(m_characters[m_selectedCharacter].displayPos);
	}

	// マウスでキャラクター選択
	for (size_t i = 0; i < m_characters.size(); ++i)
	{
		if (m_characters[i].rect.contains(mousePos))
		{
			if (m_selectedCharacter != static_cast<int>(i))
			{
				m_selectedCharacter = static_cast<int>(i);
				m_selectionTimer = 0.0;
				createSparkleEffect(m_characters[i].displayPos);
			}
			break;
		}
	}

	// 選択決定
	if (KeyEnter.down() || KeySpace.down() ||
		(MouseL.down() && m_selectButtonHovered) ||
		(MouseL.down() && m_characters[m_selectedCharacter].rect.contains(mousePos)))
	{
		// 選択されたキャラクターを保存
		s_selectedPlayerColor = m_characters[m_selectedCharacter].color;
		m_nextScene = SceneType::Game;
	}

	// 戻る
	if (KeyEscape.down() || (MouseL.down() && m_backButtonHovered))
	{
		m_nextScene = SceneType::Title;
	}
}

void CharacterSelectScene::updateAnimations()
{
	// スパークルエフェクトの更新
	for (size_t i = 0; i < m_sparkleTimers.size(); )
	{
		m_sparkleTimers[i] -= Scene::DeltaTime();
		if (m_sparkleTimers[i] <= 0.0)
		{
			m_sparklePositions.erase(m_sparklePositions.begin() + i);
			m_sparkleTimers.erase(m_sparkleTimers.begin() + i);
		}
		else
		{
			++i;
		}
	}
}

void CharacterSelectScene::drawBackground() const
{
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw(ColorF(0.8, 0.8, 0.8));
	}
	else
	{
		Scene::Rect().draw(ColorF(0.2, 0.3, 0.4));
	}

	// グラデーションオーバーレイ
	Scene::Rect().draw(Arg::top = ColorF(0.1, 0.2, 0.3, 0.3), Arg::bottom = ColorF(0.3, 0.2, 0.4, 0.3));
}

void CharacterSelectScene::drawTitle() const
{
	const String title = U"SELECT CHARACTER";
	const Vec2 titlePos = Vec2(Scene::Center().x, 100);

	// タイトルの光る効果
	for (int i = 2; i >= 0; --i)
	{
		const Vec2 offset = Vec2(i * 2, i * 2);
		const double alpha = (i == 0) ? 1.0 : 0.4;
		const ColorF color = (i == 0) ? ColorF(1.0, 1.0, 0.8) : ColorF(0.8, 0.8, 1.0, alpha);

		m_titleFont(title).drawAt(titlePos + offset, color);
	}
}

void CharacterSelectScene::drawCharacters() const
{
	for (size_t i = 0; i < m_characters.size(); ++i)
	{
		const bool isSelected = (m_selectedCharacter == static_cast<int>(i));
		drawCharacter(m_characters[i], isSelected);
	}
}

void CharacterSelectScene::drawCharacter(const CharacterData& character, bool isSelected) const
{
	const double time = m_animationTimer;

	// 五角形のトレースアニメーション
	const Vec2 pentagonMovement = getPentagonTraceMovement(time);
	const Vec2 pos = character.displayPos + pentagonMovement;

	// 選択エフェクト
	if (isSelected)
	{
		// 選択リング
		const double ringRadius = 70.0 + std::sin(time * 6.0) * 5.0;
		const ColorF ringColor = getColorTint(character.color);
		Circle(pos, ringRadius).drawFrame(4.0, ColorF(ringColor.r, ringColor.g, ringColor.b, 0.8));
		Circle(pos, ringRadius + 8.0).drawFrame(2.0, ColorF(1.0, 1.0, 1.0, 0.6));

		// バウンスエフェクト
		const double bounce = std::sin(m_selectionTimer * 8.0) * 3.0;
		const Vec2 bouncePos = pos + Vec2(0, bounce);

		// キャラクター描画（選択時）
		if (character.texture)
		{
			const double scale = 1.2 + std::sin(time * 4.0) * 0.08;
			const double rotation = std::sin(time * 2.0) * 0.1;
			character.texture.scaled(scale).rotated(rotation).drawAt(bouncePos);
		}
		else
		{
			// フォールバック
			Circle(bouncePos, 50).draw(getColorTint(character.color));
		}

		// 名前表示（選択時）
		const Vec2 namePos = Vec2(pos.x, pos.y + 90);
		const ColorF nameColor = getColorTint(character.color);
		m_labelFont(character.name).drawAt(namePos + Vec2(1, 1), ColorF(0.0, 0.0, 0.0, 0.5));
		m_labelFont(character.name).drawAt(namePos, nameColor);
	}
	else
	{
		// 通常状態のキャラクター描画
		if (character.texture)
		{
			const double gentleRotation = std::sin(time * 1.5 + static_cast<int>(character.color) * 0.5) * 0.05;
			const double scale = 0.9 + std::sin(time * 2.0 + static_cast<int>(character.color) * 0.3) * 0.05;
			character.texture.scaled(scale).rotated(gentleRotation).drawAt(pos, ColorF(0.8, 0.8, 0.8));
		}
		else
		{
			// フォールバック
			Circle(pos, 40).draw(ColorF(getColorTint(character.color).r, getColorTint(character.color).g, getColorTint(character.color).b, 0.8));
		}
	}
}

void CharacterSelectScene::drawButtons() const
{
	// SELECTボタン
	ColorF selectColor = m_selectButtonHovered ? ColorF(1.0, 1.0, 0.8) : ColorF(0.8, 0.8, 0.8);
	if (m_buttonTexture)
	{
		if (m_selectButtonHovered)
		{
			m_buttonTexture.resized(m_selectButtonRect.size * 1.05).drawAt(m_selectButtonRect.center(), ColorF(1.0, 1.0, 0.6, 0.3));
		}
		m_buttonTexture.resized(m_selectButtonRect.size).drawAt(m_selectButtonRect.center(), selectColor);
	}
	else
	{
		m_selectButtonRect.draw(ColorF(0.3, 0.3, 0.3));
		m_selectButtonRect.drawFrame(2.0, selectColor);
	}

	ColorF selectTextColor = m_selectButtonHovered ? ColorF(0.1, 0.1, 0.1) : ColorF(0.9, 0.9, 0.9);
	m_buttonFont(U"SELECT").drawAt(m_selectButtonRect.center(), selectTextColor);

	// BACKボタン
	ColorF backColor = m_backButtonHovered ? ColorF(1.0, 1.0, 0.8) : ColorF(0.8, 0.8, 0.8);
	if (m_buttonTexture)
	{
		if (m_backButtonHovered)
		{
			m_buttonTexture.resized(m_backButtonRect.size * 1.05).drawAt(m_backButtonRect.center(), ColorF(1.0, 1.0, 0.6, 0.3));
		}
		m_buttonTexture.resized(m_backButtonRect.size).drawAt(m_backButtonRect.center(), backColor);
	}
	else
	{
		m_backButtonRect.draw(ColorF(0.3, 0.3, 0.3));
		m_backButtonRect.drawFrame(2.0, backColor);
	}

	ColorF backTextColor = m_backButtonHovered ? ColorF(0.1, 0.1, 0.1) : ColorF(0.9, 0.9, 0.9);
	m_buttonFont(U"BACK").drawAt(m_backButtonRect.center(), backTextColor);
}

void CharacterSelectScene::drawSparkles() const
{
	for (size_t i = 0; i < m_sparklePositions.size(); ++i)
	{
		const Vec2 pos = m_sparklePositions[i];
		const double timer = m_sparkleTimers[i];
		const double alpha = timer / 1.0;  // 1秒で消える
		const double size = 3.0 + std::sin(m_animationTimer * 8.0 + i) * 1.0;

		if (alpha > 0.0)
		{
			// 星型スパークル
			const ColorF sparkleColor = ColorF(1.0, 1.0, 0.8, alpha);
			Line(pos.x - size, pos.y, pos.x + size, pos.y).draw(2.0, sparkleColor);
			Line(pos.x, pos.y - size, pos.x, pos.y + size).draw(2.0, sparkleColor);
			Circle(pos, size * 0.5).draw(ColorF(1.0, 1.0, 1.0, alpha * 0.8));
		}
	}
}

void CharacterSelectScene::drawInstructions() const
{
	const String instructions = U"←→ or A/D: Select  ENTER/SPACE: Confirm  ESC: Back";
	const Vec2 instructionsPos = Vec2(Scene::Center().x, Scene::Height() - 30);

	m_labelFont(instructions).drawAt(instructionsPos + Vec2(1, 1), ColorF(0.0, 0.0, 0.0, 0.5));
	m_labelFont(instructions).drawAt(instructionsPos, ColorF(0.8, 0.8, 0.8));
}

String CharacterSelectScene::getColorName(PlayerColor color) const
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

ColorF CharacterSelectScene::getColorTint(PlayerColor color) const
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

void CharacterSelectScene::createSparkleEffect(const Vec2& position)
{
	// 複数のスパークルを作成
	for (int i = 0; i < 5; ++i)
	{
		const Vec2 offset = Vec2(
			Random(-30.0, 30.0),
			Random(-30.0, 30.0)
		);
		m_sparklePositions.push_back(position + offset);
		m_sparkleTimers.push_back(1.0);  // 1秒間表示
	}
}

Vec2 CharacterSelectScene::getPentagonTraceMovement(double time) const
{
	// 全キャラクターが同じ五角形をトレースする
	const double radius = 30.0;   // トレース半径
	const double speed = 0.6;     // 回転速度（ゆっくり）

	// 五角形の辺上を移動する計算
	const int numPoints = 5;
	const double angleStep = Math::TwoPi / numPoints;
	const double currentAngle = time * speed;

	// 現在の時間での五角形上の位置を計算
	const double segmentLength = 1.0 / numPoints;
	const double normalizedTime = std::fmod(currentAngle / Math::TwoPi, 1.0);
	const int currentSegment = static_cast<int>(normalizedTime / segmentLength);
	const double segmentProgress = (normalizedTime - currentSegment * segmentLength) / segmentLength;

	// 現在の辺の開始点と終了点
	const double startAngle = currentSegment * angleStep - Math::HalfPi;
	const double endAngle = (currentSegment + 1) * angleStep - Math::HalfPi;

	const Vec2 startPoint = Vec2(
		std::cos(startAngle) * radius,
		std::sin(startAngle) * radius
	);
	const Vec2 endPoint = Vec2(
		std::cos(endAngle) * radius,
		std::sin(endAngle) * radius
	);

	// 辺上の位置を線形補間で計算
	return startPoint.lerp(endPoint, segmentProgress);
}

void CharacterSelectScene::drawPentagonTracePath() const
{
	// トレースパスの描画
	const Vec2 center = Scene::Center();
	const double radius = 30.0;
	const int numPoints = 5;
	const double angleStep = Math::TwoPi / numPoints;
	const double time = m_animationTimer;

	Array<Vec2> points;
	for (int i = 0; i < numPoints; ++i)
	{
		const double angle = i * angleStep - Math::HalfPi;
		points.push_back(center + Vec2(
			std::cos(angle) * radius,
			std::sin(angle) * radius
		));
	}

	// 五角形の輪郭を描画（点線風）
	const double dashLength = 8.0;
	for (int i = 0; i < numPoints; ++i)
	{
		const int nextIndex = (i + 1) % numPoints;
		const Vec2 start = points[i];
		const Vec2 end = points[nextIndex];
		const Vec2 direction = (end - start).normalized();
		const double lineLength = (end - start).length();

		// 点線を描画
		for (double d = 0; d < lineLength; d += dashLength * 2)
		{
			const Vec2 dashStart = start + direction * d;
			const Vec2 dashEnd = start + direction * Math::Min(d + dashLength, lineLength);
			const double alpha = 0.3 + 0.2 * std::sin(time * 3.0 + d * 0.1);
			Line(dashStart, dashEnd).draw(2.0, ColorF(1.0, 1.0, 0.8, alpha));
		}
	}

	// 現在のトレース位置にハイライト
	const Vec2 currentTracePos = center + getPentagonTraceMovement(time);
	const double glowRadius = 8.0 + std::sin(time * 8.0) * 3.0;
	Circle(currentTracePos, glowRadius).draw(ColorF(1.0, 1.0, 0.6, 0.4));
	Circle(currentTracePos, glowRadius * 0.6).draw(ColorF(1.0, 1.0, 1.0, 0.8));
}
