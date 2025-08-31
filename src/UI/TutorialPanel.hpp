#pragma once
#include <Siv3D.hpp>

class TutorialPanel {
public:
	TutorialPanel() : m_font(20, Typeface::Bold) {} // ★ コンストラクタでフォント初期化

	void show(const String& text) {
		m_text = text;
		m_visible = true;
		m_alpha = 0.0;
	}
	void hide() { m_visible = false; }
	bool visible() const { return m_visible; }

	void update() {
		const double dt = Scene::DeltaTime();
		const double target = m_visible ? 1.0 : 0.0;
		m_alpha += (target - m_alpha) * Min(1.0, dt * 8.0); // ふわっと
	}

	void draw() const {
		if (m_alpha <= 0.01) return;

		const double panelWidth = Math::Min(700.0, Scene::Width() - 40);
		const double panelHeight = 100.0;
		const RectF box(Scene::Width() / 2.0 - panelWidth / 2, 60, panelWidth, panelHeight);

		// 背景
		box.draw(ColorF(0.0, 0.0, 0.0, 0.8 * m_alpha));
		box.drawFrame(3, ColorF(1.0, 1.0, 0.6, m_alpha));

		// 内側フレーム
		box.stretched(-4).drawFrame(1, ColorF(0.8, 0.8, 0.4, m_alpha * 0.5));

		// テキスト描画
		m_font(m_text).drawAt(box.center(), ColorF(1.0, 1.0, 1.0, m_alpha));
	}

private:
	String m_text;
	bool   m_visible = false;
	double m_alpha = 0.0;
	Font   m_font;  
};
