#include "HUDSystem.hpp"

HUDSystem::HUDSystem()
	: m_maxLife(6)                      // 3ハート × 2 = 6ライフ
	, m_currentLife(6)                  // 初期値は満タン
	, m_coins(0)                        // 初期コイン数
	, m_collectedStars(0)               // 初期スター数（新機能）
	, m_totalStars(3)                   // 総スター数（新機能）
	, m_currentPlayerColor(PlayerColor::Green) // デフォルトはGreen
	, m_hudPosition(30, 30)             // 左上から30ピクセル（少し余裕を持たせる）
	, m_visible(true)                   // 初期状態で表示
	, m_previousLife(6)                 // 前フレームのライフ（ダメージ検出用）
	, m_heartShakeTimer(0.0)            // ハート揺れタイマー
	, m_heartShakeIntensity(0.0)        // ハート揺れ強度
	, m_heartShakePhase(0.0)            // ハート揺れ位相
	,m_remainingFireballs(10)           //ファイアボールの数
{
}

void HUDSystem::init()
{
	loadTextures();

	// フォントの初期化
	m_numberFont = Font(24, Typeface::Bold);
}

void HUDSystem::loadTextures()
{
	// ハートテクスチャの読み込み
	m_heartTextures.full = Texture(U"Sprites/Tiles/hud_heart.png");
	m_heartTextures.half = Texture(U"Sprites/Tiles/hud_heart_half.png");
	m_heartTextures.empty = Texture(U"Sprites/Tiles/hud_heart_empty.png");

	// プレイヤーアイコンテクスチャの読み込み
	m_playerIconTextures.beige = Texture(U"Sprites/Tiles/hud_player_beige.png");
	m_playerIconTextures.green = Texture(U"Sprites/Tiles/hud_player_green.png");
	m_playerIconTextures.pink = Texture(U"Sprites/Tiles/hud_player_pink.png");
	m_playerIconTextures.purple = Texture(U"Sprites/Tiles/hud_player_purple.png");
	m_playerIconTextures.yellow = Texture(U"Sprites/Tiles/hud_player_yellow.png");

	// コインテクスチャの読み込み
	m_coinTextures.coin = Texture(U"Sprites/Tiles/hud_coin.png");

	// スターテクスチャの読み込み（新機能）
	m_starTextures.starOutline = Texture(U"UI/PNG/Yellow/star_outline_depth.png");
	m_starTextures.starFilled = Texture(U"UI/PNG/Yellow/star.png");

	// 数字テクスチャの読み込み (0-9)
	m_coinTextures.numbers.resize(10);
	for (int i = 0; i < 10; i++)
	{
		const String numberPath = U"Sprites/Tiles/hud_character_{}.png"_fmt(i);
		m_coinTextures.numbers[i] = Texture(numberPath);

		if (!m_coinTextures.numbers[i])
		{
			Print << U"Failed to load number texture: " << numberPath;
		}
	}

	// ロード失敗チェック
	if (!m_heartTextures.full) Print << U"Failed to load heart full texture";
	if (!m_heartTextures.half) Print << U"Failed to load heart half texture";
	if (!m_heartTextures.empty) Print << U"Failed to load heart empty texture";
	if (!m_coinTextures.coin) Print << U"Failed to load coin texture";
	if (!m_starTextures.starOutline) Print << U"Failed to load star outline texture";
	if (!m_starTextures.starFilled) Print << U"Failed to load star filled texture";
}

void HUDSystem::update()
{
	// 修正: ダメージ検出とハート揺れ開始
	if (m_currentLife < m_previousLife)
	{
		// ライフが減った場合、ハート揺れを開始
		m_heartShakeTimer = 0.0;
		m_heartShakeIntensity = 1.0;
		m_heartShakePhase = 0.0;

	}

	// 前フレームのライフを更新
	m_previousLife = m_currentLife;

	// ハート揺れアニメーションの更新
	if (m_heartShakeIntensity > 0.0)
	{
		m_heartShakeTimer += Scene::DeltaTime();
		m_heartShakePhase += 0.4; // 揺れの速度

		// 揺れ強度の減衰（1秒で完全に停止）
		m_heartShakeIntensity = Math::Max(0.0, 1.0 - (m_heartShakeTimer / HEART_SHAKE_DURATION));

		if (m_heartShakeIntensity <= 0.0)
		{
			m_heartShakeTimer = 0.0;
			m_heartShakePhase = 0.0;
		}
	}
}

void HUDSystem::draw() const
{
	if (!m_visible) return;

	drawHearts();
	drawPlayerIcon();
	drawCoins();
	drawStars();

	// ★ ファイアボール残数表示
	drawFireballs();
}

void HUDSystem::drawFireballs() const
{
	const Vec2 fireballPos = Vec2(m_hudPosition.x + 400, m_hudPosition.y + HEART_SIZE + ELEMENT_SPACING);

	// ファイアボールアイコン（簡易版）
	Circle(fireballPos + Vec2(20, 20), 15).draw(ColorF(1.0, 0.5, 0.0));
	Circle(fireballPos + Vec2(20, 20), 10).draw(ColorF(1.0, 0.8, 0.2));

	// 残数表示
	const String fireballText = U"FB: {}"_fmt(m_remainingFireballs);
	Font(16)(fireballText).draw(fireballPos + Vec2(50, 10), ColorF(1.0, 1.0, 1.0));
}

void HUDSystem::drawHearts() const
{
	// 最大ライフに応じてハート数を計算
	const int heartsCount = (m_maxLife + 1) / 2; // 2ライフごとに1ハート
	const Vec2 heartBasePos = m_hudPosition;

	for (int i = 0; i < heartsCount; i++)
	{
		Vec2 heartPos = heartBasePos + Vec2(i * (HEART_SIZE + 12), 0); // 12ピクセルの間隔（拡大）
		HeartState state = getHeartState(i);

		Texture heartTexture;
		switch (state)
		{
		case HeartState::Full:
			heartTexture = m_heartTextures.full;
			break;
		case HeartState::Half:
			heartTexture = m_heartTextures.half;
			break;
		case HeartState::Empty:
			heartTexture = m_heartTextures.empty;
			break;
		}

		if (heartTexture)
		{
			// ハート揺れエフェクトの計算
			Vec2 shakeOffset(0, 0);

			if (m_heartShakeIntensity > 0.0)
			{
				// 複数方向のランダムな揺れ
				const double shakeAmount = m_heartShakeIntensity * HEART_SHAKE_AMOUNT;

				// X軸の揺れ（高周波）
				shakeOffset.x = std::sin(m_heartShakePhase * 18.0 + i * 0.5) * shakeAmount;

				// Y軸の揺れ（少し低い周波数）
				shakeOffset.y = std::cos(m_heartShakePhase * 15.0 + i * 0.3) * shakeAmount * 0.7;

				// 揺れのバリエーションを各ハートで少し変える
				if (i == 1)
				{
					shakeOffset.x *= 1.2;
					shakeOffset.y *= 0.8;
				}
				else if (i == 2)
				{
					shakeOffset.x *= 0.9;
					shakeOffset.y *= 1.1;
				}
			}

			// 滑らかな拡大表示（揺れ位置を適用）
			const Vec2 finalHeartPos = heartPos + shakeOffset;
			const RectF heartRect(finalHeartPos, HEART_SIZE, HEART_SIZE);

			heartTexture.resized(HEART_SIZE, HEART_SIZE).draw(finalHeartPos);

			// ダメージ時の追加エフェクト（オプション）
			if (m_heartShakeIntensity > 0.5 && state != HeartState::Empty)
			{
				// 赤い光のエフェクト
				const ColorF glowColor = ColorF(1.0, 0.5, 0.5, 50.0 * m_heartShakeIntensity / 255.0);
				const Vec2 glowPos = finalHeartPos - Vec2(2, 2);
				const Size glowSize(HEART_SIZE + 4, HEART_SIZE + 4);

				heartTexture.resized(glowSize).draw(glowPos, glowColor);
			}
		}
		else
		{
			// フォールバック：テクスチャがない場合
			const ColorF fallbackColor = (state == HeartState::Full) ? ColorF(1.0, 0.0, 0.0) :
				(state == HeartState::Half) ? ColorF(1.0, 0.5, 0.0) :
				ColorF(0.3, 0.3, 0.3);
			Circle(heartPos + Vec2(HEART_SIZE / 2, HEART_SIZE / 2), HEART_SIZE / 2).draw(fallbackColor);
		}
	}
}

void HUDSystem::drawPlayerIcon() const
{
	const Vec2 iconPos = m_hudPosition + Vec2(0, HEART_SIZE + ELEMENT_SPACING); // ハートの下に配置

	const Texture iconTexture = getPlayerIconTexture();
	if (iconTexture)
	{
		// 滑らかな拡大表示
		iconTexture.resized(PLAYER_ICON_SIZE, PLAYER_ICON_SIZE).draw(iconPos);
	}
	else
	{
		// フォールバック：色付きの円
		const ColorF fallbackColor = [&]() {
			switch (m_currentPlayerColor)
			{
			case PlayerColor::Green:  return ColorF(0.3, 1.0, 0.3);
			case PlayerColor::Pink:   return ColorF(1.0, 0.5, 0.8);
			case PlayerColor::Purple: return ColorF(0.8, 0.3, 1.0);
			case PlayerColor::Beige:  return ColorF(0.9, 0.8, 0.6);
			case PlayerColor::Yellow: return ColorF(1.0, 1.0, 0.3);
			default: return ColorF(1.0, 1.0, 1.0);
			}
			}();

		Circle(iconPos + Vec2(PLAYER_ICON_SIZE / 2, PLAYER_ICON_SIZE / 2), PLAYER_ICON_SIZE / 2).draw(fallbackColor);
	}
}

void HUDSystem::drawCoins() const
{
	const Vec2 coinStartPos = m_hudPosition + Vec2(PLAYER_ICON_SIZE + ELEMENT_SPACING, HEART_SIZE + ELEMENT_SPACING + (PLAYER_ICON_SIZE - COIN_ICON_SIZE) / 2); // 中央揃え

	// コインアイコンを描画（拡大表示）
	if (m_coinTextures.coin)
	{
		m_coinTextures.coin.resized(COIN_ICON_SIZE, COIN_ICON_SIZE).draw(coinStartPos);
	}
	else
	{
		// フォールバック：黄色い円
		Circle(coinStartPos + Vec2(COIN_ICON_SIZE / 2, COIN_ICON_SIZE / 2), COIN_ICON_SIZE / 2).draw(ColorF(1.0, 1.0, 0.0));
	}

	// コイン数を描画
	const Vec2 numberPos = coinStartPos + Vec2(COIN_ICON_SIZE + 12, (COIN_ICON_SIZE - NUMBER_SIZE) / 2); // 中央揃え
	drawNumber(m_coins, numberPos);
}

void HUDSystem::drawNumber(int number, const Vec2& position) const
{
	// 数値を文字列に変換
	const String numberStr = ToString(number);

	// 最低でも1桁は表示（0の場合）
	const String displayStr = numberStr.isEmpty() ? U"0" : numberStr;

	// 各桁を描画（拡大表示）
	for (size_t i = 0; i < displayStr.length(); i++)
	{
		const char32 digitChar = displayStr[i];
		const int digit = digitChar - U'0'; // 文字を数値に変換

		if (digit >= 0 && digit <= 9 && digit < static_cast<int>(m_coinTextures.numbers.size()) && m_coinTextures.numbers[digit])
		{
			const Vec2 digitPos = position + Vec2(i * (NUMBER_SIZE - 6), 0); // 数字間の間隔を調整（拡大版）

			// 滑らかな拡大表示
			m_coinTextures.numbers[digit].resized(NUMBER_SIZE, NUMBER_SIZE).draw(digitPos);
		}
		else
		{
			// フォールバック：フォントで描画
			const Vec2 digitPos = position + Vec2(i * (NUMBER_SIZE - 6), 0);
			m_numberFont(String(1, digitChar)).draw(digitPos, ColorF(1.0, 1.0, 1.0));
		}
	}
}

void HUDSystem::drawStars() const
{
	const Vec2 starStartPos = m_hudPosition + Vec2(
		PLAYER_ICON_SIZE + ELEMENT_SPACING + COIN_ICON_SIZE + 60 + ELEMENT_SPACING, // コインの右側
		HEART_SIZE + ELEMENT_SPACING + (PLAYER_ICON_SIZE - STAR_SIZE) / 2 // 中央揃え
	);

	for (int i = 0; i < m_totalStars; i++)
	{
		const Vec2 starPos = starStartPos + Vec2(i * (STAR_SIZE + 8), 0); // 8ピクセル間隔

		// スターのテクスチャ選択
		const Texture starTexture = (i < m_collectedStars) ? m_starTextures.starFilled : m_starTextures.starOutline;

		if (starTexture)
		{
			// 滑らかな拡大表示
			starTexture.resized(STAR_SIZE, STAR_SIZE).draw(starPos);
		}
		else
		{
			// フォールバック：星型
			const ColorF starColor = (i < m_collectedStars) ? ColorF(1.0, 1.0, 0.0) : ColorF(0.5, 0.5, 0.5);

			// 簡易的な星の描画
			const Vec2 center = starPos + Vec2(STAR_SIZE / 2, STAR_SIZE / 2);
			const double radius = STAR_SIZE / 3.0;

			Array<Vec2> starPoints;
			for (int j = 0; j < 10; ++j)
			{
				const double angle = j * Math::TwoPi / 10.0 - Math::HalfPi;
				const double r = (j % 2 == 0) ? radius : radius * 0.5;
				starPoints.push_back(center + Vec2(std::cos(angle), std::sin(angle)) * r);
			}

			Polygon(starPoints).draw(starColor);
		}
	}
}

HUDSystem::HeartState HUDSystem::getHeartState(int heartIndex) const
{
	// 各ハートは2ライフ分を表す
	const int heartLife = m_currentLife - (heartIndex * 2);

	if (heartLife >= 2)
	{
		return HeartState::Full;
	}
	else if (heartLife == 1)
	{
		return HeartState::Half;
	}
	else
	{
		return HeartState::Empty;
	}
}

Texture HUDSystem::getPlayerIconTexture() const
{
	switch (m_currentPlayerColor)
	{
	case PlayerColor::Beige:  return m_playerIconTextures.beige;
	case PlayerColor::Green:  return m_playerIconTextures.green;
	case PlayerColor::Pink:   return m_playerIconTextures.pink;
	case PlayerColor::Purple: return m_playerIconTextures.purple;
	case PlayerColor::Yellow: return m_playerIconTextures.yellow;
	default: return m_playerIconTextures.green;
	}
}

String HUDSystem::getCharacterColorName(PlayerColor color) const
{
	switch (color)
	{
	case PlayerColor::Beige:  return U"beige";
	case PlayerColor::Green:  return U"green";
	case PlayerColor::Pink:   return U"pink";
	case PlayerColor::Purple: return U"purple";
	case PlayerColor::Yellow: return U"yellow";
	default: return U"green";
	}
}

void HUDSystem::setCurrentLife(int newLife)
{
	const int oldLife = m_currentLife;
	m_currentLife = Math::Clamp((float)newLife, 0, m_maxLife);

	// ライフが減った場合は即座にアニメーション開始
	if (m_currentLife < oldLife)
	{
		m_heartShakeTimer = 0.0;
		m_heartShakeIntensity = 1.0;
		m_heartShakePhase = 0.0;
	}

	// previousLifeも更新して重複検出を防ぐ
	m_previousLife = m_currentLife;
}

void HUDSystem::addLife(int amount)
{
	m_currentLife = Math::Min((float)m_currentLife + amount, m_maxLife);
}

void HUDSystem::subtractLife(int amount)
{
	setCurrentLife(m_currentLife - amount);
}

void HUDSystem::setPlayerCharacter(PlayerColor color)
{
	m_currentPlayerColor = color;
}

void HUDSystem::setPlayerCharacter(int characterIndex)
{
	switch (characterIndex)
	{
	case 0: m_currentPlayerColor = PlayerColor::Beige; break;
	case 1: m_currentPlayerColor = PlayerColor::Green; break;
	case 2: m_currentPlayerColor = PlayerColor::Pink; break;
	case 3: m_currentPlayerColor = PlayerColor::Purple; break;
	case 4: m_currentPlayerColor = PlayerColor::Yellow; break;
	default: m_currentPlayerColor = PlayerColor::Green; break;
	}
}


void HUDSystem::notifyDamage()
{
	// 強制的にハート揺れアニメーションを開始
	m_heartShakeTimer = 0.0;
	m_heartShakeIntensity = 1.0;
	m_heartShakePhase = 0.0;
}
