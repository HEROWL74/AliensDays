#include "GameScene.hpp"
#include "CharacterSelectScene.hpp"  // プレイヤーカラー取得のため
#include "../Sound/SoundManager.hpp"
#include "../Systems/CollisionSystem.hpp"
#include "../Core/SceneFactory.hpp"

namespace {
	const bool registered = [] {
		SceneFactory::registerScene(SceneType::Game, [] {
			return std::make_unique<GameScene>();
		});
		return true;
		}();
}

// 静的変数の定義
StageNumber GameScene::s_nextStageNumber = StageNumber::Stage1;
StageNumber GameScene::s_gameOverStage = StageNumber::Stage1;  // 追加
int GameScene::s_resultStars = 0;
int GameScene::s_resultCoins = 0;
double GameScene::s_resultTime = 0.0;
PlayerColor GameScene::s_resultPlayerColor = PlayerColor::Green;
bool GameScene::s_shouldLoadNextStage = false;
bool GameScene::s_shouldRetryStage = false;

GameScene::GameScene(StageNumber stage)
	: m_nextScene(none)
	, m_gameTime(0.0)
	, m_player(nullptr)
	, m_stage(nullptr)
	, m_currentStageNumber(stage)
	, m_goalReached(false)
	, m_goalTimer(0.0)
	, m_isLastStage(false)
	, m_fromResultScene(false)
{
}

void GameScene::init()
{
	// 背景画像の読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");

	// 黒い炎テクスチャの読み込み
	m_blackFireTexture = Texture(U"Sprites/BlackFire.png");
	if (!m_blackFireTexture)
	{
		Print << U"Failed to load BlackFire.png";
	}


	// フォントの初期化
	m_gameFont = Font(24);

	// ゲームBGMを開始
	SoundManager::GetInstance().playBGM(SoundManager::SoundType::BGM_GAME);

	// シーン遷移フラグを必ずリセット
	m_nextScene = none;

	// リザルトシーンからの復帰処理
	StageNumber targetStage = m_currentStageNumber;

	if (s_shouldLoadNextStage && static_cast<int>(s_nextStageNumber) <= 6)
	{
		targetStage = s_nextStageNumber;
		s_shouldLoadNextStage = false;
	}
	else if (s_shouldRetryStage)
	{
		targetStage = s_gameOverStage;
		s_shouldRetryStage = false;
	}

	// ステージの読み込み
	loadStage(targetStage);

	// プレイヤーオブジェクトを完全に破棄して再作成
	if (m_player)
	{
		m_player.reset();
	}
	m_player = nullptr;

	// 選択されたプレイヤーカラーを取得してPlayerオブジェクトを新規作成
	const PlayerColor selectedColor = CharacterSelectScene::getSelectedPlayerColor();
	// ★ プレイヤーの初期位置を1ブロック基準で設定
	const double BLOCK_SIZE = 64.0;

	// ★ プレイヤーの初期位置を地面の上に設定（安全な位置）
	// 地面のY座標は 13 * 64 = 832、プレイヤーの中心を地面から32px上に配置
	const Vec2 startPosition = Vec2(3.0 * BLOCK_SIZE, 12.5 * BLOCK_SIZE); // 地面の上に立つ位置

	m_player = std::make_unique<Player>(selectedColor, startPosition);

	// HUDシステムの初期化
	m_hudSystem = std::make_unique<HUDSystem>();
	m_hudSystem->init();
	m_hudSystem->setPlayerCharacter(selectedColor);

	// プレイヤーの特性に応じてライフを設定
	if (m_player)
	{
		const int maxLife = m_player->getMaxLife();
		m_hudSystem->setMaxLife(maxLife);
		m_hudSystem->setCurrentLife(maxLife);
	}

	m_collisionSystem = std::make_unique<CollisionSystem>();

	// 収集システムの初期化
	m_coinSystem = std::make_unique<CoinSystem>();
	m_coinSystem->init();
	m_coinSystem->generateCoinsForStage(m_currentStageNumber);

	m_starSystem = std::make_unique<StarSystem>();
	m_starSystem->init();
	m_starSystem->generateStarsForStage(m_currentStageNumber);

	//BlockSystemの初期化
	m_blockSystem = std::make_unique<BlockSystem>();
	if (m_blockSystem) {
		m_blockSystem->init();
		m_blockSystem->generateBlocksForStage(m_currentStageNumber);

		//ヒップドロップ破壊時のシェーダーエフェクトコールバックを設定
		m_blockSystem->setHipDropDestructionCallback([this](const Vec2& position) {
			if (m_shaderEffects && m_stage)
			{
				// ワールド座標をスクリーン座標に変換してShockWaveエフェクトを発動
				const Vec2 screenPos = m_stage->worldToScreenPosition(position);
				m_shaderEffects->triggerShockwave(position, m_stage->getCameraOffset());
			}
		});
	}

	// 敵の初期化
	initEnemies();

	m_shaderEffects = std::make_unique<ShaderEffects>();
	m_shaderEffects->init();

	// 昼夜システムの初期化
	m_dayNightSystem = std::make_unique<DayNightSystem>();
	m_dayNightSystem->init();

	// ゲーム状態の完全初期化
	m_gameTime = 0.0;
	m_nextScene = none;
	m_goalReached = false;
	m_goalTimer = 0.0;

	// 最終ステージかチェック
	m_isLastStage = (m_currentStageNumber == StageNumber::Stage6);
}

void GameScene::update()
{
	// ゲーム時間の更新
	m_gameTime += Scene::DeltaTime();

	// 既にシーン遷移が決定している場合は早期リターン
	if (m_nextScene != none)
	{
		return;
	}

	// プレイヤーが存在しない場合は何もしない
	if (!m_player)
	{
		return;
	}

	// ライフが0になったらプレイヤー爆散開始
	if (m_hudSystem && m_hudSystem->getCurrentLife() <= 0 && !m_player->isExploding() && !m_player->isDead())
	{
		m_player->startExplosion();
	}

	// プレイヤーが死亡状態になったらゲームオーバー
	if (m_player && m_player->isDead())
	{
		s_gameOverStage = m_currentStageNumber;
		s_resultPlayerColor = CharacterSelectScene::getSelectedPlayerColor();
		s_resultStars = m_starSystem ? m_starSystem->getCollectedStarsCount() : 0;

		int totalCoins = (m_coinSystem ? m_coinSystem->getCollectedCoinsCount() : 0) +
			(m_blockSystem ? m_blockSystem->getCoinsFromBlocks() : 0);
		s_resultCoins = totalCoins;
		s_resultTime = m_gameTime;

		m_nextScene = SceneType::GameOver;
		return;
	}

	if (m_shaderEffects)
	{
		m_shaderEffects->update(Scene::DeltaTime());
	}

	if (m_dayNightSystem)
	{
		m_dayNightSystem->update();
	}

	// ゴール判定（プレイヤー更新前にチェック）
	updateGoalCheck();

	if (m_goalReached)
	{
		return;
	}

	// ★ プレイヤーの更新のみ実行（衝突判定は内部で処理）
	if (m_player)
	{
		m_player->update();
	}

	// ★ プレイヤー更新後にヒップドロップ着地判定をチェック
	if (m_player && m_player->hasJustLandedFromHipDrop())
	{
		if (m_shaderEffects && m_stage)
		{
			const Vec2 playerPos = m_player->getPosition();
			m_shaderEffects->triggerShockwave(playerPos, m_stage->getCameraOffset());
		}
		m_player->clearHipDropLandedFlag();
	}

	if (!m_player || !m_player->isExploding())
	{
		// BlockSystemの相互作用を衝突判定前に処理
		updateBlockSystemInteractions();

		// 統一衝突判定システム（物理的な位置調整）
		updatePlayerCollisionsUnified();
		updateCollectionSystems();
		m_hudSystem->update();
		updateHUDWithCollectedItems();
		updateTotalCoinsFromBlocks();
		updateEnemies();
		updatePlayerEnemyCollision();
		updateEnemyStageCollision();
		updateFireballEnemyCollision();
		updateFireballDestructionEffects();
	}
	else
	{
		// 爆散中でも継続する更新
		updateEnemies();
		updateEnemyStageCollision();
		updateFireballDestructionEffects();
	}

	// ステージの更新（カメラ追従）
	if (m_stage && m_player)
	{
		m_stage->update(m_player->getPosition());
	}

	//プレイヤーがダメージを受けたときの色収差
	if (m_player && m_player->getCurrentState() == PlayerState::Hit)
	{
		if (m_shaderEffects)
		{
			m_shaderEffects->enableChromatic(true);
			m_shaderEffects->setChromaticIntensity(0.8f);
		}
	}
	else if (m_shaderEffects)
	{
		m_shaderEffects->enableChromatic(false);
	}

#ifdef _DEBUG
	// デバッグ用ステージ切り替え
	if (Key1.down()) loadStage(StageNumber::Stage1);
	if (Key2.down()) loadStage(StageNumber::Stage2);
	if (Key3.down()) loadStage(StageNumber::Stage3);
	if (Key4.down()) loadStage(StageNumber::Stage4);
	if (Key5.down()) loadStage(StageNumber::Stage5);
	if (Key6.down()) loadStage(StageNumber::Stage6);
#endif

	// ESCキーでタイトルに戻る（爆散中でない場合のみ）
	if (KeyEscape.down() && (!m_player || !m_player->isExploding()))
	{
		m_nextScene = SceneType::Title;
	}

	// Rキーでキャラクター選択に戻る（爆散中でない場合のみ）
	if (KeyR.down() && (!m_player || !m_player->isExploding()))
	{
		m_nextScene = SceneType::CharacterSelect;
	}
}

void GameScene::updateGoalCheck()
{
	if (!m_player || !m_stage || m_goalReached) return;

	if (m_stage->hasGoal())
	{
		const Vec2 playerPos = m_player->getPosition();
		const double halfWidth = 64.0;
		const double halfHeight = 64.0;
		const RectF playerRect(playerPos.x - halfWidth, playerPos.y - halfHeight, halfWidth * 2, halfHeight * 2);

		if (m_stage->checkGoalCollision(playerRect))
		{
			m_goalReached = true;
			m_goalTimer = 0.0;

			// プレイヤーを停止
			if (m_player)
			{
				m_player->setVelocity(Vec2::Zero());
			}

			// ★ 修正: ゴール到達処理を即座に実行
			handleGoalReached();
		}
	}
}

void GameScene::updateCollectionSystems()
{
	if (!m_player) return;

	const Vec2 hudCoinPosition = Vec2(30 + 80 + 20 + 48 / 2, 30 + 64 + 20 + 48 / 2);

	if (m_coinSystem)
	{
		m_coinSystem->update(m_player.get(), hudCoinPosition);
	}

	if (m_starSystem)
	{
		// スター収集前の数を記録
		int previousStars = m_starSystem->getCollectedStarsCount();

		m_starSystem->update(m_player.get());

		// スターが収集されたら昼夜システムに通知
		int currentStars = m_starSystem->getCollectedStarsCount();
		if (currentStars > previousStars && m_dayNightSystem)
		{
			m_dayNightSystem->onStarCollected();
		}
	}
}

void GameScene::updateHUDWithCollectedItems()
{
	if (!m_hudSystem) return;

	// 既存のコード...
	if (m_coinSystem)
	{
		m_hudSystem->setCoins(m_coinSystem->getCollectedCoinsCount());
	}

	if (m_starSystem)
	{
		m_hudSystem->setCollectedStars(m_starSystem->getCollectedStarsCount());
		m_hudSystem->setTotalStars(3);
	}

	// ★ ファイアボール残数を更新
	if (m_player)
	{
		m_hudSystem->setFireballCount(m_player->getRemainingFireballs());
	}
}

void GameScene::handleGoalReached()
{
	// ★ 修正: クリア音を再生
	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_COIN);

	// ゴール時にグローエフェクト
	if (m_shaderEffects)
	{
		m_shaderEffects->enableGlow(true);
	}

	// リザルトデータを設定
	s_resultPlayerColor = CharacterSelectScene::getSelectedPlayerColor();
	s_resultStars = m_starSystem ? m_starSystem->getCollectedStarsCount() : 0;

	// ★ 修正: ブロックからのコインも含める
	int totalCoins = (m_coinSystem ? m_coinSystem->getCollectedCoinsCount() : 0) +
		(m_blockSystem ? m_blockSystem->getCoinsFromBlocks() : 0);
	s_resultCoins = totalCoins;
	s_resultTime = m_gameTime;

	// 次のステージ番号を設定
	if (static_cast<int>(m_currentStageNumber) < 6)
	{
		s_nextStageNumber = static_cast<StageNumber>(static_cast<int>(m_currentStageNumber) + 1);
	}
	else
	{
		s_nextStageNumber = m_currentStageNumber; // 最終ステージの場合
	}

	// ★ 修正: 即座にリザルト画面へ遷移
	m_nextScene = SceneType::Result;
}

void GameScene::draw() const
{
	if (m_shaderEffects)
	{
		// RenderTextureへの描画開始
		m_shaderEffects->beginCapture();

		{
			// ScopedRenderTarget2Dを使用してRenderTextureに描画
			const ScopedRenderTarget2D target(m_shaderEffects->getRenderTarget());

			// === 既存の描画コードはそのまま ===
			if (m_stage)
			{
				m_stage->draw();
			}
			else
			{
				Scene::Rect().draw(ColorF(0.2, 0.4, 0.6));
			}

			if (m_stage)
			{
				const Vec2 cameraOffset = m_stage->getCameraOffset();

				if (m_coinSystem)
				{
					m_coinSystem->draw(cameraOffset);
				}

				if (m_starSystem)
				{
					m_starSystem->draw(cameraOffset);
				}

				if (m_blockSystem)
				{
					m_blockSystem->draw(cameraOffset);
				}
			}

			drawEnemies();

			// 変身エフェクトを敵の後に描画（より前面に表示）
			if (m_dayNightSystem && m_stage)
			{
				m_dayNightSystem->drawTransformEffects(m_stage->getCameraOffset());
			}
			drawFireballDestructionEffects();

			if (m_player && m_stage)
			{
				drawPlayerFireballs();
			}

			// プレイヤーの描画
			if (m_player && m_stage)
			{
				const Vec2 playerWorldPos = m_player->getPosition();
				const Vec2 playerScreenPos = m_stage->worldToScreenPosition(playerWorldPos);

				if (m_player->isExploding())
				{
					const Vec2 originalPos = m_player->getPosition();
					const_cast<Player*>(m_player.get())->setPosition(playerScreenPos);
					m_player->draw();
					const_cast<Player*>(m_player.get())->setPosition(originalPos);
				}
				else if (!m_player->isDead())
				{
					if (m_player->getCurrentTexture())
					{
						const double scale = m_player->getScale();
						const double rotation = m_player->getRotation();
						const ColorF tint = m_player->getTint();
						const Texture currentTexture = m_player->getCurrentTexture();

						if (m_player->getDirection() == PlayerDirection::Left)
						{
							currentTexture.scaled(scale).mirrored().rotated(rotation).drawAt(playerScreenPos, tint);
						}
						else
						{
							currentTexture.scaled(scale).rotated(rotation).drawAt(playerScreenPos, tint);
						}
					}
					else
					{
						const ColorF fallbackColor = m_player->getTint();
						Circle(playerScreenPos, 25).draw(fallbackColor);
					}
				}
			}

			if (m_hudSystem)
			{
				m_hudSystem->draw();
			}

			// ★ 新規追加: 昼夜システムのUI描画
			drawDayNightUI();

			// UI要素の描画
			const String timeText = U"Time: {:.1f}s"_fmt(m_gameTime);
			m_gameFont(timeText).draw(10, 10, ColorF(1.0, 1.0, 1.0));

			if (m_player && !m_player->isExploding())
			{
				const String controlText = U"WASD: Move, SPACE: Jump, S: Duck, F: Fireball, ESC: Title, R: Character Select";
				m_gameFont(controlText).draw(10, Scene::Height() - 40, ColorF(0.8, 0.8, 0.8));
			}
			else if (m_player && m_player->isExploding())
			{
				const String explosionText = U"GAME OVER...";
				const Vec2 messagePos = Vec2(Scene::Center().x, Scene::Height() - 100);
				const double alpha = 0.7 + 0.3 * std::sin(Scene::Time() * 4.0);
				Font(32, Typeface::Bold)(explosionText).drawAt(messagePos, ColorF(1.0, 0.3, 0.3, alpha));
			}
		}

		// シェーダーエフェクトを適用して画面に描画
		m_shaderEffects->endCaptureAndDraw();

		if (m_dayNightSystem)
		{
			RenderTexture tempTexture(Scene::Size());
			{
				const ScopedRenderTarget2D target2(tempTexture);
				Scene::Rect().draw(ColorF(0.0, 0.0, 0.0, 0.0));
				m_shaderEffects->getRenderTarget().draw();
			}
			m_dayNightSystem->applyShader(tempTexture);
		}
	}
	else if (m_player && m_player->isExploding())
	{
		// 爆散中は特別なメッセージ
		const String explosionText = U"GAME OVER...";
		const Vec2 messagePos = Vec2(Scene::Center().x, Scene::Height() - 100);
		const double alpha = 0.7 + 0.3 * std::sin(Scene::Time() * 4.0);
		Font(32, Typeface::Bold)(explosionText).drawAt(messagePos, ColorF(1.0, 0.3, 0.3, alpha));
	}

	// ★ 削除: 古い時間表示コード（drawDayNightUI()に統合）

	// デバッグ情報（Debug時のみ表示）
#ifdef _DEBUG
	if (m_player)
	{
		const String playerInfo = U"Player: {} - State: {} - Exploding: {} - Dead: {}"_fmt(
			m_player->getColorString(),
			m_player->getStateString(),
			m_player->isExploding() ? U"Yes" : U"No",
			m_player->isDead() ? U"Yes" : U"No"
		);
		m_gameFont(playerInfo).draw(10, 40, ColorF(1.0, 1.0, 0.8));

		if (m_player->isExploding())
		{
			const String explosionInfo = U"Explosion Timer: {:.2f}s"_fmt(m_player->getDeathTimer());
			m_gameFont(explosionInfo).draw(10, 70, ColorF(1.0, 0.5, 0.5));
		}

		const Vec2 playerWorldPos = m_player->getPosition();
		const String positionInfo = U"Position: ({:.0f}, {:.0f})"_fmt(
			playerWorldPos.x,
			playerWorldPos.y
		);
		m_gameFont(positionInfo).draw(10, 100, ColorF(0.8, 0.8, 1.0));

		if (m_stage)
		{
			const Vec2 cameraOffset = m_stage->getCameraOffset();
			const String cameraInfo = U"Camera: ({:.0f}, {:.0f})"_fmt(
				cameraOffset.x,
				cameraOffset.y
			);
			m_gameFont(cameraInfo).draw(10, 130, ColorF(1.0, 0.8, 0.8));
		}

		const String enemyInfo = U"Enemies: {}"_fmt(m_enemies.size());
		m_gameFont(enemyInfo).draw(10, 160, ColorF(0.8, 1.0, 0.8));

		// BlockSystemのデバッグ情報
		if (m_blockSystem)
		{
			const String blockInfo = U"Blocks: {} total, {} active, {} coins earned"_fmt(
				m_blockSystem->getTotalBlockCount(),
				m_blockSystem->getActiveBlockCount(),
				m_blockSystem->getCoinsFromBlocks()
			);
			m_gameFont(blockInfo).draw(10, 250, ColorF(1.0, 1.0, 0.0));
		}

		if (m_coinSystem && m_starSystem)
		{
			const String collectibleInfo = U"Coins: {} | Stars: {} / {} (Total: {} / Active: {})"_fmt(
				m_coinSystem->getCollectedCoinsCount(),
				m_starSystem->getCollectedStarsCount(),
				3,
				m_starSystem->getTotalStarsCount(),
				m_starSystem->getActiveStarsCount()
			);
			m_gameFont(collectibleInfo).draw(10, 190, ColorF(0.8, 0.8, 1.0));
		}

		if (m_hudSystem)
		{
			const String hudInfo = U"HUD Stars: {} / {} | Life: {}"_fmt(
				m_hudSystem->getCollectedStars(),
				m_hudSystem->getTotalStars(),
				m_hudSystem->getCurrentLife()
			);
			m_gameFont(hudInfo).draw(10, 220, ColorF(1.0, 0.8, 0.8));
		}

		// 昼夜システムのデバッグ情報
		if (m_dayNightSystem)
		{
			const String dayNightInfo = U"DayNight: Phase={} | Time={:.1f}/{:.1f} | UntilNight={:.1f}s | Stars={}"_fmt(
				static_cast<int>(m_dayNightSystem->getCurrentPhase()),
				m_dayNightSystem->getTimeOfDay(),
				m_dayNightSystem->getTimeOfDay() / m_dayNightSystem->getNormalizedTime(),
				m_dayNightSystem->getTimeUntilNight(),
				m_dayNightSystem->getStarsCollected()
			);
			m_gameFont(dayNightInfo).draw(10, 280, ColorF(0.8, 1.0, 1.0));
		}
	}

	if (m_stage)
	{
		const String stageInfo = U"Current Stage: {} ({})"_fmt(
			static_cast<int>(m_currentStageNumber),
			m_stage->getStageName()
		);
		m_gameFont(stageInfo).draw(10, 310, ColorF(0.8, 1.0, 0.8));
	}
#endif
}

void GameScene::drawPlayerFireballs() const
{
	if (!m_player || !m_stage) return;

	const Vec2 cameraOffset = m_stage->getCameraOffset();
	const auto& fireballs = m_player->getFireballs();

	for (const auto& fireball : fireballs)
	{
		if (!fireball.active) continue;

		// ワールド座標からスクリーン座標に変換
		const Vec2 fireballScreenPos = m_stage->worldToScreenPosition(fireball.position);

		// 画面内にある場合のみ描画
		if (fireballScreenPos.x >= -50 && fireballScreenPos.x <= Scene::Width() + 50 &&
			fireballScreenPos.y >= -50 && fireballScreenPos.y <= Scene::Height() + 50)
		{
			// ファイアボールテクスチャを取得（Playerから直接取得できないため代替手段）
			// まず円で描画（フォールバック）
			const double size = 24.0;
			Circle(fireballScreenPos, size).draw(ColorF(1.0, 0.5, 0.0, 0.8));
			Circle(fireballScreenPos, size * 0.7).draw(ColorF(1.0, 0.8, 0.2, 0.9));
			Circle(fireballScreenPos, size * 0.4).draw(ColorF(1.0, 1.0, 0.6, 1.0));

			// 軌跡エフェクト
			const double trailAlpha = 0.4;
			Circle(fireballScreenPos, size * 1.2).draw(ColorF(1.0, 0.3, 0.0, trailAlpha * 0.3));

			// 回転エフェクト（線で表現）
			const double rotation = fireball.rotation;
			const Vec2 direction1(std::cos(rotation), std::sin(rotation));
			const Vec2 direction2(std::cos(rotation + Math::HalfPi), std::sin(rotation + Math::HalfPi));

			Line(fireballScreenPos - direction1 * size * 0.5, fireballScreenPos + direction1 * size * 0.5)
				.draw(3.0, ColorF(1.0, 1.0, 0.8));
			Line(fireballScreenPos - direction2 * size * 0.5, fireballScreenPos + direction2 * size * 0.5)
				.draw(3.0, ColorF(1.0, 1.0, 0.8));

#ifdef _DEBUG
			// デバッグ情報
			const String fireballDebug = U"FB: ({:.0f}, {:.0f})"_fmt(fireball.position.x, fireball.position.y);
			Font(12)(fireballDebug).draw(fireballScreenPos + Vec2(30, -20), ColorF(1.0, 1.0, 0.0));
#endif
		}
	}
}

void GameScene::drawFireballDestructionEffects() const
{
	if (!m_stage) return;

	for (const auto& effect : m_fireballDestructionEffects)
	{
		if (!effect.active) continue;

		const Vec2 screenPos = m_stage->worldToScreenPosition(effect.position);

		// 画面内にある場合のみ描画
		if (screenPos.x >= -100 && screenPos.x <= Scene::Width() + 100)
		{
			// 衝撃波描画
			for (const auto& shockwave : effect.shockwaves)
			{
				if (shockwave.delay <= 0.0 && shockwave.life > 0.0)
				{
					const Vec2 shockwaveScreenPos = m_stage->worldToScreenPosition(shockwave.position);
					const double alpha = shockwave.life / shockwave.maxLife;

					// 外側の衝撃波
					Circle(shockwaveScreenPos, shockwave.radius).drawFrame(4.0,
						ColorF(1.0, 0.6, 0.2, alpha * 0.6));

					// 内側の衝撃波
					Circle(shockwaveScreenPos, shockwave.radius * 0.8).drawFrame(2.0,
						ColorF(1.0, 0.9, 0.4, alpha * 0.8));
				}
			}

			// パーティクル描画
			for (const auto& particle : effect.particles)
			{
				const Vec2 particleScreenPos = m_stage->worldToScreenPosition(particle.position);
				const double alpha = particle.life / particle.maxLife;
				const ColorF color = ColorF(particle.color.r, particle.color.g, particle.color.b, alpha);

				// メインパーティクル
				Circle(particleScreenPos, particle.size).draw(color);

				// 光る効果
				Circle(particleScreenPos, particle.size * 1.5).draw(
					ColorF(1.0, 0.8, 0.4, alpha * 0.3));

				// 火花効果
				if (particle.size > 2.0)
				{
					const double sparkSize = particle.size * 0.3;
					Line(particleScreenPos.x - sparkSize, particleScreenPos.y,
						 particleScreenPos.x + sparkSize, particleScreenPos.y)
						.draw(1.0, ColorF(1.0, 1.0, 0.8, alpha));
					Line(particleScreenPos.x, particleScreenPos.y - sparkSize,
						 particleScreenPos.x, particleScreenPos.y + sparkSize)
						.draw(1.0, ColorF(1.0, 1.0, 0.8, alpha));
				}
			}

			// 中央の爆発フラッシュ
			if (effect.timer <= 0.3)
			{
				const double flashProgress = effect.timer / 0.3;
				const double flashAlpha = 1.0 - flashProgress;
				const double flashRadius = 40.0 + flashProgress * 30.0;

				Circle(screenPos, flashRadius).draw(ColorF(1.0, 0.8, 0.2, flashAlpha * 0.4));
				Circle(screenPos, flashRadius * 0.6).draw(ColorF(1.0, 1.0, 0.8, flashAlpha * 0.6));
			}
		}
	}
}

Optional<SceneType> GameScene::getNextScene() const
{
	return m_nextScene;
}

void GameScene::cleanup()
{
	SoundManager::GetInstance().stopBGM();

	m_player.reset();
	m_stage.reset();
	m_enemies.clear();
	m_hudSystem.reset();
	m_coinSystem.reset();
	m_starSystem.reset();
	m_blockSystem.reset();
	m_collisionSystem.reset();
}

void GameScene::loadStage(StageNumber stageNumber)
{
	m_currentStageNumber = stageNumber;
	m_stage = std::make_unique<Stage>(stageNumber);

	// プレイヤーの位置をステージの安全な場所にリセット
	if (m_player)
	{
		const double BLOCK_SIZE = 64.0;
		// 安全な位置: 3ブロック目、地面の上
		Vec2 safePosition = Vec2(3.0 * BLOCK_SIZE, 12.5 * BLOCK_SIZE);

		m_player->setPosition(safePosition);
		m_player->setVelocity(Vec2::Zero());
		m_player->resetFireballCount();
	}

	// 収集アイテムの再生成
	if (m_coinSystem)
	{
		m_coinSystem->generateCoinsForStage(stageNumber);
		m_coinSystem->resetCollectedCount();
	}

	if (m_starSystem)
	{
		m_starSystem->generateStarsForStage(stageNumber);
		m_starSystem->resetCollectedCount();
	}

	// BlockSystemのステージ切り替え
	if (m_blockSystem)
	{
		m_blockSystem->generateBlocksForStage(stageNumber);
	}

	// 敵の再初期化
	initEnemies();

	// ゴール状態をリセット
	m_goalReached = false;
	m_goalTimer = 0.0;
}

void GameScene::initEnemies()
{
	m_enemies.clear();

	String stageFile;
	stageFile = U"Stages/Stage{}.json"_fmt(static_cast<int>(m_currentStageNumber));
	

	if (!FileSystem::Exists(stageFile)) {
#ifdef _DEBUG
		Print << U"Not find Stage file: " << stageFile;
#endif
		return;
	}

	const JSON stageData = JSON::Load(stageFile);
	if (!stageData) {
#ifdef _DEBUG
		Print << U"Failed to load json: " << stageFile;
#endif
		return;
	}

	for (const auto& enemyEntry : stageData.arrayView()) {
		String type = enemyEntry[U"type"].getString();
		double x = enemyEntry[U"x"].get<double>();
		double y = enemyEntry[U"y"].get<double>();

		try {
			addEnemy(spawnEnemy(type, Vec2{ x,y }));
		}
		catch (const std::exception& e) {
			Print << U"Failed to generate enemy: " << Unicode::FromUTF8(e.what());
		}
	}
}


void GameScene::updateEnemies()
{
	if (!m_dayNightSystem)
	{
		// 昼夜システムがない場合は通常更新
		for (auto& enemy : m_enemies)
		{
			if (enemy && enemy->isActive())
			{
				enemy->update();
				enemy->updateBlackFireAnimation(); // 黒い炎アニメーション更新
			}
		}
	}
	else
	{
		// 夜になった瞬間の変身処理
		if (m_dayNightSystem->justBecameNight())
		{
			// すべての敵を変身させる
			for (auto& enemy : m_enemies)
			{
				if (!enemy || !enemy->isActive()) continue;

				// 変身対象の敵タイプのみ変身
				bool shouldTransform = false;
				switch (enemy->getType())
				{
				case EnemyType::NormalSlime:
				case EnemyType::SpikeSlime:
				case EnemyType::Bee:
				case EnemyType::Fly:
					shouldTransform = true;
					break;
				default:
					break;
				}

				if (shouldTransform)
				{
					// 敵を変身状態にする
					enemy->transform();

					// 変身時の速度・挙動変更
					Vec2 velocity = enemy->getVelocity();
					velocity.x *= 1.5; // 速度1.5倍
					enemy->setVelocity(velocity);

					// サウンドエフェクト再生
					SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_BREAK_BLOCK);
				}
			}

			// フラグをリセット
			m_dayNightSystem->resetNightTransition();
		}

		// 昼に戻った時の変身解除
		if (!m_dayNightSystem->isNight() && !m_dayNightSystem->isDangerous())
		{
			for (auto& enemy : m_enemies)
			{
				if (enemy && enemy->isTransformed())
				{
					enemy->untransform();

					// 速度を通常に戻す
					Vec2 velocity = enemy->getVelocity();
					velocity.x /= 1.5;
					enemy->setVelocity(velocity);
				}
			}
		}

		// 変身中の敵の特殊挙動
		const double speedMultiplier = m_dayNightSystem->getEnemySpeedMultiplier();
		const double aggressionMultiplier = m_dayNightSystem->getEnemyAggressionMultiplier();
		const bool isNightTime = m_dayNightSystem->isNight();
		const bool isDangerousTime = m_dayNightSystem->isDangerous();

		for (auto& enemy : m_enemies)
		{
			if (!enemy || !enemy->isActive()) continue;

			// 黒い炎アニメーション更新
			enemy->updateBlackFireAnimation();

			// 変身中の特殊挙動
			if (enemy->isTransformed())
			{
				switch (enemy->getType())
				{
				case EnemyType::NormalSlime:
				{
					NormalSlime* slime = static_cast<NormalSlime*>(enemy.get());
					Vec2 velocity = slime->getVelocity();

					// 変身中は常に高速移動
					if (std::abs(velocity.x) > 0)
					{
						velocity.x = (velocity.x > 0 ? 1 : -1) * 100.0;
					}

					// 時々大ジャンプ
					if (slime->isGrounded() && Random(0.0, 1.0) < 0.02)
					{
						velocity.y = -400.0;
					}

					slime->setVelocity(velocity);
				}
				break;

				case EnemyType::SpikeSlime:
				{
					SpikeSlime* spike = static_cast<SpikeSlime*>(enemy.get());
					if (m_player)
					{
						const Vec2 playerPos = m_player->getPosition();
						const Vec2 enemyPos = spike->getPosition();
						const double distance = playerPos.distanceFrom(enemyPos);

						// 変身中は追跡範囲拡大
						if (distance < 500.0)
						{
							const bool shouldGoLeft = playerPos.x < enemyPos.x;
							const EnemyDirection targetDir = shouldGoLeft ?
								EnemyDirection::Left : EnemyDirection::Right;

							if (spike->getDirection() != targetDir)
							{
								spike->changeDirection();
							}

							Vec2 velocity = spike->getVelocity();
							velocity.x = (shouldGoLeft ? -1 : 1) * 120.0;
							spike->setVelocity(velocity);
						}
					}
				}
				break;

				case EnemyType::Bee:
				{
					Bee* bee = static_cast<Bee*>(enemy.get());
					if (m_player)
					{
						// 変身中は超積極的に追跡
						const double chaseRange = 600.0;
						const Vec2 playerPos = m_player->getPosition();
						const double distance = bee->getPosition().distanceFrom(playerPos);

						if (distance < chaseRange)
						{
							bee->updateChase(playerPos);
							Vec2 velocity = bee->getVelocity();
							velocity *= 2.0; // 倍速
							bee->setVelocity(velocity);
						}
					}
				}
				break;

				case EnemyType::Fly:
				{
					Fly* fly = static_cast<Fly*>(enemy.get());
					if (m_player)
					{
						const Vec2 playerPos = m_player->getPosition();
						const Vec2 flyPos = fly->getPosition();

						// 変身中は直接プレイヤーを追跡
						const Vec2 direction = (playerPos - flyPos).normalized();
						Vec2 velocity = direction * 150.0;

						// ジグザグ動作を追加
						velocity.x += std::sin(Scene::Time() * 10.0) * 50.0;
						velocity.y += std::cos(Scene::Time() * 10.0) * 30.0;

						fly->setVelocity(velocity);
					}
				}
				break;

				default:
					break;
				}
			}

			// 共通の更新処理
			enemy->update();
		}
	}

	// 非アクティブな敵を削除
	m_enemies.erase(
		std::remove_if(m_enemies.begin(), m_enemies.end(),
			[](const std::unique_ptr<EnemyBase>& enemy) {
				return !enemy || !enemy->isActive();
			}),
		m_enemies.end()
	);
}

void GameScene::drawEnemies() const
{
	if (!m_stage) return;

	for (const auto& enemy : m_enemies)
	{
		if (enemy->isActive() || enemy->getState() == EnemyState::Flattened)
		{
			const Vec2 enemyWorldPos = enemy->getPosition();
			const Vec2 enemyScreenPos = m_stage->worldToScreenPosition(enemyWorldPos);

			if (enemyScreenPos.x >= -100 && enemyScreenPos.x <= Scene::Width() + 100)
			{
				// 変身状態なら黒い炎を描画
				if (enemy->isTransformed() && m_blackFireTexture)
				{
					// 現在のフレームを取得
					const int frame = enemy->getBlackFireFrame();
					const int row = frame / 4;
					const int col = frame % 4;

					// スプライトの切り出し（128x128）
					const Rect srcRect(
						col * BLACKFIRE_SPRITE_SIZE,
						row * BLACKFIRE_SPRITE_SIZE,
						BLACKFIRE_SPRITE_SIZE,
						BLACKFIRE_SPRITE_SIZE
					);

					// 80x80で描画
					const double scale = BLACKFIRE_DRAW_SIZE / BLACKFIRE_SPRITE_SIZE;

					// 敵の向きに応じて反転
					if (enemy->getDirection() == EnemyDirection::Left)
					{
						m_blackFireTexture(srcRect)
							.scaled(scale)
							.mirrored()
							.drawAt(enemyScreenPos);
					}
					else
					{
						m_blackFireTexture(srcRect)
							.scaled(scale)
							.drawAt(enemyScreenPos);
					}

					// 変身中の追加エフェクト（黒いオーラ）
					const double auraAlpha = 0.3 + std::sin(Scene::Time() * 8.0) * 0.2;
					Circle(enemyScreenPos, 45).draw(ColorF(0.1, 0.0, 0.2, auraAlpha));
				}
				// 通常状態なら元のテクスチャを描画
				else
				{
					ColorF tint = ColorF(1.0, 1.0, 1.0);

					// 昼夜による色調整（非変身時のみ）
					if (m_dayNightSystem && !enemy->isTransformed())
					{
						if (m_dayNightSystem->isNight())
						{
							tint = ColorF(1.2, 0.8, 0.8);
						}
						else if (m_dayNightSystem->getCurrentPhase() == DayNightSystem::TimePhase::Sunset)
						{
							tint = ColorF(1.1, 0.9, 0.8);
						}
					}

					const Texture currentTexture = enemy->getCurrentTexture();
					if (currentTexture)
					{
						if (enemy->getDirection() == EnemyDirection::Left)
						{
							currentTexture.mirrored().drawAt(enemyScreenPos, tint);
						}
						else
						{
							currentTexture.drawAt(enemyScreenPos, tint);
						}
					}
					else
					{
						// フォールバック描画
						ColorF fallbackColor;
						switch (enemy->getType())
						{
						case EnemyType::NormalSlime:  fallbackColor = ColorF(0.2, 0.8, 0.2); break;
						case EnemyType::SpikeSlime:   fallbackColor = ColorF(0.6, 0.2, 0.6); break;
						case EnemyType::Ladybug:      fallbackColor = ColorF(0.8, 0.2, 0.2); break;
						case EnemyType::SlimeBlock:   fallbackColor = ColorF(0.2, 0.5, 0.8); break;
						case EnemyType::Saw:          fallbackColor = ColorF(0.7, 0.7, 0.7); break;
						case EnemyType::Bee:          fallbackColor = ColorF(1.0, 1.0, 0.0); break;
						case EnemyType::Fly:          fallbackColor = ColorF(0.3, 0.3, 0.3); break;
						default:                      fallbackColor = ColorF(0.5, 0.5, 0.5); break;
						}
						Circle(enemyScreenPos, 32).draw(fallbackColor * tint);
					}
				}

				// 状態エフェクト描画
				if (enemy->getState() == EnemyState::Flattened)
				{
					drawEnemyFlattenedEffect(enemyScreenPos, enemy.get());
				}
				else if (enemy->getState() == EnemyState::Hit)
				{
					drawEnemyHitEffect(enemyScreenPos, enemy.get());
				}
			}
		}
	}
}

void GameScene::drawEnemyFlattenedEffect(const Vec2& screenPos, const EnemyBase* enemy) const
{
	// NormalSlimeの場合のみエフェクト描画
	if (enemy->getType() == EnemyType::NormalSlime)
	{
		const NormalSlime* slime = static_cast<const NormalSlime*>(enemy);
		// エフェクトタイマーの取得方法は実装に応じて調整
		const double effectTime = Scene::Time(); // 暫定的にScene::Time()を使用

		// 星型エフェクト
		for (int i = 0; i < 5; ++i)
		{
			const double angle = i * Math::TwoPi / 5.0 + effectTime * 2.0;
			const double distance = 30.0 + std::sin(effectTime * 8.0) * 10.0;
			const Vec2 starPos = screenPos + Vec2(std::cos(angle), std::sin(angle)) * distance;
			const double alpha = 0.8;

			// 星の描画
			const double starSize = 3.0;
			Line(starPos.x - starSize, starPos.y, starPos.x + starSize, starPos.y).draw(2.0, ColorF(1.0, 1.0, 0.0, alpha));
			Line(starPos.x, starPos.y - starSize, starPos.x, starPos.y + starSize).draw(2.0, ColorF(1.0, 1.0, 0.0, alpha));
		}
	}
}

void GameScene::drawEnemyHitEffect(const Vec2& screenPos, const EnemyBase* enemy) const
{
	// ヒット時のエフェクト（点滅）
	const double flashRate = 10.0;
	const double alpha = (std::sin(Scene::Time() * flashRate) + 1.0) * 0.3;

	if (alpha > 0.1)
	{
		Circle(screenPos, 40).draw(ColorF(1.0, 0.0, 0.0, alpha));
	}
}

void GameScene::updatePlayerEnemyCollision()
{
	if (!m_player || !m_stage) return;

	const Vec2 playerPos = m_player->getPosition();
	const double BLOCK_SIZE = 64.0;

	// ★ プレイヤーの衝突矩形を1ブロック基準で統一
	const RectF playerRect(
		playerPos.x - BLOCK_SIZE / 2,
		playerPos.y - BLOCK_SIZE / 2,
		BLOCK_SIZE,
		BLOCK_SIZE
	);

	for (auto& enemy : m_enemies)
	{
		if (!enemy->isActive() || !enemy->isAlive()) continue;

		const RectF enemyRect = enemy->getCollisionRect();

		if (playerRect.intersects(enemyRect))
		{
			// 特殊な敵の処理
			handleSpecialEnemyCollision(enemy.get());

			// 踏みつけ判定（1ブロック基準で改良）
			if (isPlayerStompingEnemy(playerRect, enemyRect) &&
				canStompEnemy(enemy.get()) &&
				enemy->getState() != EnemyState::Flattened)
			{
				handlePlayerStompEnemy(enemy.get());
			}
			else
			{
				// 横からの衝突（ダメージ）
				if (!m_player->isInvincible() && enemy->getState() != EnemyState::Flattened)
				{
					bool shouldTakeDamage = false;

					switch (enemy->getType())
					{
					case EnemyType::Saw:
					case EnemyType::SpikeSlime:
						shouldTakeDamage = true;  // 常に危険
						break;
					default:
						shouldTakeDamage = true;  // 通常の敵からもダメージ
						break;
					}

					if (shouldTakeDamage)
					{
						handlePlayerHitByEnemy(enemy.get());
					}
				}
			}
		}
	}
}

void GameScene::updateEnemyStageCollision()
{
	if (!m_stage) return;

	const Array<RectF> collisionRects = m_stage->getCollisionRects();

	for (auto& enemy : m_enemies)
	{
		if (!enemy->isActive() || !enemy->isAlive()) continue;

		const Vec2 enemyPos = enemy->getPosition();
		const RectF enemyRect = enemy->getCollisionRect();
		bool isOnGround = false;

		// 各ブロックとの衝突をチェック
		for (const auto& blockRect : collisionRects)
		{
			if (enemyRect.intersects(blockRect))
			{
				// 衝突方向を判定
				const Vec2 enemyCenter = enemyRect.center();
				const Vec2 blockCenter = blockRect.center();
				const Vec2 distance = enemyCenter - blockCenter;

				// X方向とY方向の重複を計算
				const double overlapX = (enemyRect.w + blockRect.w) / 2.0 - std::abs(distance.x);
				const double overlapY = (enemyRect.h + blockRect.h) / 2.0 - std::abs(distance.y);

				// より小さい重複の方向で押し戻し
				if (overlapX < overlapY)
				{
					// X方向の衝突（壁）
					Vec2 newPos = enemyPos;
					if (distance.x > 0)
					{
						newPos.x = blockRect.x + blockRect.w + enemyRect.w / 2;
					}
					else
					{
						newPos.x = blockRect.x - enemyRect.w / 2;
					}
					enemy->setPosition(newPos);

					// 敵の方向を反転（NormalSlimeの場合）
					if (enemy->getType() == EnemyType::NormalSlime)
					{
						NormalSlime* slime = static_cast<NormalSlime*>(enemy.get());
						slime->changeDirection();
					}
				}
				else
				{
					// Y方向の衝突
					Vec2 newPos = enemyPos;
					Vec2 newVelocity = enemy->getVelocity();

					if (distance.y > 0)
					{
						// 敵が上側（天井）
						newPos.y = blockRect.y + blockRect.h + enemyRect.h / 2;
						newVelocity.y = 0;
					}
					else
					{
						// 敵が下側（地面に着地）
						newPos.y = blockRect.y - enemyRect.h / 2;
						newVelocity.y = 0;
						isOnGround = true;
					}

					enemy->setPosition(newPos);
					enemy->setVelocity(newVelocity);
				}
				break;
			}
		}

		enemy->setGrounded(isOnGround);
	}
}

void GameScene::addEnemy(std::unique_ptr<EnemyBase> enemy)
{
	if (enemy)
	{
		m_enemies.push_back(std::move(enemy));
	}
}

bool GameScene::isPlayerStompingEnemy(const RectF& playerRect, const RectF& enemyRect) const
{
	// ★ 1ブロック基準での踏みつけ判定を改良
	const Vec2 playerVelocity = m_player->getVelocity();
	const double BLOCK_SIZE = 64.0;

	// プレイヤーが落下中であることを確認
	if (playerVelocity.y <= 0) return false;

	// プレイヤーの下端と敵の上端の距離をチェック
	const double playerBottom = playerRect.y + playerRect.h;
	const double enemyTop = enemyRect.y;

	// 1ブロックの1/4以内の距離で踏みつけと判定
	const double stompTolerance = BLOCK_SIZE * 0.25;

	return (playerBottom >= enemyTop && playerBottom <= enemyTop + stompTolerance);
}

void GameScene::handlePlayerStompEnemy(EnemyBase* enemy)
{
	if (!enemy || !enemy->isActive()) return;

	enemy->onStomp();
	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_HIT);

	if (!m_player->getTutorialNotifiedStomp()) {
		m_player->setTutorialNotifiedStomp(true);
		TutorialEmit(TutorialEvent::StompEnemy, enemy->getPosition());
	}

	// 衝撃波エフェクトを追加
	if (m_shaderEffects && m_stage)
	{
		m_shaderEffects->triggerShockwave(enemy->getPosition(), m_stage->getCameraOffset());
	}

	if (m_player)
	{
		Vec2 playerVelocity = m_player->getVelocity();
		playerVelocity.y = -300.0;
		m_player->setVelocity(playerVelocity);
	}
}

void GameScene::updateFireballEnemyCollision()
{
	if (!m_player) return;

	const auto& fireballs = m_player->getFireballs();

	for (const auto& fireball : fireballs)
	{
		if (!fireball.active) continue;

		const RectF fireballRect(fireball.position.x - 16, fireball.position.y - 16, 32, 32);

		for (auto& enemy : m_enemies)
		{
			if (!enemy->isActive() || !enemy->isAlive()) continue;

			// Sawには効かない
			if (enemy->getType() == EnemyType::Saw) continue;

			const RectF enemyRect = enemy->getCollisionRect();

			if (fireballRect.intersects(enemyRect))
			{
				// ★ 派手な撃破エフェクトを生成
				createFireballDestructionEffect(enemy->getPosition(), enemy->getType(), fireball.position);

				// 敵を撃破
				enemy->onDestroy();

				if (m_player && !m_player->getTutorialNotifiedFireball()) {
					m_player->setTutorialNotifiedFireball(true);
					TutorialEmit(TutorialEvent::FireballKill, enemy->getPosition());
				}

				// ファイアボールを無効化
				m_player->deactivateFireball(fireball.position);

				// ★ より派手な効果音を再生
				SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_BREAK_BLOCK);

				break; // 一つの敵に当たったらループ終了
			}
		}
	}
}

void GameScene::handleEnemyHitByFireball(EnemyBase* enemy, const Vec2& fireballPosition)
{
	if (!enemy) return;

	// 敵を即座に死亡状態にする
	enemy->onDestroy();
	// ★ 敵の種類に応じた撃破エフェクトを生成
	createFireballDestructionEffect(enemy->getPosition(), enemy->getType(), fireballPosition);
}

void GameScene::handlePlayerHitByEnemy(EnemyBase* enemy)
{
	if (!enemy) return;

	const double BLOCK_SIZE = 64.0;

	// プレイヤーがダメージを受ける
	m_player->hit();

	// ノックバック効果（1ブロック基準）
	const Vec2 playerPos = m_player->getPosition();
	const Vec2 enemyPos = enemy->getPosition();
	const Vec2 knockbackDirection = (playerPos - enemyPos).normalized();

	Vec2 playerVelocity = m_player->getVelocity();
	playerVelocity.x = knockbackDirection.x * BLOCK_SIZE * 3.0;  // 3ブロック/秒のノックバック
	playerVelocity.y = -BLOCK_SIZE * 2.0;  // 2ブロック/秒で軽く浮く
	m_player->setVelocity(playerVelocity);

	m_hudSystem->subtractLife(1);  // ライフを1減らす
}

// 新敵用の特殊処理メソッド
void GameScene::handleSpecialEnemyCollision(EnemyBase* enemy)
{
	if (!enemy) return;

	switch (enemy->getType())
	{
	case EnemyType::SpikeSlime:
	{
		SpikeSlime* spikeSlime = static_cast<SpikeSlime*>(enemy);
		if (spikeSlime->isDangerous())
		{
			// SpikeSlimeが危険状態の場合、プレイヤーがダメージを受ける
			handlePlayerHitByEnemy(enemy);
		}
	}
	break;

	case EnemyType::Saw:
	{
		Saw* saw = static_cast<Saw*>(enemy);
		if (saw->isDangerous())
		{
			// Sawは常に危険
			handlePlayerHitByEnemy(enemy);
		}
	}
	break;

	case EnemyType::Bee:
	{
		Bee* bee = static_cast<Bee*>(enemy);
		if (m_player)
		{
			bee->updateChase(m_player->getPosition());
		}
	}
	break;

	default:
		// 通常の敵は従来の処理
		break;
	}
}

// 敵を踏めるかどうかの判定
bool GameScene::canStompEnemy(const EnemyBase* enemy) const
{
	if (!enemy || !enemy->isActive() || !enemy->isAlive()) return false;

	switch (enemy->getType())
	{
	case EnemyType::Saw:
		// Sawは踏めない（プレイヤーがダメージを受ける）
		return false;

	case EnemyType::SpikeSlime:
		// SpikeSlimeは踏めない（常にスパイクがあるためダメージを受ける）
		return false;

	case EnemyType::Bee:
	case EnemyType::Fly:
		// 飛行中の敵は踏みにくいが、可能
		return true;

	case EnemyType::NormalSlime:
	case EnemyType::Ladybug:
	case EnemyType::SlimeBlock:
	default:
		// 通常の敵は踏める
		return true;
	}
}


void GameScene::updateTotalCoinsFromBlocks()
{
	if (!m_blockSystem || !m_hudSystem || !m_coinSystem) return;

	// 通常のコインとブロックから獲得したコインの合計をHUDに反映
	int totalCoins = m_coinSystem->getCollectedCoinsCount() + m_blockSystem->getCoinsFromBlocks();
	m_hudSystem->setCoins(totalCoins);
}

void GameScene::createFireballDestructionEffect(const Vec2& enemyPos, EnemyType enemyType, const Vec2& fireballPos)
{
	// エフェクトデータを作成
	FireballDestructionEffect effect;
	effect.position = enemyPos;
	effect.fireballDirection = (enemyPos - fireballPos).normalized();
	effect.enemyType = enemyType;
	effect.timer = 0.0;
	effect.active = true;

	// 敵の種類に応じたパーティクル数と色を設定
	switch (enemyType)
	{
	case EnemyType::NormalSlime:
		effect.particleCount = 15;
		effect.primaryColor = ColorF(0.2, 0.8, 0.2);
		effect.secondaryColor = ColorF(0.5, 1.0, 0.5);
		break;
	case EnemyType::SpikeSlime:
		effect.particleCount = 20;
		effect.primaryColor = ColorF(0.6, 0.2, 0.6);
		effect.secondaryColor = ColorF(1.0, 0.4, 1.0);
		break;
	case EnemyType::Ladybug:
		effect.particleCount = 18;
		effect.primaryColor = ColorF(0.8, 0.2, 0.2);
		effect.secondaryColor = ColorF(1.0, 0.5, 0.2);
		break;
	case EnemyType::SlimeBlock:
		effect.particleCount = 25;
		effect.primaryColor = ColorF(0.2, 0.5, 0.8);
		effect.secondaryColor = ColorF(0.4, 0.7, 1.0);
		break;
	case EnemyType::Bee:
		effect.particleCount = 12;
		effect.primaryColor = ColorF(1.0, 1.0, 0.0);
		effect.secondaryColor = ColorF(1.0, 0.8, 0.2);
		break;
	case EnemyType::Fly:
		effect.particleCount = 10;
		effect.primaryColor = ColorF(0.3, 0.3, 0.3);
		effect.secondaryColor = ColorF(0.6, 0.6, 0.6);
		break;
	default:
		effect.particleCount = 15;
		effect.primaryColor = ColorF(0.8, 0.8, 0.8);
		effect.secondaryColor = ColorF(1.0, 1.0, 1.0);
		break;
	}

	// パーティクルを生成
	for (int i = 0; i < effect.particleCount; ++i)
	{
		FireballParticle particle;

		// ファイアボールの方向を基準に少し散らす
		const double baseAngle = std::atan2(effect.fireballDirection.y, effect.fireballDirection.x);
		const double angle = baseAngle + Random(-Math::Pi * 0.7, Math::Pi * 0.7);
		const double speed = Random(150.0, 400.0);

		particle.position = enemyPos + Vec2(Random(-10.0, 10.0), Random(-10.0, 10.0));
		particle.velocity = Vec2(std::cos(angle), std::sin(angle)) * speed;
		particle.life = Random(0.8, 1.5);
		particle.maxLife = particle.life;
		particle.size = Random(3.0, 8.0);
		particle.rotation = Random(0.0, Math::TwoPi);
		particle.rotationSpeed = Random(-15.0, 15.0);

		// 色をランダムに選択
		particle.color = (i % 2 == 0) ? effect.primaryColor : effect.secondaryColor;

		effect.particles.push_back(particle);
	}

	// 衝撃波を生成
	for (int i = 0; i < 3; ++i)
	{
		FireballShockwave shockwave;
		shockwave.position = enemyPos;
		shockwave.radius = 0.0;
		shockwave.maxRadius = 80.0 + i * 20.0;
		shockwave.life = 0.8 + i * 0.2;
		shockwave.maxLife = shockwave.life;
		shockwave.delay = i * 0.1;

		effect.shockwaves.push_back(shockwave);
	}

	// エフェクトリストに追加
	m_fireballDestructionEffects.push_back(effect);
}

void GameScene::updateFireballDestructionEffects()
{
	const double deltaTime = Scene::DeltaTime();

	for (auto it = m_fireballDestructionEffects.begin(); it != m_fireballDestructionEffects.end();)
	{
		auto& effect = *it;

		if (!effect.active)
		{
			it = m_fireballDestructionEffects.erase(it);
			continue;
		}

		effect.timer += deltaTime;

		// パーティクル更新
		for (auto particleIt = effect.particles.begin(); particleIt != effect.particles.end();)
		{
			auto& particle = *particleIt;

			// 物理更新
			particle.position += particle.velocity * deltaTime;
			particle.velocity.y += 400.0 * deltaTime; // 重力
			particle.velocity *= 0.98; // 空気抵抗

			// 回転更新
			particle.rotation += particle.rotationSpeed * deltaTime;

			// 生存時間更新
			particle.life -= deltaTime;
			particle.size *= 0.996;

			if (particle.life <= 0.0 || particle.size <= 0.5)
			{
				particleIt = effect.particles.erase(particleIt);
			}
			else
			{
				++particleIt;
			}
		}

		// 衝撃波更新
		for (auto shockwaveIt = effect.shockwaves.begin(); shockwaveIt != effect.shockwaves.end();)
		{
			auto& shockwave = *shockwaveIt;

			shockwave.delay -= deltaTime;

			if (shockwave.delay <= 0.0)
			{
				shockwave.life -= deltaTime;
				shockwave.radius += (shockwave.maxRadius / shockwave.maxLife) * deltaTime;

				if (shockwave.life <= 0.0)
				{
					shockwaveIt = effect.shockwaves.erase(shockwaveIt);
				}
				else
				{
					++shockwaveIt;
				}
			}
			else
			{
				++shockwaveIt;
			}
		}

		// エフェクト終了判定
		if (effect.particles.empty() && effect.shockwaves.empty())
		{
			effect.active = false;
		}

		++it;
	}
}

//統一衝突判定システムを使った衝突処理
void GameScene::updatePlayerCollisionsUnified()
{
	if (!m_player || !m_collisionSystem) return;

	// ★ 修正：ヒップドロップ状態の正しい処理
	// ヒップドロップ中でも衝突判定は必要（地面への着地判定のため）
	m_collisionSystem->resolvePlayerCollisions(m_player.get(), m_stage.get(), m_blockSystem.get());

	// ★ 重要：ヒップドロップ着地後の処理を追加
	if (m_player->hasJustLandedFromHipDrop())
	{
		// シェーダーエフェクトを発動
		if (m_shaderEffects && m_stage)
		{
			const Vec2 playerPos = m_player->getPosition();
			m_shaderEffects->triggerShockwave(playerPos, m_stage->getCameraOffset());
		}

		// フラグをクリア
		m_player->clearHipDropLandedFlag();
	}
}


// BlockSystemとの相互作用のみを処理
void GameScene::updateBlockSystemInteractions()
{
	if (!m_blockSystem || !m_player) return;

	// BlockSystem特有の処理（ブロックを叩く、コイン獲得など）
	m_blockSystem->update(m_player.get());
}

void GameScene::updateDayNight()
{
    if (!m_dayNightSystem) return;
    
    // 昼夜の基本更新はDayNightSystem内で行われる
    // ここではゲーム固有の昼夜影響を処理
    
    // ゴール到達時は時間を停止
    if (m_goalReached)
    {
        m_dayNightSystem->pauseTime(true);
    }
    
    // プレイヤーが死亡した場合も時間停止
    if (m_player && (m_player->isDead() || m_player->isExploding()))
    {
        m_dayNightSystem->pauseTime(true);
    }
    
    // 夜の追加効果
    if (m_dayNightSystem->isNight())
    {
        // 夜は視界が悪くなる効果（オプション）
        // プレイヤーの移動速度をわずかに低下
        if (m_player && !m_player->isExploding())
        {
            Vec2 velocity = m_player->getVelocity();
            velocity.x *= 0.9; // 夜は10%移動速度低下
            m_player->setVelocity(velocity);
        }
    }
}

void GameScene::drawDayNightEffects() const
{
    if (!m_dayNightSystem) return;
    
    // 時間帯のUI表示
    const Vec2 timeDisplayPos(Scene::Width() - 200, 20);
    Font timeFont(18, Typeface::Bold);
    
    // 時間帯のテキスト
    String phaseText;
    ColorF phaseColor;
    
    switch (m_dayNightSystem->getCurrentPhase())
    {
    case DayNightSystem::TimePhase::Day:
        phaseText = U"DAY TIME";
        phaseColor = ColorF(1.0, 0.9, 0.3);
        break;
    case DayNightSystem::TimePhase::Sunset:
        phaseText = U"SUNSET";
        phaseColor = ColorF(1.0, 0.6, 0.3);
        break;
    case DayNightSystem::TimePhase::Night:
        phaseText = U"NIGHT TIME";
        phaseColor = ColorF(0.6, 0.6, 1.0);
        break;
    case DayNightSystem::TimePhase::Dawn:
        phaseText = U"DAWN";
        phaseColor = ColorF(1.0, 0.8, 0.5);
        break;
    }
    
    // 背景ボックス
    RectF(timeDisplayPos.x - 10, timeDisplayPos.y - 5, 180, 30)
        .draw(ColorF(0.0, 0.0, 0.0, 0.5));
    
    timeFont(phaseText).draw(timeDisplayPos, phaseColor);
    
    // 時間の進行バー
    const double timeOfDay = m_dayNightSystem->getTimeOfDay();
    const Vec2 barPos(timeDisplayPos.x, timeDisplayPos.y + 35);
    const Size barSize(160, 8);
    
    // バーの背景
    RectF(barPos, barSize).draw(ColorF(0.2, 0.2, 0.2, 0.7));
    
    // 時間の進行状況
    const double barProgress = timeOfDay;
    RectF(barPos, barSize.x * barProgress, barSize.y).draw(phaseColor);
    
    // バーの枠
    RectF(barPos, barSize).drawFrame(1.0, ColorF(0.8, 0.8, 0.8));
    
    // 夜の警告メッセージ
    if (m_dayNightSystem->isNight())
    {
        const double pulse = std::sin(Scene::Time() * 3.0) * 0.3 + 0.7;
        const ColorF warningColor = ColorF(1.0, 0.3, 0.3, pulse);
        
        Font warningFont(24, Typeface::Bold);
        const String warningText = U"DANGER! Enemies are aggressive!";
        const Vec2 warningPos(Scene::Center().x - 150, 80);
        
        // 警告背景
        RectF(warningPos.x - 10, warningPos.y - 5, 320, 35)
            .draw(ColorF(0.0, 0.0, 0.0, pulse * 0.6));
        
        warningFont(warningText).draw(warningPos, warningColor);
    }
    
    // スターによる時間ボーナス表示
    if (m_starSystem)
    {
        const int starsCollected = m_starSystem->getCollectedStarsCount();
        if (starsCollected > 0)
        {
            const String bonusText = U"star×{} Time Bonus: +{}s"_fmt(
                starsCollected, 
                starsCollected * 10  // STAR_TIME_BONUSと同じ値
            );
            Font(16)(bonusText).draw(timeDisplayPos.x, timeDisplayPos.y + 50, ColorF(1.0, 1.0, 0.5));
        }
    }
    
    // 月や太陽のアイコン描画（装飾的）
    if (m_dayNightSystem->getCurrentPhase() == DayNightSystem::TimePhase::Night)
    {
        // 月のアイコン
        Circle(100, 100, 30).draw(ColorF(0.9, 0.9, 1.0, 0.3));
        Circle(110, 90, 25).draw(ColorF(0.1, 0.1, 0.2, 0.3)); // 月のクレーター
    }
    else if (m_dayNightSystem->getCurrentPhase() == DayNightSystem::TimePhase::Day)
    {
        // 太陽のアイコン
        Circle(100, 100, 25).draw(ColorF(1.0, 0.9, 0.3, 0.5));
        
        // 太陽の光線
        for (int i = 0; i < 8; ++i)
        {
            const double angle = i * Math::TwoPi / 8.0;
            const Vec2 start = Vec2(100, 100) + Vec2(std::cos(angle), std::sin(angle)) * 30;
            const Vec2 end = Vec2(100, 100) + Vec2(std::cos(angle), std::sin(angle)) * 40;
            Line(start, end).draw(3.0, ColorF(1.0, 0.9, 0.3, 0.3));
        }
    }
}


void GameScene::drawDayNightUI() const
{
	if (!m_dayNightSystem) return;

	// UI配置位置の計算
	const Vec2 gaugePos(Scene::Width() - 250, 20);
	const Vec2 infoPos(Scene::Width() - 250, 55);

	// 時間ゲージUI描画
	m_dayNightSystem->drawTimeGaugeUI(gaugePos);

	// 時間情報UI描画
	m_dayNightSystem->drawTimeInfoUI(infoPos);

	// 夜時間中の追加警告表示
	if (m_dayNightSystem->isNight())
	{
		drawNightWarningEffects();
	}

	// 夕暮れ時の注意表示
	else if (m_dayNightSystem->getCurrentPhase() == DayNightSystem::TimePhase::Sunset)
	{
		drawSunsetCautionEffects();
	}
}

// 夜時間中の警告エフェクト
void GameScene::drawNightWarningEffects() const
{
	// 画面端の赤い点滅
	const double pulse = std::sin(Scene::Time() * 6.0) * 0.5 + 0.5;
	const ColorF warningColor(1.0, 0.0, 0.0, pulse * 0.3);

	// 上端の警告バー
	RectF(0, 0, Scene::Width(), 5).draw(warningColor);
	// 下端の警告バー
	RectF(0, Scene::Height() - 5, Scene::Width(), 5).draw(warningColor);
	// 左端の警告バー
	RectF(0, 0, 5, Scene::Height()).draw(warningColor);
	// 右端の警告バー
	RectF(Scene::Width() - 5, 0, 5, Scene::Height()).draw(warningColor);

	// 中央の大きな警告メッセージ（一定間隔で表示）
	const double messageTime = std::fmod(Scene::Time(), 8.0);
	if (messageTime < 2.0)
	{
		const double messageAlpha = messageTime < 1.0 ?
			messageTime : (2.0 - messageTime);

		const String warningMsg = U"NIGHT TIME - Enemies are aggressive!";
		const Vec2 msgPos(Scene::Center().x, 120);

		Font(28, Typeface::Bold)(warningMsg).drawAt(msgPos,
			ColorF(1.0, 0.2, 0.2, messageAlpha * 0.8));

		// 背景の半透明ボックス
		const SizeF msgSize = Font(28)(warningMsg).region().size;
		RectF(msgPos.x - msgSize.x / 2 - 15, msgPos.y - msgSize.y / 2 - 8,
			  msgSize.x + 30, msgSize.y + 16)
			.draw(ColorF(0.0, 0.0, 0.0, messageAlpha * 0.5));
	}
}

//夕暮れ時の注意エフェクト
void GameScene::drawSunsetCautionEffects() const
{
	const double pulse = std::sin(Scene::Time() * 3.0) * 0.3 + 0.4;
	const ColorF cautionColor(1.0, 0.6, 0.0, pulse * 0.4);

	// 上端の注意バー（夜よりも控えめ）
	RectF(0, 0, Scene::Width(), 3).draw(cautionColor);
	RectF(0, Scene::Height() - 3, Scene::Width(), 3).draw(cautionColor);

	// 夕暮れメッセージ（控えめに表示）
	const double messageTime = std::fmod(Scene::Time(), 12.0);
	if (messageTime < 1.5)
	{
		const double messageAlpha = messageTime < 0.75 ?
			messageTime / 0.75 : (1.5 - messageTime) / 0.75;

		const String cautionMsg = U"SUNSET TIME - Be cautious";
		const Vec2 msgPos(Scene::Center().x, 140);

		Font(20)(cautionMsg).drawAt(msgPos,
			ColorF(1.0, 0.7, 0.0, messageAlpha * 0.6));
	}
}
