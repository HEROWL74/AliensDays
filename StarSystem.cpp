#include "StarSystem.hpp"
#include "SoundManager.hpp"

StarSystem::StarSystem()
	: m_collectedStarsCount(0)
{
}

void StarSystem::init()
{
	loadTextures();
	m_collectedStarsCount = 0;
}

void StarSystem::loadTextures()
{
	// 星テクスチャを読み込み
	m_starTexture = Texture(U"UI/PNG/Yellow/star.png");
	
	if (!m_starTexture)
	{
		Print << U"Failed to load star texture";
	}
}

void StarSystem::update(const Player* player)
{
	if (!player) return;

	// プレイヤーの位置を取得
	const Vec2 playerPosition = player->getPosition();

	// 全ての星を更新（逆順でイテレート、削除対応）
	for (auto it = m_stars.begin(); it != m_stars.end();)
	{
		if ((*it)->active)
		{
			updateStar(**it, playerPosition);

			// 非アクティブになった星を完全に削除
			if (!(*it)->active)
			{
				it = m_stars.erase(it); // イテレータを更新
			}
			else
			{
				++it; // 次の要素へ
			}
		}
		else
		{
			// 既に非アクティブの星も削除
			it = m_stars.erase(it);
		}
	}
}

void StarSystem::updateStar(Star& star, const Vec2& playerPosition)
{
	// プレイヤーと星の距離を計算
	const double playerDistance = star.position.distanceFrom(playerPosition);

	// 状態に応じた処理
	switch (star.state)
	{
	case StarState::Idle:
		// 引き寄せ範囲内かチェック
		if (playerDistance <= ATTRACT_DISTANCE)
		{
			star.state = StarState::Attracting;
			star.attractTimer = 0.0;
		}
		break;

	case StarState::Attracting:
		// 収集範囲内かチェック
		if (playerDistance <= COLLECTION_DISTANCE)
		{
			star.state = StarState::Collected;
			star.attractTimer = 0.0;
			star.collected = true;
			m_collectedStarsCount++;

			// スター収集音を再生（コインと同じ音を使用）
			SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_COIN);

			// 収集時の初期エフェクト設定
			star.collectionPhase = 0.0;
			star.burstIntensity = 1.0;
			star.sparkleCount = 12; // 放射する光の数
		}
		break;

	case StarState::Collected:
		// 収集アニメーション
		star.attractTimer += Scene::DeltaTime();
		star.collectionPhase += Scene::DeltaTime();

		// アニメーション完了後に確実に非アクティブ化
		if (star.attractTimer >= SPARKLE_DURATION)
		{
			star.active = false; // 非アクティブ化
			star.alpha = 0.0;    // 完全透明
			star.scale = 0.0;    // サイズ0
			return; // 以降の処理をスキップ
		}
		break;
	}

	// アクティブな星のみ物理・アニメーション更新
	if (star.active)
	{
		updateStarPhysics(star);
		updateStarAnimation(star);
	}
}

void StarSystem::updateStarPhysics(Star& star)
{
	switch (star.state)
	{
	case StarState::Idle:
	case StarState::Attracting:
		// 浮遊効果
		star.position.y = star.originalPosition.y + std::sin(star.bobPhase) * BOB_AMPLITUDE;
		break;

	case StarState::Collected:
		// 3回点滅アニメーション
		const double progress = star.attractTimer / SPARKLE_DURATION;

		// 最終段階での完全消失チェック
		if (progress >= 1.0)
		{
			star.active = false;
			star.alpha = 0.0;
			star.scale = 0.0;
			star.burstIntensity = 0.0;
			return;
		}

		// 点滅計算（3回）
		const double blinkPhase = progress * (Math::Pi * 3.0); // 3π ≈ 9.42 (3回の点滅)
		const int blinkCount = static_cast<int>(blinkPhase / Math::Pi); // 完了した点滅回数

		if (blinkCount < 3)
		{
			// 点滅中（0～3回）
			star.alpha = 0.3 + 0.7 * std::abs(std::sin(blinkPhase));
		}
		else
		{
			// 3回点滅完了後は徐々にフェード
			const double fadeProgress = Math::Clamp((progress - 0.67) / 0.33, 0.0, 1.0); // 残り33%でフェード
			star.alpha = 1.0 - fadeProgress;

			// フェード完了で即座に非アクティブ
			if (fadeProgress >= 1.0)
			{
				star.active = false;
				star.alpha = 0.0;
				return;
			}
		}

		// サイズは一定（点滅のみ）
		star.scale = 1.0;

		// パーティクル用の強度計算
		star.burstIntensity = Math::Max(0.0, 1.0 - progress);

		break;
	}
}

void StarSystem::updateStarAnimation(Star& star)
{
	// 浮遊アニメーション
	star.bobPhase += BOB_SPEED;
	if (star.bobPhase >= Math::TwoPi)
	{
		star.bobPhase -= Math::TwoPi;
	}

	// 回転アニメーション
	star.rotation += ROTATE_SPEED;
	if (star.rotation >= Math::TwoPi)
	{
		star.rotation -= Math::TwoPi;
	}

	// 引き寄せ時のパルス効果
	if (star.state == StarState::Attracting)
	{
		const double pulseScale = 1.0 + std::sin(star.animationTimer * 6.0) * 0.15;
		star.scale = pulseScale;
	}

	star.animationTimer += Scene::DeltaTime();
}

void StarSystem::draw(const Vec2& cameraOffset) const
{
	if (!m_starTexture) return;

	for (const auto& star : m_stars)
	{
		// アクティブかつ透明度がある星のみ描画
		if (star->active && star->alpha > 0.0)
		{
			drawStar(*star, cameraOffset);

			// 収集エフェクト（バースト強度がある場合のみ）
			if (star->state == StarState::Collected && star->burstIntensity > 0.0)
			{
				drawCollectionEffect(*star, cameraOffset);
			}
		}
	}
}

void StarSystem::drawStar(const Star& star, const Vec2& cameraOffset) const
{
	// 画面座標に変換
	const Vec2 screenPos = star.position - cameraOffset;

	// 画面外なら描画しない
	if (screenPos.x < -STAR_SIZE || screenPos.x > Scene::Width() + STAR_SIZE) return;

	// スケールと透明度を適用
	const double starSize = STAR_SIZE * star.scale;
	const Vec2 offset(starSize / 2, starSize / 2);

	// 透明度設定
	const ColorF color = ColorF(1.0, 1.0, 1.0, star.alpha);

	// 回転を考慮した描画（簡易版）
	const double rotationScale = std::abs(std::sin(star.rotation)) * 0.2 + 0.8;
	const double rotatedSize = starSize * rotationScale;
	const Vec2 rotateOffset((starSize - rotatedSize) / 2, (starSize - rotatedSize) / 2);

	// 星描画
	const RectF starRect(screenPos - offset + rotateOffset, rotatedSize, rotatedSize);
	m_starTexture.resized(starRect.size).draw(starRect.pos, color);
}

void StarSystem::drawCollectionEffect(const Star& star, const Vec2& cameraOffset) const
{
	const Vec2 screenPos = star.position - cameraOffset;
	drawCollectionBurst(star, screenPos);
}

// 新機能：3回点滅とCircleパーティクル
void StarSystem::drawCollectionBurst(const Star& star, const Vec2& screenPos) const
{
	if (star.burstIntensity <= 0.0) return;

	const double progress = star.collectionPhase / SPARKLE_DURATION;

	// 1. 中央の光る円（脈動効果）
	const double pulseScale = 1.0 + std::sin(progress * 20.0) * 0.3;
	const double centralSize = 25.0 * pulseScale * star.burstIntensity;

	const ColorF centralColor = ColorF(1.0, 1.0, 0.4, 150.0 * star.burstIntensity / 255.0);
	Circle(screenPos, centralSize).draw(centralColor);

	// 2. 外側に広がる円（リング効果）
	for (int ring = 0; ring < 4; ring++)
	{
		const double ringProgress = progress - ring * 0.1; // 時間差で広がる
		if (ringProgress > 0.0)
		{
			const double ringRadius = ringProgress * 80.0 + ring * 15.0;
			const double ringAlpha = (80.0 * star.burstIntensity / (ring + 1)) / 255.0;

			const ColorF ringColor = ColorF(
				1.0,
				(220 - ring * 30) / 255.0,
				(50 + ring * 40) / 255.0,
				ringAlpha
			);
			Circle(screenPos, ringRadius).drawFrame(2.0, ringColor);
		}
	}

	// 3. 放射状に飛び散るパーティクル（円）
	const int particleCount = 8;
	for (int i = 0; i < particleCount; i++)
	{
		const double angle = i * (Math::TwoPi / particleCount) + progress * 3.0;
		const double distance = progress * 70.0 + 15.0;

		const Vec2 particlePos = screenPos + Vec2(std::cos(angle), std::sin(angle)) * distance;

		// パーティクルサイズ（時間経過で小さくなる）
		const double particleSize = 8.0 * star.burstIntensity;

		const ColorF particleColor = ColorF(1.0, 1.0, 0.8, 120.0 * star.burstIntensity / 255.0);
		Circle(particlePos, particleSize).draw(particleColor);
	}

	// 4. ランダムな小さなキラキラ（円）
	if (progress < 0.7)
	{
		for (int s = 0; s < 12; s++)
		{
			// 疑似ランダム計算（sin/cosを使用）
			const double randomAngle = std::sin(s * 2.3 + progress * 5.0) * Math::TwoPi;
			const double randomDistance = (std::cos(s * 1.7 + progress * 3.0) * 0.5 + 0.5) * 50.0 + 20.0;

			const Vec2 sparklePos = screenPos + Vec2(std::cos(randomAngle), std::sin(randomAngle)) * randomDistance;

			// 小さなキラキラサイズ
			const double sparkleSize = 3.0 + std::sin(s + progress * 8.0) * 2.0;

			const ColorF sparkleColor = ColorF(1.0, 1.0, 0.6, 100.0 * star.burstIntensity / 255.0);
			Circle(sparklePos, sparkleSize).draw(sparkleColor);
		}
	}

	// 5. 収束する光の粒子
	if (progress > 0.3)
	{
		const int convergenceCount = 6;
		for (int c = 0; c < convergenceCount; c++)
		{
			const double convergenceProgress = (progress - 0.3) / 0.7;
			const double startDistance = 60.0;
			const double currentDistance = startDistance * (1.0 - convergenceProgress);

			const double angle = c * (Math::TwoPi / convergenceCount) + progress * 2.0;
			const Vec2 convPos = screenPos + Vec2(std::cos(angle), std::sin(angle)) * currentDistance;

			const double convSize = 5.0 * (1.0 - convergenceProgress);

			const ColorF convColor = ColorF(1.0, 0.8, 0.4, 80.0 * star.burstIntensity / 255.0);
			Circle(convPos, convSize).draw(convColor);
		}
	}
}

void StarSystem::addStar(const Vec2& position)
{
	m_stars.push_back(std::make_unique<Star>(position));
}

void StarSystem::clearAllStars()
{
	m_stars.clear();
}


void StarSystem::generateStarsForStage(StageNumber stageNumber)
{
	

	switch (stageNumber)
	{
	case StageNumber::Stage1:
		generateStarsForGrassStage();
		break;
	case StageNumber::Stage2:
		generateStarsForSandStage();
		break;
	case StageNumber::Stage3:
		generateStarsForPurpleStage();
		break;
	case StageNumber::Stage4:
		generateStarsForSnowStage();
		break;
	case StageNumber::Stage5:
		generateStarsForStoneStage();
		break;
	case StageNumber::Stage6:
		generateStarsForDirtStage();
		break;
	default:
		Print << U"Warning: Unknown stage number: " << static_cast<int>(stageNumber);
		break;
	}

	
}

void StarSystem::generateStarsForGrassStage()
{
	clearAllStars();

	addStar(Vec2(1600, 180));  // 星1: 高いジャンプが必要な場所
	addStar(Vec2(3008, 250));  // 星2: 狭い足場の上
	addStar(Vec2(4416, 200));  // 星3: 最高点、最も取りづらい
	
}

void StarSystem::generateStarsForSandStage()
{
	clearAllStars();
	// 同じ構造、砂漠テクスチャ
	addStar(Vec2(384, 300));   // 星1: 左エリア上空
	addStar(Vec2(1536, 200));  // 星2: 中央エリア上空
	addStar(Vec2(3840, 180));  // 星3: 右エリア上空
	
}

void StarSystem::generateStarsForPurpleStage()
{
	clearAllStars();
	// 同じ構造、魔法テクスチャ
	addStar(Vec2(384, 300));   // 星1: 左エリア上空
	addStar(Vec2(1536, 200));  // 星2: 中央エリア上空
	addStar(Vec2(3840, 180));  // 星3: 右エリア上空
	
}

void StarSystem::generateStarsForSnowStage()
{
	clearAllStars();
	// 同じ構造、雪テクスチャ
	addStar(Vec2(384, 300));   // 星1: 左エリア上空
	addStar(Vec2(1536, 200));  // 星2: 中央エリア上空
	addStar(Vec2(3840, 180));  // 星3: 右エリア上空
	
}

void StarSystem::generateStarsForStoneStage()
{
	clearAllStars();
	// 同じ構造、石テクスチャ
	addStar(Vec2(384, 300));   // 星1: 左エリア上空
	addStar(Vec2(1536, 200));  // 星2: 中央エリア上空
	addStar(Vec2(3840, 180));  // 星3: 右エリア上空
	
}

void StarSystem::generateStarsForDirtStage()
{
	clearAllStars();
	// 同じ構造、地下テクスチャ
	addStar(Vec2(384, 300));   // 星1: 左エリア上空
	addStar(Vec2(1536, 200));  // 星2: 中央エリア上空
	addStar(Vec2(3840, 180));  // 星3: 右エリア上空
	
}
