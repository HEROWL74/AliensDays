#include "SceneManagers.hpp"
#include "SplashScene.hpp"
#include "TitleScene.hpp"
#include "CharacterSelectScene.hpp"
#include "OptionScene.hpp"
#include "CreditScene.hpp"
#include "GameScene.hpp"
#include "GameOverScene.hpp"
#include "ResultScene.hpp"

SceneManagers::SceneManagers()
	: m_currentScene(nullptr)
	, m_currentSceneType(SceneType::Splash)
	, m_warpCenter(Scene::Center())
{
}

SceneManagers::~SceneManagers()
{
	if (m_currentScene)
	{
		m_currentScene->cleanup();
	}
}

void SceneManagers::init(SceneType initialScene)
{
	m_currentSceneType = initialScene;
	m_currentScene = createScene(initialScene);
	if (m_currentScene)
	{
		m_currentScene->init();
	}
}

void SceneManagers::update()
{
	if (!m_currentScene)
		return;

	if (m_fadeState == FadeState::None)
	{
		m_currentScene->update();

		if (auto next = m_currentScene->getNextScene())
		{
			m_nextSceneType = next.value();

			// シーン遷移の種類を判定
			if (shouldUseSpaceTransition(m_currentSceneType, m_nextSceneType))
			{
				initializeSpaceTransition();
				m_fadeState = FadeState::SpaceTransitionOut;
			}
			else
			{
				m_fadeState = FadeState::NormalFadeOut;
			}
			m_fadeTimer = 0.0;
		}
	}
	else if (m_fadeState == FadeState::SpaceTransitionOut || m_fadeState == FadeState::SpaceTransitionIn)
	{
		updateSpaceTransition();

		if (m_fadeState == FadeState::SpaceTransitionOut)
		{
			m_fadeTimer += Scene::DeltaTime();
			if (m_fadeTimer >= TRANSITION_DURATION * 0.6) // 60%で切り替え
			{
				changeScene(m_nextSceneType);
				m_fadeState = FadeState::SpaceTransitionIn;
			}
		}
		else if (m_fadeState == FadeState::SpaceTransitionIn)
		{
			m_fadeTimer += Scene::DeltaTime();
			if (m_fadeTimer >= TRANSITION_DURATION)
			{
				m_fadeState = FadeState::None;
				m_fadeTimer = 0.0;
				m_spaceParticles.clear();
			}
		}
	}
	else // 通常のフェード
	{
		if (m_fadeState == FadeState::NormalFadeOut)
		{
			m_fadeTimer += Scene::DeltaTime();
			if (m_fadeTimer >= 1.0)
			{
				changeScene(m_nextSceneType);
				m_fadeState = FadeState::NormalFadeIn;
				m_fadeTimer = 0.0;
			}
		}
		else if (m_fadeState == FadeState::NormalFadeIn)
		{
			m_fadeTimer += Scene::DeltaTime();
			if (m_fadeTimer >= 1.0)
			{
				m_fadeState = FadeState::None;
				m_fadeTimer = 0.0;
			}
		}
	}
}

void SceneManagers::draw() const
{
	if (m_currentScene)
	{
		m_currentScene->draw();
	}

	if (m_fadeState == FadeState::SpaceTransitionOut || m_fadeState == FadeState::SpaceTransitionIn)
	{
		drawSpaceTransition();
	}
	else if (m_fadeState == FadeState::NormalFadeOut || m_fadeState == FadeState::NormalFadeIn)
	{
		// 通常のフェード
		double alpha = (m_fadeState == FadeState::NormalFadeOut)
			? m_fadeTimer / 1.0
			: 1.0 - m_fadeTimer / 1.0;

		Rect(Scene::Size()).draw(ColorF(0.0, alpha));
	}
}

void SceneManagers::changeScene(SceneType newScene)
{
	if (m_currentScene)
	{
		m_currentScene->cleanup();
	}

	m_currentSceneType = newScene;
	m_currentScene = createScene(newScene);

	if (m_currentScene)
	{
		m_currentScene->init();
	}
}

SceneType SceneManagers::getCurrentSceneType() const
{
	return m_currentSceneType;
}

std::unique_ptr<SceneBase> SceneManagers::createScene(SceneType sceneType)
{
	switch (sceneType)
	{
	case SceneType::Splash:
		return std::make_unique<SplashScene>();
	case SceneType::Title:
		return std::make_unique<TitleScene>();
	case SceneType::CharacterSelect:
		return std::make_unique<CharacterSelectScene>();
	case SceneType::Option:
		return std::make_unique<OptionScene>();
	case SceneType::Credit:
		return std::make_unique<CreditScene>();
	case SceneType::Game:
		return std::make_unique<GameScene>();
	case SceneType::GameOver:
		return std::make_unique<GameOverScene>();
	case SceneType::Result:
		return std::make_unique<ResultScene>();
	default:
		return nullptr;
	}
}

void SceneManagers::initializeSpaceTransition()
{
	m_spaceParticles.clear();
	m_warpCenter = Scene::Center();
	m_warpIntensity = 0.0;
	createSpaceParticles();
}

void SceneManagers::updateSpaceTransition()
{
	const double progress = m_fadeTimer / TRANSITION_DURATION;

	// ワープ強度の計算（山型カーブ）
	if (progress <= 0.5)
	{
		m_warpIntensity = progress * 2.0;
	}
	else
	{
		m_warpIntensity = 2.0 - progress * 2.0;
	}

	updateSpaceParticles();
}

void SceneManagers::createSpaceParticles()
{
	for (int i = 0; i < PARTICLE_COUNT; ++i)
	{
		SpaceParticle particle;

		// 画面全体をカバーする初期位置（ランダム配置）
		particle.position = Vec2(
		Random(0.0, static_cast<double>(Scene::Width())),
		Random(0.0, static_cast<double>(Scene::Height()))
		);

		// 中央から外側に向かう方向
		const Vec2 direction = (particle.position - m_warpCenter).normalized();
		const double speed = Random(100.0, 400.0);
		particle.velocity = direction * speed;

		particle.life = Random(1.0, 1.8);
		particle.maxLife = particle.life;
		particle.size = Random(1.0, 5.0);

		// 宇宙っぽい色（青、紫、白、ピンク）
		const Array<ColorF> spaceColors = {
			ColorF(0.8, 0.9, 1.0),   // 青白
			ColorF(1.0, 0.8, 1.0),   // ピンク
			ColorF(0.9, 0.8, 1.0),   // 紫
			ColorF(1.0, 1.0, 1.0),   // 白
			ColorF(0.7, 1.0, 1.0),   // シアン
			ColorF(1.0, 1.0, 0.7)    // 淡い黄色
		};

		particle.color = spaceColors[Random(spaceColors.size() - 1)];
		particle.brightness = Random(0.6, 1.0);

		m_spaceParticles.push_back(particle);
	}
}

void SceneManagers::updateSpaceParticles()
{
	const double deltaTime = Scene::DeltaTime();

	for (auto& particle : m_spaceParticles)
	{
		// 位置更新
		particle.position += particle.velocity * deltaTime;

		// ワープ効果による加速
		const Vec2 centerDirection = (particle.position - m_warpCenter).normalized();
		particle.velocity += centerDirection * (m_warpIntensity * 1200.0 * deltaTime);

		// 生存時間更新
		particle.life -= deltaTime;

		// 明度更新（生存時間に応じて減衰）
		particle.brightness = Math::Max(0.0, particle.life / particle.maxLife);

		// サイズ更新（ワープ効果で伸びる）
		particle.size = particle.size + m_warpIntensity * 3.0;
	}

	// 死亡したパーティクルを削除
	m_spaceParticles.erase(
		std::remove_if(m_spaceParticles.begin(), m_spaceParticles.end(),
			[](const SpaceParticle& p) { return p.life <= 0.0; }),
		m_spaceParticles.end()
	);

	// パーティクルが少なくなったら画面全体に補充
	if (m_spaceParticles.size() < PARTICLE_COUNT / 2 && m_warpIntensity > 0.2)
	{
		for (int i = 0; i < 20; ++i)
		{
			SpaceParticle newParticle;

			// 画面端から新しいパーティクルを生成
			const int edge = Random(3); // 0:上, 1:右, 2:下, 3:左
			switch (edge)
			{
			case 0: // 上
				newParticle.position = Vec2(Random(0.0, static_cast<double>(Scene::Width())), -20.0);
				break;
			case 1: // 右
				newParticle.position = Vec2(Scene::Width() + 20.0, Random(0.0, static_cast<double>(Scene::Height())));
				break;
			case 2: // 下
				newParticle.position = Vec2(Random(0.0, static_cast<double>(Scene::Width())), Scene::Height() + 20.0);
				break;
			case 3: // 左
				newParticle.position = Vec2(-20.0, Random(0.0, static_cast<double>(Scene::Height())));
				break;
			}

			const Vec2 direction = (newParticle.position - m_warpCenter).normalized();
			const double speed = Random(200.0, 500.0);
			newParticle.velocity = direction * speed;

			newParticle.life = Random(0.8, 1.2);
			newParticle.maxLife = newParticle.life;
			newParticle.size = Random(1.0, 4.0);

			const Array<ColorF> colors = {
				ColorF(0.8, 0.9, 1.0), ColorF(1.0, 0.8, 1.0),
				ColorF(0.9, 0.8, 1.0), ColorF(1.0, 1.0, 1.0),
				ColorF(0.7, 1.0, 1.0), ColorF(1.0, 1.0, 0.7)
			};
			newParticle.color = colors[Random(colors.size() - 1)];
			newParticle.brightness = Random(0.5, 1.0);

			m_spaceParticles.push_back(newParticle);
		}
	}
}

void SceneManagers::drawSpaceTransition() const
{
	const double progress = m_fadeTimer / TRANSITION_DURATION;

	// 画面全体を覆う暗化（完全に覆う）
	drawFullScreenCover();

	// 星のパーティクル描画
	for (const auto& particle : m_spaceParticles)
	{
		const double alpha = particle.brightness * particle.color.a;
		if (alpha > 0.02)
		{
			const ColorF particleColor = ColorF(
				particle.color.r,
				particle.color.g,
				particle.color.b,
				alpha
			);

			// パーティクル本体
			Circle(particle.position, particle.size).draw(particleColor);

			// 光る効果
			const double glowSize = particle.size * 2.5;
			Circle(particle.position, glowSize).draw(
				ColorF(particleColor.r, particleColor.g, particleColor.b, alpha * 0.4)
			);

			// ワープ効果による光線（より強力に）
			if (m_warpIntensity > 0.3)
			{
				const Vec2 direction = (particle.position - m_warpCenter).normalized();
				const Vec2 trailStart = particle.position - direction * particle.size * 15.0;
				const Vec2 trailEnd = particle.position;

				Line(trailStart, trailEnd).draw(
					particle.size * 0.8,
					ColorF(particleColor.r, particleColor.g, particleColor.b, alpha * 0.7)
				);
			}
		}
	}

	// ワープの中心効果
	drawWarpEffect();

	// 星空背景
	if (progress > 0.2)
	{
		drawStarField();
	}
}

void SceneManagers::drawWarpEffect() const
{
	if (m_warpIntensity <= 0.1) return;

	// 中央のワープホール効果
	for (int ring = 0; ring < 5; ++ring)
	{
		const double ringProgress = m_warpIntensity + ring * 0.2;
		const double radius = 50.0 + ringProgress * 100.0;
		const double alpha = (m_warpIntensity * (5 - ring)) / 10.0;

		if (alpha > 0.02)
		{
			const ColorF ringColor = ColorF(0.6, 0.8, 1.0, alpha);
			Circle(m_warpCenter, radius).drawFrame(3.0, ringColor);
		}
	}

	// 中央の光る点
	const double coreSize = 10.0 + m_warpIntensity * 15.0;
	const double coreAlpha = m_warpIntensity * 0.8;

	Circle(m_warpCenter, coreSize).draw(ColorF(1.0, 1.0, 1.0, coreAlpha));
	Circle(m_warpCenter, coreSize * 1.5).draw(ColorF(0.7, 0.9, 1.0, coreAlpha * 0.5));
}

void SceneManagers::drawFullScreenCover() const
{
	const double progress = m_fadeTimer / TRANSITION_DURATION;

	// 完全な画面カバー（段階的に暗化）
	double coverAlpha;
	if (m_fadeState == FadeState::SpaceTransitionOut)
	{
		// フェードアウト時：徐々に完全に覆う
		coverAlpha = Math::Min(1.0, progress * 1.8);
	}
	else
	{
		// フェードイン時：徐々に透明に
		const double fadeInProgress = (m_fadeTimer - TRANSITION_DURATION * 0.6) / (TRANSITION_DURATION * 0.4);
		coverAlpha = Math::Max(0.0, 1.0 - fadeInProgress * 1.5);
	}

	// 深い宇宙の色で画面全体を覆う
	Scene::Rect().draw(ColorF(0.02, 0.02, 0.1, coverAlpha));
}

bool SceneManagers::shouldUseSpaceTransition(SceneType from, SceneType to) const
{
	// ゲームスタート時：CharacterSelect → Game
	if (from == SceneType::CharacterSelect && to == SceneType::Game)
	{
		return true;
	}

	// リザルト画面に行くとき：Game → Result
	if (from == SceneType::Game && to == SceneType::Result)
	{
		return true;
	}

	// その他は通常のフェード
	return false;
}

void SceneManagers::drawStarField() const
{
	// 静的な星空（パフォーマンス重視）
	const double progress = m_fadeTimer / TRANSITION_DURATION;
	const double starAlpha = Math::Min(0.8, (progress - 0.2) / 0.8); // 20%から表示開始

	if (starAlpha <= 0.0) return;

	// 画面全体に星を配置（シードベース）
	for (int i = 0; i < 200; ++i)
	{
		// 疑似ランダム座標生成（シードベース）
		const double x = std::fmod(i * 127.1, Scene::Width());
		const double y = std::fmod(i * 311.7 + 50.0, Scene::Height());

		// サイズと明度の疑似ランダム
		const double size = 0.3 + std::fmod(i * 43.7, 1.5);
		const double brightness = 0.2 + std::fmod(i * 67.3, 0.8);

		const double finalAlpha = starAlpha * brightness;
		if (finalAlpha > 0.05)
		{
			Circle(x, y, size).draw(ColorF(1.0, 1.0, 1.0, finalAlpha));
		}
	}
}
