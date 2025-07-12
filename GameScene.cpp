#include "GameScene.hpp"
#include "CharacterSelectScene.hpp"  // プレイヤーカラー取得のため

// 静的変数の定義
StageNumber GameScene::s_nextStageNumber = StageNumber::Stage1;
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

	// リザルトシーンからの復帰処理
	StageNumber targetStage = m_currentStageNumber;

	if (s_shouldLoadNextStage && static_cast<int>(s_nextStageNumber) <= 6)
	{
		// 次のステージをロード
		targetStage = s_nextStageNumber;
		s_shouldLoadNextStage = false;
	}
	else if (s_shouldRetryStage)
	{
		// 現在のステージをリトライ（targetStageはそのまま）
		s_shouldRetryStage = false;
	}

	// ステージの読み込み
	loadStage(targetStage);

	// 選択されたプレイヤーカラーを取得してPlayerオブジェクトを作成
	const PlayerColor selectedColor = CharacterSelectScene::getSelectedPlayerColor();
	const Vec2 startPosition = Vec2(200.0, 300.0);

	m_player = std::make_unique<Player>(selectedColor, startPosition);

	// HUDシステムの初期化
	m_hudSystem = std::make_unique<HUDSystem>();
	m_hudSystem->init();
	m_hudSystem->setPlayerCharacter(selectedColor);

	// 収集システムの初期化
	m_coinSystem = std::make_unique<CoinSystem>();
	m_coinSystem->init();
	m_coinSystem->generateCoinsForStage(m_currentStageNumber);

	m_starSystem = std::make_unique<StarSystem>();
	m_starSystem->init();
	m_starSystem->generateStarsForStage(m_currentStageNumber);

	// 敵の初期化
	initEnemies();

	// ゲーム状態の初期化
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

	// ゴール判定（プレイヤー更新前にチェック）
	updateGoalCheck();

	// ゴールに到達していない場合のみ通常更新
	if (!m_goalReached)
	{
		// プレイヤーの更新
		if (m_player)
		{
			m_player->update();
			updatePlayerStageCollision();
		}

		// 収集システムの更新
		updateCollectionSystems();

		// HUDシステムの更新
		m_hudSystem->update();
		updateHUDWithCollectedItems();

		// 敵の更新
		updateEnemies();

		// プレイヤーと敵の衝突判定
		updatePlayerEnemyCollision();

		// 敵とステージの衝突判定
		updateEnemyStageCollision();
	}
	else
	{
		// ゴール到達後の処理
		m_goalTimer += Scene::DeltaTime();
		if (m_goalTimer >= 2.0)  // 2秒後に次のステージへ
		{
			handleGoalReached();
		}
	}

	// ステージの更新（カメラ追従）
	if (m_stage && m_player)
	{
		m_stage->update(m_player->getPosition());
	}

	// ステージ切り替え（数字キー1-6）
	if (Key1.down()) loadStage(StageNumber::Stage1);
	if (Key2.down()) loadStage(StageNumber::Stage2);
	if (Key3.down()) loadStage(StageNumber::Stage3);
	if (Key4.down()) loadStage(StageNumber::Stage4);
	if (Key5.down()) loadStage(StageNumber::Stage5);
	if (Key6.down()) loadStage(StageNumber::Stage6);

	// ESCキーでタイトルに戻る
	if (KeyEscape.down())
	{
		m_nextScene = SceneType::Title;
	}

	// Rキーでキャラクター選択に戻る
	if (KeyR.down())
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

			// ゴール到達時のエフェクト
			Print << U"GOAL REACHED!";

			// プレイヤーを停止
			if (m_player)
			{
				m_player->setVelocity(Vec2::Zero());
			}
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

	// 収集したアイテム数をHUDに反映
	if (m_coinSystem)
	{
		// HUDに現在のコイン数を設定
		m_hudSystem->setCoins(m_coinSystem->getCollectedCoinsCount());
	}

	if (m_starSystem)
	{
		// HUDに現在の星数を設定
		m_hudSystem->setCollectedStars(m_starSystem->getCollectedStarsCount());
		// 総スター数は3つ固定
		m_hudSystem->setTotalStars(3);
	}
}

void GameScene::handleGoalReached()
{
	// リザルトデータを設定
	s_resultPlayerColor = CharacterSelectScene::getSelectedPlayerColor();
	s_resultStars = m_starSystem ? m_starSystem->getCollectedStarsCount() : 0;
	s_resultCoins = m_coinSystem ? m_coinSystem->getCollectedCoinsCount() : 0;
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

	// リザルト画面へ遷移
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
		// フォールバック背景
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
	}

	// 敵の描画
	drawEnemies();

	// プレイヤーの描画（スクロールに対応）
	if (m_player && m_stage)
	{
		// プレイヤーの描画位置をカメラオフセットで調整
		const Vec2 playerWorldPos = m_player->getPosition();
		const Vec2 playerScreenPos = m_stage->worldToScreenPosition(playerWorldPos);

		// プレイヤーを画面座標で描画
		if (m_player->getCurrentTexture())
		{
			const double scale = m_player->getScale();
			const double rotation = m_player->getRotation();
			const ColorF tint = m_player->getTint();
			const Texture currentTexture = m_player->getCurrentTexture();

			// 向きに応じて左右反転
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
			// フォールバック：テクスチャがない場合は円で表示
			const ColorF fallbackColor = m_player->getTint();
			Circle(playerScreenPos, 25).draw(fallbackColor);
		}
	}

	// HUDシステムの描画（最前面）
	if (m_hudSystem)
	{
		m_hudSystem->draw();
	}

	if (m_goalReached)
	{
		const String goalMessage = U"Game Clear！！";
		const String nextMessage = U"Moving to next stage...";

		// 背景
		Scene::Rect().draw(ColorF(0.0, 0.0, 0.0, 0.5));

		// メッセージ
		Font goalFont(48, Typeface::Bold);
		goalFont(goalMessage).drawAt(Scene::Center() - Vec2(0, 30), ColorF(1.0, 1.0, 0.0));
		m_gameFont(nextMessage).drawAt(Scene::Center() + Vec2(0, 30), ColorF(1.0, 1.0, 1.0));
	}

	// UI要素の描画
	// ゲーム時間
	const String timeText = U"Time: {:.1f}s"_fmt(m_gameTime);
	m_gameFont(timeText).draw(10, 10, ColorF(1.0, 1.0, 1.0));

	// 操作説明
	const String controlText = U"WASD: Move, SPACE: Jump, S: Duck, 1-6: Change Stage, ESC: Title, R: Character Select";
	m_gameFont(controlText).draw(10, Scene::Height() - 40, ColorF(0.8, 0.8, 0.8));

	// デバッグ情報（Debug時のみ表示）
#ifdef _DEBUG
	if (m_player)
	{
		const String playerInfo = U"Player: {} - State: {}"_fmt(
			m_player->getColorString(),
			m_player->getStateString()
		);
		m_gameFont(playerInfo).draw(10, 40, ColorF(1.0, 1.0, 0.8));

		const Vec2 playerWorldPos = m_player->getPosition();
		const String positionInfo = U"Position: ({:.0f}, {:.0f})"_fmt(
			playerWorldPos.x,
			playerWorldPos.y
		);
		m_gameFont(positionInfo).draw(10, 70, ColorF(0.8, 0.8, 1.0));

		// プレイヤーの当たり判定を可視化（ワールド座標→画面座標変換）
		const Vec2 playerScreenPos = m_stage->worldToScreenPosition(playerWorldPos);
		const RectF playerRect(playerScreenPos.x - 64, playerScreenPos.y - 64, 128, 128);
		playerRect.drawFrame(2.0, ColorF(0.0, 1.0, 0.0, 0.8));  // 緑の枠

		// カメラ情報
		if (m_stage)
		{
			const Vec2 cameraOffset = m_stage->getCameraOffset();
			const String cameraInfo = U"Camera: ({:.0f}, {:.0f})"_fmt(
				cameraOffset.x,
				cameraOffset.y
			);
			m_gameFont(cameraInfo).draw(10, 130, ColorF(1.0, 0.8, 0.8));
		}

		// 敵の情報表示
		const String enemyInfo = U"Enemies: {}"_fmt(m_enemies.size());
		m_gameFont(enemyInfo).draw(10, 160, ColorF(0.8, 1.0, 0.8));

		// 収集アイテム情報（詳細版）
		if (m_coinSystem && m_starSystem)
		{
			const String collectibleInfo = U"Coins: {} | Stars: {} / {} (Total: {} / Active: {})"_fmt(
				m_coinSystem->getCollectedCoinsCount(),
				m_starSystem->getCollectedStarsCount(),
				3,  // 期待される総スター数
				m_starSystem->getTotalStarsCount(),
				m_starSystem->getActiveStarsCount()
			);
			m_gameFont(collectibleInfo).draw(10, 190, ColorF(0.8, 0.8, 1.0));
		}

		// HUD状態の確認
		if (m_hudSystem)
		{
			const String hudInfo = U"HUD Stars: {} / {}"_fmt(
				m_hudSystem->getCollectedStars(),
				m_hudSystem->getTotalStars()
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
		m_gameFont(stageInfo).draw(10, 100, ColorF(0.8, 1.0, 0.8));
	}
#endif
}

Optional<SceneType> GameScene::getNextScene() const
{
	return m_nextScene;
}

void GameScene::cleanup()
{
	// プレイヤーとステージオブジェクトのクリーンアップ
	m_player.reset();
	m_stage.reset();
	m_enemies.clear();

	// 収集システムのクリーンアップ
	m_hudSystem.reset();
	m_coinSystem.reset();
	m_starSystem.reset();
}

void GameScene::loadStage(StageNumber stageNumber)
{
	m_currentStageNumber = stageNumber;
	m_stage = std::make_unique<Stage>(stageNumber);

	// プレイヤーの位置をステージの安全な場所にリセット
	if (m_player)
	{
		Vec2 safePosition = Vec2(300.0, 200.0);  // ステージ左側の安全な位置
		m_player->setPosition(safePosition);
		m_player->setVelocity(Vec2::Zero());
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

		// デバッグ情報出力
		Print << U"=== Stage " << static_cast<int>(stageNumber) << U" Star Debug Info ===";
		m_starSystem->printStarPositions();
		Print << U"Active stars: " << m_starSystem->getActiveStarsCount();
		Print << U"=======================================";
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

	// ステージに応じて敵を配置
	switch (m_currentStageNumber)
	{
	case StageNumber::Stage1:
		// Stage1: 基本的なNormalSlimeを配置
		addEnemy(std::make_unique<NormalSlime>(Vec2(800.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1200.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1600.0, 300.0)));
		break;
	case StageNumber::Stage2:
		// Stage2: 丘陵地帯に敵を配置
		addEnemy(std::make_unique<NormalSlime>(Vec2(600.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1000.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1400.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1800.0, 300.0)));
		break;
	case StageNumber::Stage3:
		// Stage3: 洞窟ステージ
		addEnemy(std::make_unique<NormalSlime>(Vec2(500.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(900.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1300.0, 300.0)));
		break;
	case StageNumber::Stage4:
		// Stage4: 浮島ステージ
		addEnemy(std::make_unique<NormalSlime>(Vec2(700.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1100.0, 300.0)));
		break;
	case StageNumber::Stage5:
		// Stage5: 山岳ステージ
		addEnemy(std::make_unique<NormalSlime>(Vec2(600.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1000.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1400.0, 300.0)));
		break;
	case StageNumber::Stage6:
		// Stage6: 城ステージ
		addEnemy(std::make_unique<NormalSlime>(Vec2(800.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1200.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(1600.0, 300.0)));
		addEnemy(std::make_unique<NormalSlime>(Vec2(2000.0, 300.0)));
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
		if (enemy->isActive() || enemy->getState() == EnemyState::Flattened)  // Flattened状態も描画
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
					// フォールバック
					const ColorF fallbackColor = ColorF(0.2, 0.8, 0.2);
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
	const double halfWidth = 64.0;
	const double halfHeight = 64.0;
	const RectF playerRect(playerPos.x - halfWidth, playerPos.y - halfHeight, halfWidth * 2, halfHeight * 2);

	for (auto& enemy : m_enemies)
	{
		if (!enemy->isActive() || !enemy->isAlive()) continue;

		const RectF enemyRect = enemy->getCollisionRect();

		if (playerRect.intersects(enemyRect))
		{
			// 踏みつけ判定
			if (isPlayerStompingEnemy(playerRect, enemyRect))
			{
				handlePlayerStompEnemy(enemy.get());
			}
			else
			{
				// 横からの衝突（ダメージ）
				if (m_player->getCurrentState() != PlayerState::Hit)
				{
					handlePlayerHitByEnemy(enemy.get());
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

	// プレイヤーに小さなジャンプを与える
	if (m_player)
	{
		Vec2 playerVelocity = m_player->getVelocity();
		playerVelocity.y = -300.0;  // 小さなバウンス
		m_player->setVelocity(playerVelocity);
	}

	// デバッグ出力
	Print << U"Enemy stomped and destroyed!";
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
	m_hudSystem->subtractLife(1);  // または notifyDamage()

	// デバッグ出力
	Print << U"Player hit by " << enemy->getTypeString();
}

void GameScene::updatePlayerStageCollision()
{
	if (!m_player || !m_stage) return;

	const Vec2 playerPos = m_player->getPosition();  // ワールド座標
	const Vec2 playerVelocity = m_player->getVelocity();

	// プレイヤーの当たり判定矩形（ワールド座標）
	const double halfWidth = 64.0;
	const double halfHeight = 64.0;
	const RectF playerRect(playerPos.x - halfWidth, playerPos.y - halfHeight, halfWidth * 2, halfHeight * 2);

	// ステージの境界チェック
	const double stageWidth = 80 * 64;  // STAGE_WIDTH * BLOCK_SIZE
	const double stageHeight = 17 * 64; // STAGE_HEIGHT * BLOCK_SIZE

	Vec2 constrainedPos = playerPos;
	bool positionChanged = false;

	// X方向の境界チェック
	if (constrainedPos.x - halfWidth < 0)
	{
		constrainedPos.x = halfWidth;
		positionChanged = true;
		if (playerVelocity.x < 0)
		{
			Vec2 newVelocity = playerVelocity;
			newVelocity.x = 0;
			m_player->setVelocity(newVelocity);
		}
	}
	else if (constrainedPos.x + halfWidth > stageWidth)
	{
		constrainedPos.x = stageWidth - halfWidth;
		positionChanged = true;
		if (playerVelocity.x > 0)
		{
			Vec2 newVelocity = playerVelocity;
			newVelocity.x = 0;
			m_player->setVelocity(newVelocity);
		}
	}

	// Y方向の境界チェック（落下防止）
	if (constrainedPos.y + halfHeight > stageHeight)
	{
		constrainedPos.y = stageHeight - halfHeight;
		positionChanged = true;
		Vec2 newVelocity = playerVelocity;
		newVelocity.y = 0;
		m_player->setVelocity(newVelocity);
		m_player->setGrounded(true);
	}

	if (positionChanged)
	{
		m_player->setPosition(constrainedPos);
	}

	// 更新された位置で再度取得
	const Vec2 finalPlayerPos = m_player->getPosition();
	const RectF finalPlayerRect(finalPlayerPos.x - halfWidth, finalPlayerPos.y - halfHeight, halfWidth * 2, halfHeight * 2);

	// ステージの全衝突矩形を取得（ワールド座標）
	const Array<RectF> collisionRects = m_stage->getCollisionRects();

	bool isOnGround = false;

	// 各ブロックとの衝突をチェック（全てワールド座標で処理）
	for (const auto& blockRect : collisionRects)
	{
		if (finalPlayerRect.intersects(blockRect))
		{
			// 衝突方向を判定
			const Vec2 playerCenter = finalPlayerRect.center();
			const Vec2 blockCenter = blockRect.center();
			const Vec2 distance = playerCenter - blockCenter;

			// X方向とY方向の重複を計算
			const double overlapX = (finalPlayerRect.w + blockRect.w) / 2.0 - std::abs(distance.x);
			const double overlapY = (finalPlayerRect.h + blockRect.h) / 2.0 - std::abs(distance.y);

			// より小さい重複の方向で押し戻し
			if (overlapX < overlapY)
			{
				// X方向の衝突
				Vec2 newPos = finalPlayerPos;
				if (distance.x > 0)
				{
					// プレイヤーがブロックの右側
					newPos.x = blockRect.x + blockRect.w + halfWidth;
				}
				else
				{
					// プレイヤーがブロックの左側
					newPos.x = blockRect.x - halfWidth;
				}
				m_player->setPosition(newPos);

				// X方向の速度をリセット
				Vec2 newVelocity = m_player->getVelocity();
				newVelocity.x = 0;
				m_player->setVelocity(newVelocity);
			}
			else
			{
				// Y方向の衝突
				Vec2 newPos = finalPlayerPos;
				Vec2 newVelocity = m_player->getVelocity();

				if (distance.y > 0)
				{
					// プレイヤーがブロックの上側（天井に頭をぶつけた）
					newPos.y = blockRect.y + blockRect.h + halfHeight;
					newVelocity.y = 0;  // 上向きの速度をリセット
				}
				else
				{
					// プレイヤーがブロックの下側（地面に着地）
					newPos.y = blockRect.y - halfHeight;
					newVelocity.y = 0;  // 下向きの速度をリセット
					isOnGround = true;  // 地面に接触
				}

				m_player->setPosition(newPos);
				m_player->setVelocity(newVelocity);
			}

			// 一つの衝突を処理したら次のフレームで再計算
			break;
		}
	}

	// 地面に接触していない場合は空中状態
	if (!isOnGround)
	{
		// プレイヤーが地面から少し離れているかチェック（ワールド座標）
		const Vec2 currentPos = m_player->getPosition();
		const RectF groundCheckRect(currentPos.x - halfWidth, currentPos.y + halfHeight, halfWidth * 2, 5.0);
		bool nearGround = false;

		for (const auto& blockRect : collisionRects)
		{
			if (groundCheckRect.intersects(blockRect))
			{
				nearGround = true;
				break;
			}
		}

		// 地面から離れている場合はジャンプ状態に変更
		if (!nearGround && m_player->getCurrentState() != PlayerState::Jump && m_player->getCurrentState() != PlayerState::Hit)
		{
			m_player->setState(PlayerState::Jump);
			m_player->setGrounded(false);
		}
	}
	else
	{
		// 地面に接触している場合、ジャンプ状態ならアイドル状態に変更
		m_player->setGrounded(true);
		if (m_player->getCurrentState() == PlayerState::Jump)
		{
			m_player->setState(PlayerState::Idle);
		}
	}
}
