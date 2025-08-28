#include "CoinSystem.hpp"
#include "SoundManager.hpp"

CoinSystem::CoinSystem()
	: m_collectedCoinsCount(0)
{
}

void CoinSystem::init()
{
	loadTextures();
	m_collectedCoinsCount = 0;
}

void CoinSystem::loadTextures()
{
	// コインテクスチャを読み込み
	m_coinTexture = Texture(U"Sprites/Tiles/hud_coin.png");

	// きらめき効果用（オプション）
	// m_sparkleTexture = Texture(U"Sprites/Effects/sparkle.png");

	if (!m_coinTexture)
	{
		Print << U"Failed to load coin texture";
	}
}

void CoinSystem::update(const Player* player, const Vec2& hudCoinPosition)
{
	if (!player) return;

	// プレイヤーの位置を取得
	const Vec2 playerPosition = player->getPosition();

	// 全てのコインを更新
	for (auto it = m_coins.begin(); it != m_coins.end();)
	{
		if ((*it)->active)
		{
			updateCoin(**it, playerPosition, hudCoinPosition);
			++it;
		}
		else
		{
			// 非アクティブなコインを削除
			it = m_coins.erase(it);
		}
	}
}

void CoinSystem::updateCoin(Coin& coin, const Vec2& playerPosition, const Vec2& hudCoinPosition)
{
	// プレイヤーとコインの距離を計算
	const double playerDistance = coin.position.distanceFrom(playerPosition);

	// 状態に応じた処理
	switch (coin.state)
	{
	case CoinState::Idle:
		// 引き寄せ範囲内かチェック（プレイヤーとの距離で判定）
		if (playerDistance <= ATTRACT_DISTANCE)
		{
			coin.state = CoinState::Attracting;
			coin.attractTimer = 0.0;
		}
		break;

	case CoinState::Attracting:
		// 収集範囲内かチェック（プレイヤーとの距離で判定）
		if (playerDistance <= COLLECTION_DISTANCE)
		{
			coin.state = CoinState::Collected;
			coin.attractTimer = 0.0;

		
			m_collectedCoinsCount++;

			// コイン収集音を再生
			SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_COIN);
		}
		break;

	case CoinState::Collected:
		// 収集アニメーション（ゆっくりと消え去りをかけて）
		coin.attractTimer += Scene::DeltaTime();

		// HUDに到達したかチェック（距離が十分近くなったら削除）
		const double hudDistance = coin.position.distanceFrom(hudCoinPosition);

		if (coin.attractTimer >= 1.2 || hudDistance <= 20.0) // 1.2秒経過または20px以内に到達
		{
			coin.active = false;
		}
		break;
	}

	// 物理更新
	updateCoinPhysics(coin, hudCoinPosition);

	// アニメーション更新
	updateCoinAnimation(coin);
}

void CoinSystem::updateCoinPhysics(Coin& coin, const Vec2& hudCoinPosition)
{
	switch (coin.state)
	{
	case CoinState::Idle:
		// 通常の浮遊動作
		coin.position.y = coin.originalPosition.y + std::sin(coin.bobPhase) * BOB_AMPLITUDE;
		break;

	case CoinState::Collected:
	{
		// 収集後の演出（HUDのコインアイコンに向かって加速移動）
		const double progress = coin.attractTimer / 1.2; // 1.2秒でゆっくり完了
		const Vec2 direction = hudCoinPosition - coin.position;
		const double distance = direction.length();

		if (distance > 0.1)
		{
			// 正規化されたベクトル
			const Vec2 normalizedDir = direction.normalized();

			// 加速的に移動（HUDに近づくほど速く）
			const double speed = 15.0 * (1.0 + progress * 3.0);

			coin.position += normalizedDir * speed * Scene::DeltaTime();
		}

		// フェードアウトと拡大効果
		coin.alpha = Math::Max(0.0, 1.0 - progress * 0.9); // よりキビキビとフェードアウト
		coin.scale = 1.0 + progress * 0.3; // ゆっくりと拡大
	}
	break;
	}
}

void CoinSystem::updateCoinAnimation(Coin& coin)
{
	// 浮遊アニメーション
	coin.bobPhase += BOB_SPEED;
	if (coin.bobPhase >= Math::TwoPi)
	{
		coin.bobPhase -= Math::TwoPi;
	}

	// 回転アニメーション
	coin.animationTimer += ROTATE_SPEED;
	if (coin.animationTimer >= Math::TwoPi)
	{
		coin.animationTimer -= Math::TwoPi;
	}

	// 引き寄せ時のスケールアニメーション
	if (coin.state == CoinState::Attracting)
	{
		const double pulseScale = 1.0 + std::sin(coin.animationTimer * 4.0) * 0.1;
		coin.scale = pulseScale;
	}
}

void CoinSystem::draw(const Vec2& cameraOffset) const
{
	if (!m_coinTexture) return;

	for (const auto& coin : m_coins)
	{
		if (coin->active)
		{
			drawCoin(*coin, cameraOffset);

			// 収集エフェクト
			if (coin->state == CoinState::Collected)
			{
				drawCollectionEffect(*coin, cameraOffset);
			}
		}
	}
}

void CoinSystem::drawCoin(const Coin& coin, const Vec2& cameraOffset) const
{
	// 画面座標に変換
	const Vec2 screenPos = coin.position - cameraOffset;

	// 画面外なら描画しない（最適化）
	if (screenPos.x < -COIN_SIZE || screenPos.x > Scene::Width() + COIN_SIZE) return;

	// スケールと透明度を適用
	const double coinSize = COIN_SIZE * coin.scale;
	const Vec2 offset(coinSize / 2, coinSize / 2);

	// 透明度適用
	const ColorF color = ColorF(1.0, 1.0, 1.0, coin.alpha);

	// 回転効果（簡易版：横方向のスケール変更で回転を表現）
	const double rotationScale = std::abs(std::cos(coin.animationTimer));
	const double rotatedWidth = coinSize * (0.3 + rotationScale * 0.7);

	// コイン描画（64pxに拡大）
	const RectF coinRect(screenPos - offset, rotatedWidth, coinSize);
	m_coinTexture.resized(coinRect.size).draw(coinRect.pos, color);

	// デバッグ用：コインの当たり判定範囲とHUDターゲットを表示
#ifdef _DEBUG
	// 収集範囲（緑）
	Circle(screenPos, COLLECTION_DISTANCE).drawFrame(2.0, ColorF(0.0, 1.0, 0.0, 0.3));
	// 引き寄せ範囲（黄）
	Circle(screenPos, ATTRACT_DISTANCE).drawFrame(2.0, ColorF(1.0, 1.0, 0.0, 0.3));

	// HUDターゲット位置を表示（赤い×印）
	const Vec2 hudTargetScreen = Vec2(30 + 80 + 20 + 48 / 2, 30 + 64 + 20 + 48 / 2);
	Line(hudTargetScreen.x - 10, hudTargetScreen.y - 10, hudTargetScreen.x + 10, hudTargetScreen.y + 10).draw(2.0, ColorF(1.0, 0.0, 0.0));
	Line(hudTargetScreen.x - 10, hudTargetScreen.y + 10, hudTargetScreen.x + 10, hudTargetScreen.y - 10).draw(2.0, ColorF(1.0, 0.0, 0.0));

	// HUDまでの距離を表示
	if (coin.state == CoinState::Collected)
	{
		const Vec2 hudWorldPos = hudTargetScreen + cameraOffset;
		const double hudDistance = coin.position.distanceFrom(hudWorldPos);

		const String distanceText = U"Dist: {}"_fmt(static_cast<int>(hudDistance));
		Font(16)(distanceText).draw(screenPos + Vec2(-30, -60), ColorF(1.0, 1.0, 0.0));
	}

	// ワールド座標での方向ベクトルを表示
	if (coin.state == CoinState::Attracting || coin.state == CoinState::Collected)
	{
		const Vec2 hudWorldPos = hudTargetScreen + cameraOffset;
		const Vec2 direction = hudWorldPos - coin.position;

		// 方向ベクトルを矢印で表示
		const Vec2 endPos = screenPos + direction * 0.1;
		Line(screenPos, endPos).draw(2.0, ColorF(1.0, 0.0, 1.0));
	}
#endif
}

void CoinSystem::drawCollectionEffect(const Coin& coin, const Vec2& cameraOffset) const
{
	const Vec2 screenPos = coin.position - cameraOffset;

	// きらめき効果（ゆっくりとした演出）
	const double progress = coin.attractTimer / 1.2; // 1.2秒でゆっくり
	const double sparkleSize = 25 + progress * 35; // ゆっくりと大きくなる
	const double sparkleAlpha = 160 * (1.0 - progress) / 255.0;

	if (sparkleAlpha > 0.0)
	{
		// 複数の円できらめき効果（ゆっくりと回転）
		for (int i = 0; i < 4; i++)
		{
			const double angle = progress * 4.0 + i * Math::HalfPi; // ゆっくり回転
			const Vec2 sparklePos = screenPos + Vec2(std::cos(angle), std::sin(angle)) * 18.0;

			const double currentSparkleSize = sparkleSize / (i + 1);
			const ColorF sparkleColor = ColorF(1.0, 1.0, 0.6, sparkleAlpha);

			Circle(sparklePos, currentSparkleSize).draw(sparkleColor);
		}

		// 中央に追加のきらめき
		const ColorF centralColor = ColorF(1.0, 0.84, 0.0, sparkleAlpha);
		Circle(screenPos, sparkleSize / 2).draw(centralColor);
	}
}

void CoinSystem::addCoin(const Vec2& position)
{
	m_coins.push_back(std::make_unique<Coin>(position));
}

void CoinSystem::clearAllCoins()
{
	m_coins.clear();
}

void CoinSystem::generateCoinsForStage(StageNumber stageNumber)
{
	switch (stageNumber)
	{
	case StageNumber::Stage1:
		generateCoinsForGrassStage();
		break;
	case StageNumber::Stage2:
		generateCoinsForSandStage();
		break;
	case StageNumber::Stage3:
		generateCoinsForPurpleStage();
		break;
	case StageNumber::Stage4:
		generateCoinsForSnowStage();
		break;
	case StageNumber::Stage5:
		generateCoinsForStoneStage();
		break;
	case StageNumber::Stage6:
		generateCoinsForDirtStage();
		break;
	}
}

void CoinSystem::generateCoinsForGrassStage()
{
	clearAllCoins();

	// コインの配置（64の倍数座標、ブロックと重複しない空中位置）
	Array<Vec2> grassCoins = {
		// 空中の浮遊コイン（ブロックやプラットフォームを避けた配置）
		Vec2(4 * 64, 11 * 64),   // X: 256, Y: 704
		Vec2(8 * 64, 9 * 64),    // X: 512, Y: 576
		Vec2(11 * 64, 7 * 64),   // X: 704, Y: 448
		Vec2(14 * 64, 5 * 64),   // X: 896, Y: 320

		Vec2(17 * 64, 8 * 64),   // X: 1088, Y: 512
		Vec2(22 * 64, 6 * 64),   // X: 1408, Y: 384
		Vec2(27 * 64, 4 * 64),   // X: 1728, Y: 256
		Vec2(30 * 64, 7 * 64),   // X: 1920, Y: 448

		Vec2(33 * 64, 5 * 64),   // X: 2112, Y: 320
		Vec2(36 * 64, 9 * 64),   // X: 2304, Y: 576
		Vec2(41 * 64, 4 * 64),   // X: 2624, Y: 256
		Vec2(46 * 64, 8 * 64),   // X: 2944, Y: 512

		Vec2(50 * 64, 6 * 64),   // X: 3200, Y: 384
		Vec2(52 * 64, 4 * 64),   // X: 3328, Y: 256
		Vec2(55 * 64, 7 * 64),   // X: 3520, Y: 448
		Vec2(57 * 64, 5 * 64),   // X: 3648, Y: 320

		Vec2(60 * 64, 8 * 64),   // X: 3840, Y: 512
		Vec2(63 * 64, 6 * 64),   // X: 4032, Y: 384
		Vec2(65 * 64, 4 * 64),   // X: 4160, Y: 256
		Vec2(69 * 64, 7 * 64),   // X: 4416, Y: 448

		// チャレンジコイン（非常に高い位置）
		Vec2(24 * 64, 3 * 64),   // X: 1536, Y: 192
		Vec2(43 * 64, 3 * 64),   // X: 2752, Y: 192
		Vec2(58 * 64, 3 * 64),   // X: 3712, Y: 192
		Vec2(72 * 64, 3 * 64)    // X: 4608, Y: 192
	};

	for (const auto& pos : grassCoins)
	{
		addCoin(pos);
	}
}

void CoinSystem::generateCoinsForSandStage()
{
	clearAllCoins();

	// 砂漠ステージの地形を避けた配置
	Array<Vec2> sandCoins = {
		// オアシス近辺とピラミッド間隔
		Vec2(400, 350), Vec2(700, 300), Vec2(1000, 250),    // オアシス近辺
		Vec2(1400, 280), Vec2(1800, 200), Vec2(2200, 320),  // 砂丘の間
		Vec2(2700, 180), Vec2(3100, 150), Vec2(3500, 220),  // ピラミッド近辺
		Vec2(4000, 260), Vec2(4400, 180), Vec2(4800, 300),  // 砂の谷
		Vec2(5300, 200), Vec2(5700, 150), Vec2(6100, 240),  // 終盤の砂丘

		// 砂漠の宝（隠されたコイン）
		Vec2(1100, 120), Vec2(2900, 80), Vec2(4600, 100), Vec2(6400, 60)
	};

	for (const auto& pos : sandCoins)
	{
		addCoin(pos);
	}
}

void CoinSystem::generateCoinsForPurpleStage()
{
	clearAllCoins();

	// 魔法ステージの浮遊島を避けた魔法的配置
	Array<Vec2> purpleCoins = {
		// 浮遊する魔法のコイン
		Vec2(500, 320), Vec2(900, 280), Vec2(1300, 220),    // 魔法の島近辺
		Vec2(1800, 200), Vec2(2200, 160), Vec2(2600, 260),  // 魔法の空付近
		Vec2(3200, 180), Vec2(3600, 120), Vec2(4000, 240),  // 高い魔法の島
		Vec2(4500, 200), Vec2(4900, 140), Vec2(5300, 280),  // 魔法の道
		Vec2(5800, 160), Vec2(6200, 100), Vec2(6600, 180),  // 最終エリア

		// 隠された魔法のコイン
		Vec2(1200, 80), Vec2(3000, 40), Vec2(4800, 60), Vec2(6400, 20)
	};

	for (const auto& pos : purpleCoins)
	{
		addCoin(pos);
	}
}

void CoinSystem::generateCoinsForSnowStage()
{
	clearAllCoins();

	// 雪山の地形を避けた安全な配置
	Array<Vec2> snowCoins = {
		// 雪原と氷の足場の間
		Vec2(500, 380), Vec2(900, 320), Vec2(1300, 260),    // 山麓
		Vec2(1800, 280), Vec2(2200, 200), Vec2(2600, 240),  // 氷の足場近辺
		Vec2(3200, 180), Vec2(3600, 140), Vec2(4000, 220),  // 山頂付近
		Vec2(4500, 260), Vec2(4900, 180), Vec2(5300, 300),  // 氷だまり近辺
		Vec2(5800, 160), Vec2(6200, 200), Vec2(6600, 120),  // 雪原の秘密

		// 氷河の宝
		Vec2(1400, 100), Vec2(3400, 60), Vec2(5100, 80), Vec2(6800, 40)
	};

	for (const auto& pos : snowCoins)
	{
		addCoin(pos);
	}
}

void CoinSystem::generateCoinsForStoneStage()
{
	clearAllCoins();

	// 石ステージの地形を避けた配置
	Array<Vec2> stoneCoins = {
		// 岩の隙間の安全な場所
		Vec2(500, 380), Vec2(800, 320), Vec2(1100, 280),    // 初期エリア
		Vec2(1600, 250), Vec2(2000, 200), Vec2(2400, 300),  // 中間の岩場
		Vec2(3000, 220), Vec2(3500, 160), Vec2(3900, 240),  // 高い岩場
		Vec2(4400, 280), Vec2(4900, 200), Vec2(5300, 180),  // 石の道
		Vec2(5800, 300), Vec2(6200, 150), Vec2(6600, 200),  // 終盤エリア

		// ボーナスコイン（高所）
		Vec2(1200, 150), Vec2(2800, 100), Vec2(4200, 120), Vec2(5600, 90)
	};

	for (const auto& pos : stoneCoins)
	{
		addCoin(pos);
	}
}

void CoinSystem::generateCoinsForDirtStage()
{
	clearAllCoins();

	// 地下ステージの洞窟を避けた配置
	Array<Vec2> dirtCoins = {
		// 洞窟の安全な通路
		Vec2(400, 300), Vec2(700, 250), Vec2(1000, 200),    // 洞窟入口
		Vec2(1400, 180), Vec2(1800, 230), Vec2(2200, 160),  // 地下通路
		Vec2(2700, 200), Vec2(3100, 140), Vec2(3500, 180),  // 深い洞窟
		Vec2(4000, 220), Vec2(4400, 160), Vec2(4800, 240),  // 地下の隠し部屋
		Vec2(5300, 180), Vec2(5700, 120), Vec2(6100, 200),  // 洞窟の奥

		// 地下の宝
		Vec2(1200, 100), Vec2(2900, 60), Vec2(4600, 80), Vec2(6400, 40)
	};

	for (const auto& pos : dirtCoins)
	{
		addCoin(pos);
	}
}
