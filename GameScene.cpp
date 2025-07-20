#include "GameScene.hpp"
#include "CharacterSelectScene.hpp"  // プレイヤーカラー取得のため
#include "SoundManager.hpp"
#include "CollisionSystem.hpp"
// 静的変数の定義
StageNumber GameScene::s_nextStageNumber = StageNumber::Stage1;
StageNumber GameScene::s_gameOverStage = StageNumber::Stage1;  // 追加
int GameScene::s_resultStars = 0;
int GameScene::s_resultCoins = 0;
double GameScene::s_resultTime = 0.0;
PlayerColor GameScene::s_resultPlayerColor = PlayerColor::Green;
bool GameScene::s_shouldLoadNextStage = false;
bool GameScene::s_shouldRetryStage = false;

GameScene::GameScene()
	: m_nextScene(none)
	, m_gameTime(0.0)
	, m_player(nullptr)
	, m_stage(nullptr)
	, m_currentStageNumber(StageNumber::Stage1)
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

	// フォントの初期化
	m_gameFont = Font(24);

	//ゲームBGMを開始
	SoundManager::GetInstance().playBGM(SoundManager::SoundType::BGM_GAME);

	// ★ 重要：シーン遷移フラグを必ずリセット
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
		// リトライ時は必ずゲームオーバーステージを使用
		targetStage = s_gameOverStage;
		s_shouldRetryStage = false;
	}

	// ステージの読み込み
	loadStage(targetStage);

	// ★ 重要：Playerオブジェクトを完全に破棄して再作成
	if (m_player)
	{
		m_player.reset();  // 既存のPlayerを完全に削除
	}
	m_player = nullptr;

	// 選択されたプレイヤーカラーを取得してPlayerオブジェクトを新規作成
	const PlayerColor selectedColor = CharacterSelectScene::getSelectedPlayerColor();
	const Vec2 startPosition = Vec2(200.0, 300.0);

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

	// 【追加】BlockSystemの初期化
	m_blockSystem = std::make_unique<BlockSystem>();
	if (m_blockSystem) {
		m_blockSystem->init();
		m_blockSystem->generateBlocksForStage(m_currentStageNumber);
	}

	// 敵の初期化
	initEnemies();

	// ★ 重要：ゲーム状態の完全初期化
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

	// ゴール判定（プレイヤー更新前にチェック）
	updateGoalCheck();

	if (m_goalReached)
	{
		return;
	}

	// ★ 緊急修正: プレイヤーの更新のみ実行（衝突判定は内部で処理）
	if (m_player)
	{
		m_player->update();
	}

	if (!m_player || !m_player->isExploding())
	{
		// ★ 重要修正: 統一衝突判定システムを復活
		updatePlayerCollisionsUnified();

		// その他のシステム更新
		updateBlockSystemInteractions();
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

	// HUDのコイン位置を計算（固定UI位置）
	const Vec2 hudCoinPosition = Vec2(30 + 80 + 20 + 48 / 2, 30 + 64 + 20 + 48 / 2);

	// コインシステムの更新
	if (m_coinSystem)
	{
		m_coinSystem->update(m_player.get(), hudCoinPosition);
	}

	// 星システムの更新
	if (m_starSystem)
	{
		m_starSystem->update(m_player.get());
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
	// ステージの描画
	if (m_stage)
	{
		m_stage->draw();
	}
	else
	{
		Scene::Rect().draw(ColorF(0.2, 0.4, 0.6));
	}

	// 収集アイテムの描画（ステージの後、プレイヤーの前）
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

		// 【追加】BlockSystemの描画（ステージの後、プレイヤーの前）
		if (m_blockSystem)
		{
			m_blockSystem->draw(cameraOffset);
		}
	}

	// 敵の描画
	drawEnemies();

	drawFireballDestructionEffects();

	if (m_player && m_stage)
	{
		drawPlayerFireballs();
	}

	// プレイヤーの描画（スクロールに対応）
	if (m_player && m_stage)
	{
		const Vec2 playerWorldPos = m_player->getPosition();
		const Vec2 playerScreenPos = m_stage->worldToScreenPosition(playerWorldPos);

		// 爆散中の場合
		if (m_player->isExploding())
		{
			// Player::draw() が爆散エフェクトを描画する
			// 画面座標に調整するため、一時的にプレイヤーの位置を変更
			const Vec2 originalPos = m_player->getPosition();
			const_cast<Player*>(m_player.get())->setPosition(playerScreenPos);
			m_player->draw();
			const_cast<Player*>(m_player.get())->setPosition(originalPos);
		}
		// 死亡状態の場合は何も描画しない
		else if (!m_player->isDead())
		{
			// 通常のプレイヤー描画
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

	// HUDシステムの描画（最前面）
	if (m_hudSystem)
	{
		m_hudSystem->draw();
	}

	// UI要素の描画
	const String timeText = U"Time: {:.1f}s"_fmt(m_gameTime);
	m_gameFont(timeText).draw(10, 10, ColorF(1.0, 1.0, 1.0));

	// 爆散中でない場合のみ操作説明を表示
	if (m_player && !m_player->isExploding())
	{
		const String controlText = U"WASD: Move, SPACE: Jump, S: Duck, F: Fireball, ESC: Title, R: Character Select";
		m_gameFont(controlText).draw(10, Scene::Height() - 40, ColorF(0.8, 0.8, 0.8));
	}
	else if (m_player && m_player->isExploding())
	{
		// 爆散中は特別なメッセージ
		const String explosionText = U"GAME OVER...";
		const Vec2 messagePos = Vec2(Scene::Center().x, Scene::Height() - 100);
		const double alpha = 0.7 + 0.3 * std::sin(Scene::Time() * 4.0);
		Font(32, Typeface::Bold)(explosionText).drawAt(messagePos, ColorF(1.0, 0.3, 0.3, alpha));
	}

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

		// 爆散タイマーの表示
		if (m_player->isExploding())
		{
			const String explosionInfo = U"Explosion Timer: {:.2f}s"_fmt(m_player->getDeathTimer());
			m_gameFont(explosionInfo).draw(10, 70, ColorF(1.0, 0.5, 0.5));
		}

		// その他のデバッグ情報...
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

		// 【追加】BlockSystemのデバッグ情報
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
	}

	if (m_stage)
	{
		const String stageInfo = U"Current Stage: {} ({})"_fmt(
			static_cast<int>(m_currentStageNumber),
			m_stage->getStageName()
		);
		m_gameFont(stageInfo).draw(10, 280, ColorF(0.8, 1.0, 0.8));
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
		Vec2 safePosition = Vec2(300.0, 200.0);
		m_player->setPosition(safePosition);
		m_player->setVelocity(Vec2::Zero());
		// ★ ファイアボール数もリセット
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

	// 【追加】BlockSystemのステージ切り替え
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

	// ステージに応じて多様な敵を配置（密度を大幅に増加）
	switch (m_currentStageNumber)
	{
	case StageNumber::Stage1:
		// Stage1: 基本的な敵（チュートリアル的だが適度な挑戦）
		// 前半エリア (0-1280)
		addEnemy(std::make_unique<NormalSlime>(Vec2(400.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(600.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(800.0, 200.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(1000.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1200.0, 300.0)));

		// 中間エリア (1280-2560)
		addEnemy(std::make_unique<Fly>(Vec2(1400.0, 180.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1600.0, 300.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(1800.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(2000.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(2200.0, 220.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(2400.0, 300.0)));

		// 後半エリア (2560-3840)
		addEnemy(std::make_unique<SlimeBlock>(Vec2(2600.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(2800.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(3000.0, 200.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(3200.0, 300.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(3400.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(3600.0, 300.0)));

		// 終盤エリア (3840-5120)
		addEnemy(std::make_unique<Fly>(Vec2(3800.0, 180.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(4000.0, 300.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(4200.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(4400.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(4600.0, 200.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(4800.0, 300.0)));
		break;

	case StageNumber::Stage2:
		// Stage2: 砂漠ステージ - 中程度の難易度、敵密度アップ
		// 前半エリア (0-1280)
		addEnemy(std::make_unique<NormalSlime>(Vec2(300.0, 300.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(500.0, 300.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(700.0, 250.0)));
		addEnemy(std::make_unique<Fly>(Vec2(900.0, 180.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(1100.0, 300.0)));

		// 中間エリア (1280-2560)
		addEnemy(std::make_unique<SpikeSlime>(Vec2(1300.0, 300.0)));
		addEnemy(std::make_unique<Bee>(Vec2(1500.0, 200.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1700.0, 300.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(1900.0, 250.0)));
		addEnemy(std::make_unique<Fly>(Vec2(2100.0, 180.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(2300.0, 300.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(2500.0, 300.0)));

		// 後半エリア (2560-3840)
		addEnemy(std::make_unique<Bee>(Vec2(2700.0, 200.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(2900.0, 300.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(3100.0, 250.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(3300.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(3500.0, 180.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(3700.0, 300.0)));

		// 終盤エリア (3840-5120)
		addEnemy(std::make_unique<Bee>(Vec2(3900.0, 200.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(4100.0, 300.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(4300.0, 250.0)));
		addEnemy(std::make_unique<Fly>(Vec2(4500.0, 180.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(4700.0, 300.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(4900.0, 300.0)));
		break;

	case StageNumber::Stage3:
		// Stage3: 魔法の森 - 飛行敵が多い、高難易度
		// 前半エリア (0-1280)
		addEnemy(std::make_unique<NormalSlime>(Vec2(300.0, 300.0)));
		addEnemy(std::make_unique<Bee>(Vec2(500.0, 150.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(700.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(900.0, 180.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(1100.0, 250.0)));

		// 中間エリア (1280-2560)
		addEnemy(std::make_unique<Bee>(Vec2(1300.0, 150.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(1500.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(1700.0, 180.0)));
		addEnemy(std::make_unique<Saw>(Vec2(1900.0, 280.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(2100.0, 250.0)));
		addEnemy(std::make_unique<Bee>(Vec2(2300.0, 150.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(2500.0, 300.0)));

		// 後半エリア (2560-3840)
		addEnemy(std::make_unique<Fly>(Vec2(2700.0, 180.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(2900.0, 300.0)));
		addEnemy(std::make_unique<Bee>(Vec2(3100.0, 150.0)));
		addEnemy(std::make_unique<Saw>(Vec2(3300.0, 280.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(3500.0, 250.0)));
		addEnemy(std::make_unique<Fly>(Vec2(3700.0, 180.0)));

		// 終盤エリア (3840-5120) - 最高難易度
		addEnemy(std::make_unique<Bee>(Vec2(3900.0, 150.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(4100.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(4300.0, 280.0)));
		addEnemy(std::make_unique<Fly>(Vec2(4500.0, 180.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(4700.0, 250.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(4900.0, 300.0)));
		break;

	case StageNumber::Stage4:
		// Stage4: 雪山 - 危険な敵の組み合わせ、高密度配置
		// 前半エリア (0-1280)
		addEnemy(std::make_unique<NormalSlime>(Vec2(300.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(500.0, 280.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(700.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(900.0, 180.0)));
		addEnemy(std::make_unique<Bee>(Vec2(1100.0, 160.0)));

		// 中間エリア (1280-2560)
		addEnemy(std::make_unique<SpikeSlime>(Vec2(1300.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(1500.0, 280.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(1700.0, 220.0)));
		addEnemy(std::make_unique<Fly>(Vec2(1900.0, 180.0)));
		addEnemy(std::make_unique<Bee>(Vec2(2100.0, 160.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(2300.0, 300.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(2500.0, 300.0)));

		// 後半エリア (2560-3840)
		addEnemy(std::make_unique<Saw>(Vec2(2700.0, 280.0)));
		addEnemy(std::make_unique<Bee>(Vec2(2900.0, 160.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(3100.0, 300.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(3300.0, 220.0)));
		addEnemy(std::make_unique<Fly>(Vec2(3500.0, 180.0)));
		addEnemy(std::make_unique<Saw>(Vec2(3700.0, 280.0)));

		// 終盤エリア (3840-5120) - 最高難易度
		addEnemy(std::make_unique<SpikeSlime>(Vec2(3900.0, 300.0)));
		addEnemy(std::make_unique<Bee>(Vec2(4100.0, 160.0)));
		addEnemy(std::make_unique<Saw>(Vec2(4300.0, 280.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(4500.0, 220.0)));
		addEnemy(std::make_unique<Fly>(Vec2(4700.0, 180.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(4900.0, 300.0)));
		break;

	case StageNumber::Stage5:
		// Stage5: 石の遺跡 - 非常に高難易度、敵密度最大
		// 前半エリア (0-1280)
		addEnemy(std::make_unique<Saw>(Vec2(300.0, 280.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(500.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(700.0, 280.0)));
		addEnemy(std::make_unique<Bee>(Vec2(900.0, 150.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(1100.0, 300.0)));

		// 中間エリア (1280-2560)
		addEnemy(std::make_unique<Saw>(Vec2(1300.0, 280.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(1500.0, 200.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(1700.0, 300.0)));
		addEnemy(std::make_unique<Fly>(Vec2(1900.0, 170.0)));
		addEnemy(std::make_unique<Bee>(Vec2(2100.0, 150.0)));
		addEnemy(std::make_unique<Saw>(Vec2(2300.0, 280.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(2500.0, 300.0)));

		// 後半エリア (2560-3840)
		addEnemy(std::make_unique<Bee>(Vec2(2700.0, 150.0)));
		addEnemy(std::make_unique<Saw>(Vec2(2900.0, 280.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(3100.0, 300.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(3300.0, 200.0)));
		addEnemy(std::make_unique<Fly>(Vec2(3500.0, 170.0)));
		addEnemy(std::make_unique<Saw>(Vec2(3700.0, 280.0)));

		// 終盤エリア (3840-5120) - 地獄の難易度
		addEnemy(std::make_unique<SpikeSlime>(Vec2(3900.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(4100.0, 280.0)));
		addEnemy(std::make_unique<Bee>(Vec2(4300.0, 150.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(4500.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(4700.0, 280.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(4900.0, 300.0)));
		break;

	case StageNumber::Stage6:
		// Stage6: 最終ステージ - 全敵種混在の最高難易度
		// 前半エリア (0-1280)
		addEnemy(std::make_unique<NormalSlime>(Vec2(300.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(500.0, 280.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(700.0, 300.0)));
		addEnemy(std::make_unique<Bee>(Vec2(900.0, 150.0)));
		addEnemy(std::make_unique<Fly>(Vec2(1100.0, 180.0)));

		// 中間エリア1 (1280-2560)
		addEnemy(std::make_unique<SpikeSlime>(Vec2(1300.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(1500.0, 280.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(1700.0, 200.0)));
		addEnemy(std::make_unique<Bee>(Vec2(1900.0, 150.0)));
		addEnemy(std::make_unique<Fly>(Vec2(2100.0, 180.0)));
		addEnemy(std::make_unique<SlimeBlock>(Vec2(2300.0, 300.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(2500.0, 300.0)));

		// 中間エリア2 (2560-3840)
		addEnemy(std::make_unique<Saw>(Vec2(2700.0, 280.0)));
		addEnemy(std::make_unique<Bee>(Vec2(2900.0, 150.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(3100.0, 300.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(3300.0, 200.0)));
		addEnemy(std::make_unique<Saw>(Vec2(3500.0, 280.0)));
		addEnemy(std::make_unique<Fly>(Vec2(3700.0, 180.0)));

		// 終盤エリア (3840-5120) - 究極の難易度
		addEnemy(std::make_unique<SpikeSlime>(Vec2(3900.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(4100.0, 280.0)));
		addEnemy(std::make_unique<Bee>(Vec2(4300.0, 150.0)));
		addEnemy(std::make_unique<SpikeSlime>(Vec2(4500.0, 300.0)));
		addEnemy(std::make_unique<Saw>(Vec2(4700.0, 280.0)));
		addEnemy(std::make_unique<Ladybug>(Vec2(4900.0, 200.0)));
		addEnemy(std::make_unique<Fly>(Vec2(5000.0, 180.0)));
		break;
	}
}

void GameScene::updateEnemies()
{
	// 敵の更新と死亡した敵の削除
	for (auto& enemy : m_enemies)
	{
		if (enemy && enemy->isActive())
		{
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
			// 敵の描画位置をカメラオフセットで調整
			const Vec2 enemyWorldPos = enemy->getPosition();
			const Vec2 enemyScreenPos = m_stage->worldToScreenPosition(enemyWorldPos);

			// 画面内にある敵のみ描画
			if (enemyScreenPos.x >= -100 && enemyScreenPos.x <= Scene::Width() + 100)
			{
				// 敵のテクスチャを直接画面座標で描画
				const Texture currentTexture = enemy->getCurrentTexture();

				if (currentTexture)
				{
					// 向きに応じて左右反転
					if (enemy->getDirection() == EnemyDirection::Left)
					{
						currentTexture.mirrored().drawAt(enemyScreenPos);
					}
					else
					{
						currentTexture.drawAt(enemyScreenPos);
					}
				}
				else
				{
					// フォールバック：敵タイプに応じた色の円
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
					Circle(enemyScreenPos, 32).draw(fallbackColor);
				}

				// エフェクト描画（画面座標で）
				if (enemy->getState() == EnemyState::Flattened)
				{
					drawEnemyFlattenedEffect(enemyScreenPos, enemy.get());
				}
				else if (enemy->getState() == EnemyState::Hit)
				{
					drawEnemyHitEffect(enemyScreenPos, enemy.get());
				}

				// 特殊エフェクト描画
				switch (enemy->getType())
				{
				case EnemyType::SpikeSlime:
				{
					// SpikeSlimeは常にスパイク警告エフェクト
					if (enemy->isActive() && enemy->isAlive())
					{
						const double time = Scene::Time();
						const double pulseAlpha = 0.2 + 0.15 * std::sin(time * 8.0);
						Circle(enemyScreenPos, 55).draw(ColorF(1.0, 0.0, 0.0, pulseAlpha * 0.4));
						Circle(enemyScreenPos, 50).drawFrame(2.0, ColorF(1.0, 0.2, 0.0, pulseAlpha));
					}
				}
				break;

				case EnemyType::Saw:
				{
					// Sawの危険エフェクト
					const double time = Scene::Time();
					const double pulseAlpha = 0.15 + 0.1 * std::sin(time * 10.0);
					Circle(enemyScreenPos, 60).draw(ColorF(1.0, 0.2, 0.0, pulseAlpha));
				}
				break;

				default:
					break;
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
	const double halfWidth = 50.0;
	const double halfHeight = 50.0;
	const RectF playerRect(playerPos.x - halfWidth, playerPos.y - halfHeight, halfWidth * 2, halfHeight * 2);

	for (auto& enemy : m_enemies)
	{
		if (!enemy->isActive() || !enemy->isAlive()) continue;

		const RectF enemyRect = enemy->getCollisionRect();

		if (playerRect.intersects(enemyRect))
		{
			// 特殊な敵の処理
			handleSpecialEnemyCollision(enemy.get());

			// 踏みつけ判定（Flattened状態の敵は踏めない）
			if (isPlayerStompingEnemy(playerRect, enemyRect) &&
				canStompEnemy(enemy.get()) &&
				enemy->getState() != EnemyState::Flattened)  // ← この条件を追加
			{
				handlePlayerStompEnemy(enemy.get());
			}
			else
			{
				// 横からの衝突（ダメージ） - 無敵状態でない場合のみ
				// ただし、Flattened状態の敵からはダメージを受けない
				if (!m_player->isInvincible() && enemy->getState() != EnemyState::Flattened)
				{
					// 危険な敵かどうかチェック
					bool shouldTakeDamage = false;

					switch (enemy->getType())
					{
					case EnemyType::Saw:
						shouldTakeDamage = true;  // Sawは常に危険
						break;
					case EnemyType::SpikeSlime:
						shouldTakeDamage = true;  // SpikeSlimeは常に危険（スパイクがある）
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
	// プレイヤーが敵の上部にいて、下向きの速度を持っているかチェック
	const Vec2 playerVelocity = m_player->getVelocity();
	const double playerBottom = playerRect.y + playerRect.h;
	const double enemyTop = enemyRect.y;

	// プレイヤーの下端が敵の上部に近く、下向きの速度があるか
	return (playerBottom <= enemyTop + 20.0) && (playerVelocity.y >= 0);
}

void GameScene::handlePlayerStompEnemy(EnemyBase* enemy)
{
	if (!enemy || !enemy->isActive()) return;

	// 敵を踏みつけた処理（シンプルに）
	enemy->onStomp();

	// 敵を踏んだ音を再生
	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_HIT);

	// プレイヤーに小さなジャンプを与える
	if (m_player)
	{
		Vec2 playerVelocity = m_player->getVelocity();
		playerVelocity.y = -300.0;  // 小さなバウンス
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

	// プレイヤーがダメージを受ける
	m_player->hit();

	// ノックバック効果
	const Vec2 playerPos = m_player->getPosition();
	const Vec2 enemyPos = enemy->getPosition();
	const Vec2 knockbackDirection = (playerPos - enemyPos).normalized();

	Vec2 playerVelocity = m_player->getVelocity();
	playerVelocity.x = knockbackDirection.x * 200.0;  // ノックバック力
	playerVelocity.y = -150.0;  // 軽く浮く
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

// ★ 新規メソッド: 統一衝突判定システムを使った衝突処理
void GameScene::updatePlayerCollisionsUnified()
{
	if (!m_player || !m_collisionSystem) return;

	// 新しい統一衝突判定システムで全ての衝突を処理
	m_collisionSystem->resolvePlayerCollisions(m_player.get(), m_stage.get(), m_blockSystem.get());
}

// ★ 新規メソッド: BlockSystemとの相互作用のみを処理
void GameScene::updateBlockSystemInteractions()
{
	if (!m_blockSystem || !m_player) return;

	// BlockSystem特有の処理（ブロックを叩く、コイン獲得など）
	m_blockSystem->update(m_player.get());
}
