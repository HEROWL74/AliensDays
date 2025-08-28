#include "DayNightSystem.hpp"

DayNightSystem::DayNightSystem()
	: m_currentTime(0.0)
	, m_dayDuration(BASE_DAY_DURATION)
	, m_nightStartTime(BASE_DAY_DURATION * 0.7)
	, m_timeSpeed(1.0)
	, m_starsCollected(0)
	, m_currentPhase(TimePhase::Day)
	, m_isPaused(false)
	, m_phaseTransitionTimer(0.0)
	, m_enemyAggressionLevel(1.0)
{
}

void DayNightSystem::init()
{
	m_dayNightShader = HLSL(U"Shaders/DayNight.hlsl", U"PS");
	if (!m_dayNightShader)
	{
		Print << U"Failed to load DayNight shader";
	}

	m_shaderParams = ConstantBuffer<DayNightParams>();

	// UI用のテクスチャ読み込み
	m_gaugeTexture = Texture(U"UI/MNGage.png");
	if (!m_gaugeTexture)
	{
		Print << U"Failed to load gauge texture, using fallback";
	}

	// UI用フォント初期化
	m_uiFont = Font(20, Typeface::Bold);
	m_smallFont = Font(16);

	updateDayDuration();
	m_currentTime = 0.0;
	updatePhase();
}

void DayNightSystem::update()
{
	if (m_isPaused) return;

	const double deltaTime = Scene::DeltaTime();
	m_currentTime += deltaTime * m_timeSpeed;

	if (m_currentTime >= m_dayDuration)
	{
		m_currentTime = 0.0;
	}

	updatePhase();
	updatePhaseTransition();
	updateEnemyAggression();
	updateShaderParams();
}

void DayNightSystem::applyShader(const RenderTexture& source) const
{
	if (!m_dayNightShader)
	{
		source.draw();
		return;
	}

	Graphics2D::SetConstantBuffer(ShaderStage::Pixel, 1, m_shaderParams);
	const ScopedCustomShader2D shader(m_dayNightShader);
	source.draw();
}

void DayNightSystem::onStarCollected()
{
	m_starsCollected++;
	updateDayDuration();

	if (isNight())
	{
		m_currentTime = m_dayDuration * 0.85;
	}
}

void DayNightSystem::updateDayDuration()
{
	m_dayDuration = BASE_DAY_DURATION + (m_starsCollected * STAR_TIME_BONUS);
	m_nightStartTime = m_dayDuration * 0.7 + (m_starsCollected * STAR_NIGHT_DELAY);
}

void DayNightSystem::updatePhase()
{
	TimePhase previousPhase = m_currentPhase;
	const double normalizedTime = m_currentTime / m_dayDuration;

	if (normalizedTime < 0.5)
	{
		m_currentPhase = TimePhase::Day;
	}
	else if (normalizedTime < 0.65)
	{
		m_currentPhase = TimePhase::Sunset;
	}
	else if (normalizedTime < 0.85)
	{
		m_currentPhase = TimePhase::Night;
	}
	else
	{
		m_currentPhase = TimePhase::Dawn;
	}

	if (previousPhase != m_currentPhase)
	{
		m_phaseTransitionTimer = 0.0;
	}
}

void DayNightSystem::updatePhaseTransition()
{
	m_phaseTransitionTimer += Scene::DeltaTime();
	const double transitionDuration = 2.0;
	m_phaseTransitionTimer = Min(m_phaseTransitionTimer, transitionDuration);
}

void DayNightSystem::updateEnemyAggression()
{
	if (isNight())
	{
		const double nightProgress = (m_currentTime / m_dayDuration - 0.65) / 0.2;
		m_enemyAggressionLevel = 1.0 + nightProgress * 1.0;
	}
	else if (m_currentPhase == TimePhase::Sunset)
	{
		m_enemyAggressionLevel = 1.2;
	}
	else
	{
		m_enemyAggressionLevel = 1.0;
	}
}

void DayNightSystem::updateShaderParams()
{
	const double normalizedTime = m_currentTime / m_dayDuration;
	m_shaderParams->timeOfDay = static_cast<float>(normalizedTime);
	m_shaderParams->moonlightIntensity = static_cast<float>(calculateMoonlight());
	m_shaderParams->phaseBlend = static_cast<float>(m_phaseTransitionTimer / 2.0);
	m_shaderParams->starsBonus = static_cast<float>(m_starsCollected * 0.1);
}

double DayNightSystem::calculateMoonlight() const
{
	if (m_currentPhase == TimePhase::Night)
	{
		const double baseLight = 0.3;
		const double pulse = std::sin(m_currentTime * Math::TwoPi / 10.0) * 0.2;
		return baseLight + pulse;
	}
	else if (m_currentPhase == TimePhase::Dawn)
	{
		return 0.2 * (1.0 - m_phaseTransitionTimer / 2.0);
	}
	else if (m_currentPhase == TimePhase::Sunset)
	{
		return 0.1 * (m_phaseTransitionTimer / 2.0);
	}

	return 0.0;
}

double DayNightSystem::getEnemySpeedMultiplier() const
{
	if (isNight())
	{
		return 1.5;
	}
	else if (m_currentPhase == TimePhase::Sunset)
	{
		return 1.2;
	}
	return 1.0;
}

double DayNightSystem::getEnemyAggressionMultiplier() const
{
	return m_enemyAggressionLevel;
}

ColorF DayNightSystem::getEnemyGlowColor() const
{
	if (isNight())
	{
		const double pulse = std::sin(Scene::Time() * 4.0) * 0.3 + 0.7;
		return ColorF(1.0, 0.0, 0.0, pulse * 0.5);
	}
	else if (m_currentPhase == TimePhase::Sunset)
	{
		const double pulse = std::sin(Scene::Time() * 3.0) * 0.2 + 0.3;
		return ColorF(1.0, 0.5, 0.0, pulse * 0.3);
	}
	return ColorF(0.0, 0.0, 0.0, 0.0);
}

ColorF DayNightSystem::getAmbientLight() const
{
	switch (m_currentPhase)
	{
	case TimePhase::Day:
		return ColorF(1.0, 1.0, 1.0, 1.0);
	case TimePhase::Sunset:
		return ColorF(1.0, 0.7, 0.5, 1.0);
	case TimePhase::Night:
		return ColorF(0.3, 0.3, 0.5, 1.0);
	case TimePhase::Dawn:
		return ColorF(0.7, 0.6, 0.8, 1.0);
	default:
		return ColorF(1.0, 1.0, 1.0, 1.0);
	}
}

double DayNightSystem::getTimeUntilNight() const
{
	if (isNight()) return 0.0;

	const double normalizedTime = m_currentTime / m_dayDuration;
	const double nightStartNormalized = 0.65;

	if (normalizedTime < nightStartNormalized)
	{
		return (nightStartNormalized - normalizedTime) * m_dayDuration;
	}

	return (1.0 - normalizedTime + nightStartNormalized) * m_dayDuration;
}

String DayNightSystem::getPhaseText() const
{
	switch (m_currentPhase)
	{
	case TimePhase::Day:
		return U"DAY TIME - Safe";
	case TimePhase::Sunset:
		return U"SUNSET - Caution";
	case TimePhase::Night:
		return U"NIGHT - DANGER!";
	case TimePhase::Dawn:
		return U"DAWN - Calming";
	default:
		return U"";
	}
}

ColorF DayNightSystem::getPhaseColor() const
{
	switch (m_currentPhase)
	{
	case TimePhase::Day:
		return ColorF(1.0, 0.9, 0.3);
	case TimePhase::Sunset:
		return ColorF(1.0, 0.6, 0.3);
	case TimePhase::Night:
		return ColorF(0.8, 0.3, 0.3);
	case TimePhase::Dawn:
		return ColorF(1.0, 0.8, 0.5);
	default:
		return ColorF(1.0, 1.0, 1.0);
	}
}

// ★ 新規追加: 時間ゲージUI描画メソッド
void DayNightSystem::drawTimeGaugeUI(const Vec2& position) const
{
	const Vec2 gaugePos = position;
	const Size gaugeSize(200, 24);
	const double normalizedTime = m_currentTime / m_dayDuration;

	// 背景フレーム描画
	RectF(gaugePos, gaugeSize).draw(ColorF(0.1, 0.1, 0.1, 0.8));
	RectF(gaugePos, gaugeSize).drawFrame(2.0, ColorF(0.6, 0.6, 0.6));

	// ゲージテクスチャがある場合は使用、ない場合は色で表現
	if (m_gaugeTexture)
	{
		// ゲージの満たされた部分の幅を計算
		const double fillWidth = gaugeSize.x * normalizedTime;

		// 太陽部分（左端）- 常に表示
		const double sunSize = 24.0; // ゲージの高さと同じ
		Circle(gaugePos.x + sunSize / 2, gaugePos.y + gaugeSize.y / 2, sunSize / 2 - 2)
			.draw(ColorF(1.0, 0.9, 0.2));

		// 月部分（右端）
		const double moonX = gaugePos.x + gaugeSize.x - sunSize / 2;
		Circle(moonX, gaugePos.y + gaugeSize.y / 2, sunSize / 2 - 2)
			.draw(normalizedTime > 0.65 ? ColorF(0.8, 0.8, 1.0) : ColorF(0.3, 0.3, 0.4));

		// メインゲージバー（太陽と月の間）
		const double barX = gaugePos.x + sunSize;
		const double barWidth = gaugeSize.x - sunSize * 2;
		const double barFillWidth = Math::Max(0.0, fillWidth - sunSize);

		// ゲージバーの背景
		RectF(barX, gaugePos.y, barWidth, gaugeSize.y)
			.draw(ColorF(0.2, 0.2, 0.2, 0.7));

		// 進行度に応じたゲージバーの塗りつぶし
		if (barFillWidth > 0.0)
		{
			const double actualFillWidth = Math::Min(barFillWidth, barWidth);
			RectF(barX, gaugePos.y, actualFillWidth, gaugeSize.y)
				.draw(getPhaseColor());
		}
	}
	else
	{
		// フォールバック: 色でゲージを表現
		const double fillWidth = gaugeSize.x * normalizedTime;
		RectF(gaugePos.x, gaugePos.y, fillWidth, gaugeSize.y).draw(getPhaseColor());
	}

	// 時間帯区切り線を描画
	const Array<double> phaseMarkers = { 0.5, 0.65, 0.85 };
	const Array<String> phaseLabels = { U"Sunset", U"Night", U"Dawn" };

	for (size_t i = 0; i < phaseMarkers.size(); ++i)
	{
		const double markerX = gaugePos.x + gaugeSize.x * phaseMarkers[i];
		Line(markerX, gaugePos.y, markerX, gaugePos.y + gaugeSize.y)
			.draw(2.0, ColorF(1.0, 1.0, 1.0, 0.7));

		// テキストラベル描画
		Font(12)(phaseLabels[i]).drawAt(markerX, gaugePos.y - 15, ColorF(1.0, 1.0, 1.0));
	}

	// 現在位置のインジケーター
	const double indicatorX = gaugePos.x + gaugeSize.x * normalizedTime;

	// ダイヤモンド型インジケーター
	const Array<Vec2> diamond = {
		Vec2(indicatorX, gaugePos.y - 8),
		Vec2(indicatorX + 6, gaugePos.y - 2),
		Vec2(indicatorX, gaugePos.y + 4),
		Vec2(indicatorX - 6, gaugePos.y - 2)
	};

	Polygon(diamond).draw(ColorF(1.0, 1.0, 1.0, 0.9));
	Polygon(diamond).drawFrame(2.0, getPhaseColor());
}

void DayNightSystem::drawTimeInfoUI(const Vec2& position) const
{
	const Vec2 infoPos = position;

	// フェーズテキストの描画
	const String phaseText = getPhaseText();
	const ColorF phaseColor = getPhaseColor();

	// 背景ボックス
	const SizeF textSize = m_uiFont(phaseText).region().size;
	RectF(infoPos.x - 10, infoPos.y - 5, textSize.x + 20, textSize.y + 10)
		.draw(ColorF(0.0, 0.0, 0.0, 0.6));

	m_uiFont(phaseText).draw(infoPos, phaseColor);

	// 夜までの残り時間表示
	const double timeUntilNight = getTimeUntilNight();

	if (timeUntilNight > 0.0)
	{
		const int minutes = static_cast<int>(timeUntilNight / 60.0);
		const int seconds = static_cast<int>(timeUntilNight) % 60;
		const String timeText = U"Until Night: {}:{:02d}"_fmt(minutes, seconds);

		const Vec2 timeTextPos(infoPos.x, infoPos.y + 30);
		const SizeF timeTextSize = m_smallFont(timeText).region().size;

		// 時間表示の背景
		RectF(timeTextPos.x - 5, timeTextPos.y - 3, timeTextSize.x + 10, timeTextSize.y + 6)
			.draw(ColorF(0.0, 0.0, 0.0, 0.5));

		// 緊急度に応じて色を変更
		ColorF timeColor = ColorF(1.0, 1.0, 1.0);
		if (timeUntilNight < 30.0)
		{
			// 30秒以内は赤色で点滅
			const double blink = std::sin(Scene::Time() * 8.0) * 0.5 + 0.5;
			timeColor = ColorF(1.0, 0.3 + blink * 0.4, 0.3 + blink * 0.4);
		}
		else if (timeUntilNight < 60.0)
		{
			// 1分以内は橙色
			timeColor = ColorF(1.0, 0.7, 0.3);
		}

		m_smallFont(timeText).draw(timeTextPos, timeColor);
	}
	else if (isNight())
	{
		// 夜の間は警告メッセージ
		const String warningText = U"DANGEROUS NIGHT!";
		const Vec2 warningPos(infoPos.x, infoPos.y + 30);

		const double pulse = std::sin(Scene::Time() * 4.0) * 0.3 + 0.7;
		const ColorF warningColor(1.0, 0.2, 0.2, pulse);

		const SizeF warningSize = m_smallFont(warningText).region().size;
		RectF(warningPos.x - 5, warningPos.y - 3, warningSize.x + 10, warningSize.y + 6)
			.draw(ColorF(0.2, 0.0, 0.0, pulse * 0.5));

		m_smallFont(warningText).draw(warningPos, warningColor);
	}

	// スターボーナス表示
	if (m_starsCollected > 0)
	{
		const String bonusText = U"Stars x{} Time Extension: +{}sec"_fmt(
			m_starsCollected,
			static_cast<int>(m_starsCollected * STAR_TIME_BONUS)
		);

		const Vec2 bonusPos(infoPos.x, infoPos.y + 55);
		const SizeF bonusSize = Font(14)(bonusText).region().size;

		RectF(bonusPos.x - 3, bonusPos.y - 2, bonusSize.x + 6, bonusSize.y + 4)
			.draw(ColorF(0.0, 0.0, 0.0, 0.4));

		Font(14)(bonusText).draw(bonusPos, ColorF(1.0, 1.0, 0.5));
	}
}
