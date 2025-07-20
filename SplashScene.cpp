#include "SplashScene.hpp"
#include "SoundManager.hpp"

SplashScene::SplashScene()
	: m_timer(0.0)
	, m_totalDuration(7.0)
	, m_currentPhase(Phase::FadeIn)
	, m_explosionTimer(0.0)
	, m_textRevealTimer(0.0)
	, m_skipRequested(false)
	, m_nextScene(none)
{
}

void SplashScene::init()
{
	// テクスチャの読み込み
	m_bombTexture = Texture(U"Sprites/Tiles/bomb.png");
	m_bombActiveTexture = Texture(U"Sprites/Tiles/bomb_active.png");

	// フォントの初期化
	m_companyFont = Font(48, Typeface::Bold);
	m_poweredByFont = Font(20);

	// 初期化
	m_timer = 0.0;
	m_currentPhase = Phase::FadeIn;
	m_explosionTimer = 0.0;
	m_textRevealTimer = 0.0;
	m_skipRequested = false;
	m_nextScene = none;

	// パーティクル配列の初期化
	m_particles.clear();
	m_particleColors.clear();
	m_particleLifetime.clear();
	m_particleVelocities.clear();
	m_particleSizes.clear();

	// 衝撃波用配列の初期化
	m_shockwaves.clear();
	m_shockwaveTimers.clear();

	// BGMは開始しない（無音のスプラッシュ）
}

void SplashScene::update()
{
	// スキップ処理
	if (KeySpace.down() || KeyEnter.down() || MouseL.down())
	{
		m_skipRequested = true;
	}

	if (m_skipRequested)
	{
		m_nextScene = SceneType::Title;
		return;
	}

	// タイマー更新
	m_timer += Scene::DeltaTime();

	// フェーズ更新
	switch (m_currentPhase)
	{
	case Phase::FadeIn:
		updateFadeIn();
		break;
	case Phase::BombIdle:
		updateBombIdle();
		break;
	case Phase::BombActive:
		updateBombActive();
		break;
	case Phase::Explosion:
		updateExplosion();
		break;
	case Phase::TextReveal:
		updateTextReveal();
		break;
	case Phase::Hold:
		updateHold();
		break;
	case Phase::FadeOut:
		updateFadeOut();
		break;
	}

	// パーティクル更新
	updateParticles();
	updateShockwaves();

	// 全体時間でのスキップ
	if (m_timer >= m_totalDuration)
	{
		m_nextScene = SceneType::Title;
	}
}

void SplashScene::draw() const
{
	// 背景
	Scene::Rect().draw(getBackgroundColor());

	// フェーズに応じた描画
	const Vec2 bombPos = getBombPosition();
	const double bombScale = getBombScale();
	const double progress = getPhaseProgress();

	switch (m_currentPhase)
	{
	case Phase::FadeIn:
	{
		const double alpha = progress;
		drawBomb(bombPos, bombScale);
		Scene::Rect().draw(ColorF(0.0, 1.0 - alpha));
	}
	break;

	case Phase::BombIdle:
	{
		// 爆弾をゆっくりと回転
		const double rotation = m_timer * 0.5;
		drawBomb(bombPos, bombScale, rotation);
	}
	break;

	case Phase::BombActive:
	{
		// 点滅効果
		const double blinkRate = 8.0;
		const bool isActive = std::fmod(m_timer * blinkRate, 2.0) < 1.0;
		const double rotation = m_timer * 2.0;

		if (isActive)
		{
			drawBombActive(bombPos, bombScale, rotation);
		}
		else
		{
			drawBomb(bombPos, bombScale, rotation);
		}
	}
	break;

	case Phase::Explosion:
	{
		// 衝撃波の描画
		drawShockwaves();

		// 爆発エフェクト
		drawParticles();

		// 爆発の光（複数層で描画）
		const double explosionAlpha = 1.0 - (m_explosionTimer / 1.0);
		if (explosionAlpha > 0.0)
		{
			// メイン爆発光
			const double explosionRadius = 400.0 * (m_explosionTimer / 1.0);
			Circle(bombPos, explosionRadius).draw(ColorF(1.0, 0.6, 0.1, explosionAlpha * 0.3));
			Circle(bombPos, explosionRadius * 0.7).draw(ColorF(1.0, 0.8, 0.3, explosionAlpha * 0.5));
			Circle(bombPos, explosionRadius * 0.4).draw(ColorF(1.0, 1.0, 0.8, explosionAlpha * 0.8));

			// 内側の白い光
			Circle(bombPos, explosionRadius * 0.2).draw(ColorF(1.0, 1.0, 1.0, explosionAlpha * 0.9));
		}
	}
	break;

	case Phase::TextReveal:
	{
		drawParticles();
		const double textAlpha = progress;
		drawCompanyText(textAlpha);
	}
	break;

	case Phase::Hold:
	{
		drawCompanyText(1.0);
		drawPoweredByText(1.0);
	}
	break;

	case Phase::FadeOut:
	{
		drawCompanyText(1.0);
		drawPoweredByText(1.0);
		Scene::Rect().draw(ColorF(0.0, progress));
	}
	break;
	}

	// スキップ指示（最初の1秒後に表示）
	if (m_timer > 1.0)
	{
		const String skipText = U"Press any key to skip";
		const double skipAlpha = 0.6 + 0.4 * std::sin(m_timer * 3.0);
		m_poweredByFont(skipText).draw(Scene::Width() - 200, Scene::Height() - 30, ColorF(1.0, 1.0, 1.0, skipAlpha));
	}
}

Optional<SceneType> SplashScene::getNextScene() const
{
	return m_nextScene;
}

void SplashScene::cleanup()
{
	// リソースのクリーンアップ（BGMは停止しない）
	m_particles.clear();
	m_particleColors.clear();
	m_particleLifetime.clear();
	m_particleVelocities.clear();
	m_particleSizes.clear();
	m_shockwaves.clear();
	m_shockwaveTimers.clear();
}

// 以下、既存のメソッドは変更なし...
void SplashScene::updateFadeIn()
{
	if (m_timer >= 1.0)
	{
		m_currentPhase = Phase::BombIdle;
		m_timer = 0.0;
	}
}

void SplashScene::updateBombIdle()
{
	if (m_timer >= 1.5)
	{
		m_currentPhase = Phase::BombActive;
		m_timer = 0.0;
	}
}

void SplashScene::updateBombActive()
{
	if (m_timer >= 1.5)
	{
		m_currentPhase = Phase::Explosion;
		m_timer = 0.0;
		m_explosionTimer = 0.0;
		createExplosionParticles();
		createShockwaves();
	}
}

void SplashScene::updateExplosion()
{
	m_explosionTimer += Scene::DeltaTime();

	if (m_timer >= 1.0)
	{
		m_currentPhase = Phase::TextReveal;
		m_timer = 0.0;
		m_textRevealTimer = 0.0;
	}
}

void SplashScene::updateTextReveal()
{
	m_textRevealTimer += Scene::DeltaTime();

	if (m_timer >= 1.0)
	{
		m_currentPhase = Phase::Hold;
		m_timer = 0.0;
	}
}

void SplashScene::updateHold()
{
	if (m_timer >= 1.5)
	{
		m_currentPhase = Phase::FadeOut;
		m_timer = 0.0;
	}
}

void SplashScene::updateFadeOut()
{
	if (m_timer >= 0.5)
	{
		m_nextScene = SceneType::Title;
	}
}

void SplashScene::createExplosionParticles()
{
	const Vec2 center = getBombPosition();
	const int particleCount = 80;

	for (int i = 0; i < particleCount; ++i)
	{
		const double angle = Random(0.0, Math::TwoPi);
		const double speed = Random(150.0, 500.0);
		const Vec2 velocity = Vec2(std::cos(angle), std::sin(angle)) * speed;

		const Vec2 startPos = center + Vec2(Random(-30.0, 30.0), Random(-30.0, 30.0));

		m_particles.push_back(startPos);

		ColorF color;
		const double colorType = Random(0.0, 1.0);
		if (colorType < 0.4)
		{
			color = ColorF(Random(0.8, 1.0), Random(0.3, 0.7), Random(0.0, 0.2));
		}
		else if (colorType < 0.7)
		{
			color = ColorF(Random(0.9, 1.0), Random(0.8, 1.0), Random(0.0, 0.3));
		}
		else
		{
			color = ColorF(Random(0.8, 1.0), Random(0.0, 0.3), Random(0.0, 0.2));
		}

		m_particleColors.push_back(color);
		m_particleLifetime.push_back(Random(1.0, 2.5));
		m_particleVelocities.push_back(velocity);
		m_particleSizes.push_back(Random(2.0, 8.0));
	}
}

void SplashScene::createShockwaves()
{
	const Vec2 center = getBombPosition();

	for (int i = 0; i < 3; ++i)
	{
		m_shockwaves.push_back(center);
		m_shockwaveTimers.push_back(i * 0.1);
	}
}

void SplashScene::updateParticles()
{
	const double deltaTime = Scene::DeltaTime();

	for (size_t i = 0; i < m_particles.size(); )
	{
		m_particleLifetime[i] -= deltaTime;

		if (m_particleLifetime[i] <= 0.0)
		{
			m_particles.erase(m_particles.begin() + i);
			m_particleColors.erase(m_particleColors.begin() + i);
			m_particleLifetime.erase(m_particleLifetime.begin() + i);
			m_particleVelocities.erase(m_particleVelocities.begin() + i);
			m_particleSizes.erase(m_particleSizes.begin() + i);
		}
		else
		{
			m_particles[i] += m_particleVelocities[i] * deltaTime;
			m_particleVelocities[i].y += 300.0 * deltaTime;
			m_particleVelocities[i] *= 0.98;
			m_particleSizes[i] *= 0.99;
			++i;
		}
	}
}

void SplashScene::updateShockwaves()
{
	const double deltaTime = Scene::DeltaTime();

	for (size_t i = 0; i < m_shockwaveTimers.size(); )
	{
		m_shockwaveTimers[i] += deltaTime;

		if (m_shockwaveTimers[i] > 1.0)
		{
			m_shockwaves.erase(m_shockwaves.begin() + i);
			m_shockwaveTimers.erase(m_shockwaveTimers.begin() + i);
		}
		else
		{
			++i;
		}
	}
}

void SplashScene::drawParticles() const
{
	for (size_t i = 0; i < m_particles.size(); ++i)
	{
		const double alpha = m_particleLifetime[i] / 2.5;
		const ColorF color = m_particleColors[i];
		const Vec2 pos = m_particles[i];
		const double size = m_particleSizes[i];

		Circle(pos, size).draw(ColorF(color.r, color.g, color.b, alpha));
		Circle(pos, size * 1.5).draw(ColorF(color.r, color.g, color.b, alpha * 0.3));
	}
}

void SplashScene::drawShockwaves() const
{
	for (size_t i = 0; i < m_shockwaves.size(); ++i)
	{
		const double timer = m_shockwaveTimers[i];
		if (timer < 0.0) continue;

		const Vec2 center = m_shockwaves[i];
		const double radius = timer * 800.0;
		const double alpha = Math::Max(0.0, 1.0 - timer);

		Circle(center, radius).drawFrame(8.0, ColorF(1.0, 0.8, 0.4, alpha * 0.6));
		Circle(center, radius).drawFrame(4.0, ColorF(1.0, 1.0, 0.8, alpha * 0.8));
	}
}

void SplashScene::drawBomb(const Vec2& pos, double scale, double rotation) const
{
	if (m_bombTexture)
	{
		m_bombTexture.scaled(scale).rotated(rotation).drawAt(pos);
	}
	else
	{
		Circle(pos, 30 * scale).draw(ColorF(0.2, 0.2, 0.2));
		Circle(pos, 25 * scale).draw(ColorF(0.4, 0.4, 0.4));
	}
}

void SplashScene::drawBombActive(const Vec2& pos, double scale, double rotation) const
{
	if (m_bombActiveTexture)
	{
		m_bombActiveTexture.scaled(scale).rotated(rotation).drawAt(pos);
	}
	else
	{
		Circle(pos, 30 * scale).draw(ColorF(0.8, 0.2, 0.2));
		Circle(pos, 25 * scale).draw(ColorF(1.0, 0.4, 0.4));
	}
}

void SplashScene::drawCompanyText(double alpha) const
{
	const String companyText = U"HerowlGames";

	const double time = Scene::Time();
	const double bounce = std::sin(time * 2.0) * 5.0;
	const double scale = 1.0 + std::sin(time * 1.5) * 0.1;

	const Vec2 basePos = Vec2(Scene::Center().x, Scene::Center().y + 100);
	const Vec2 textPos = basePos + Vec2(0, bounce);

	for (int i = 3; i >= 0; --i)
	{
		const double glowAlpha = alpha * (0.2 + i * 0.1);
		const double glowSize = scale * (1.0 + i * 0.05);
		const Vec2 glowOffset = Vec2(i, i);

		ColorF glowColor;
		if (i == 0)
		{
			glowColor = ColorF(1.0, 1.0, 1.0, alpha);
		}
		else
		{
			glowColor = ColorF(0.8, 0.8, 1.0, glowAlpha);
		}

		const auto scaledFont = Font(static_cast<int>(48 * glowSize), Typeface::Bold);
		scaledFont(companyText).drawAt(textPos + glowOffset, glowColor);
	}
}

void SplashScene::drawPoweredByText(double alpha) const
{
	const String poweredByText = U"Powered by Siv3D";
	const double time = Scene::Time();
	const double sway = std::sin(time * 1.0) * 2.0;
	const Vec2 textPos = Vec2(Scene::Center().x + sway, Scene::Center().y + 150);

	m_poweredByFont(poweredByText).drawAt(textPos, ColorF(0.8, 0.8, 0.8, alpha));
}

double SplashScene::getPhaseProgress() const
{
	const double phaseDuration = 1.0;
	return Math::Clamp(m_timer / phaseDuration, 0.0, 1.0);
}

Vec2 SplashScene::getBombPosition() const
{
	return Vec2(Scene::Center().x, Scene::Center().y - 50);
}

double SplashScene::getBombScale() const
{
	switch (m_currentPhase)
	{
	case Phase::FadeIn:
		return 1.0 + 0.2 * std::sin(m_timer * 2.0);
	case Phase::BombIdle:
		return 1.0 + 0.1 * std::sin(m_timer * 1.5);
	case Phase::BombActive:
		return 1.2 + 0.3 * std::sin(m_timer * 12.0);
	default:
		return 1.0;
	}
}

ColorF SplashScene::getBackgroundColor() const
{
	switch (m_currentPhase)
	{
	case Phase::FadeIn:
	case Phase::BombIdle:
		return ColorF(0.1, 0.1, 0.2);
	case Phase::BombActive:
	{
		const double flash = 0.1 + 0.1 * std::sin(m_timer * 15.0);
		return ColorF(0.1 + flash, 0.1, 0.2);
	}
	case Phase::Explosion:
	{
		const double flash = 1.0 - (m_explosionTimer / 1.0);
		return ColorF(0.1 + flash * 0.5, 0.1 + flash * 0.3, 0.2);
	}
	default:
		return ColorF(0.1, 0.1, 0.2);
	}
}
