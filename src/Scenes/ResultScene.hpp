#pragma once
#include <Siv3D.hpp>
#include "../Core/SceneBase.hpp"
#include "../Stages/Stage.hpp"
#include "../Player/PlayerColor.hpp"
#include "../Sound/SoundManager.hpp"

class ResultScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_backgroundTexture;
	Texture m_blockTexture;          // 新規: ブロックテクスチャ
	Texture m_buttonTexture;         // 変更: 新しいボタンテクスチャ
	Texture m_starFilledTexture;
	Texture m_starOutlineTexture;
	Texture m_coinTexture;

	// フォント
	Font m_titleFont;
	Font m_headerFont;
	Font m_dataFont;
	Font m_buttonFont;

	// リザルトデータ
	struct ResultData
	{
		StageNumber clearedStage;
		PlayerColor playerColor;
		int collectedStars;
		int totalStars;
		int collectedCoins;
		double clearTime;
		String stageName;
		String playerColorName;
	};

	ResultData m_resultData;

	// 背景ブロック構造体
	struct BackgroundBlock
	{
		Vec2 position;
		double rotation;
		double scale;
		double alpha;
		double rotationSpeed;
		double bobSpeed;
		double bobPhase;
	};

	Array<BackgroundBlock> m_backgroundBlocks;

	// パーティクル構造体
	struct Particle
	{
		Vec2 position;
		Vec2 velocity;
		double life;
		double maxLife;
		double rotation;
		double rotationSpeed;
		ColorF color;
	};

	Array<Particle> m_particles;

	// 花火構造体（3つ星時の特別エフェクト）
	struct Firework
	{
		Vec2 position;
		double delay;
		double timer;
		bool active;
		bool exploded;
	};

	Array<Firework> m_fireworks;

	// ボタン関連
	enum class ButtonAction
	{
		NextStage,
		Retry,
		BackToTitle
	};

	struct ButtonData
	{
		String text;
		RectF rect;
		ButtonAction action;
		bool enabled;
	};

	Array<ButtonData> m_buttons;
	int m_selectedButton;
	bool m_buttonHovered;

	// アニメーション関連
	double m_animationTimer;
	double m_starAnimationDelay;
	bool m_showStarAnimation;
	double m_blockAnimationTimer;      // 新規: ブロックアニメーション用
	double m_titlePulseTimer;          // 新規: タイトル脈動用
	double m_fireworksTimer;           // 新規: 花火用

	Optional<SceneType> m_nextScene;

public:
	ResultScene();
	~ResultScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

	// リザルトデータ設定
	void setResultData(StageNumber stage, PlayerColor playerColor,
					   int stars, int totalStars, int coins, double time);

private:
	// 初期化メソッド
	void setupBlocks();                // 新規: 背景ブロック設定
	void setupButtons();
	void setupParticles();             // 新規: パーティクル初期化
	void setupFireworks();             // 新規: 花火初期化
	void loadTextures();

	// 更新メソッド
	void updateInput();
	void updateAnimations();
	void updateParticles();            // 新規: パーティクル更新
	void updateFireworks();            // 新規: 花火更新

	// パーティクル生成メソッド
	void createParticle(const Vec2& position);
	void createFireworkExplosion(const Vec2& position);

	// 描画メソッド
	void drawBackground() const;
	void drawBackgroundBlocks() const; // 新規: 背景ブロック描画
	void drawMainPanel() const;        // 新規: メインパネル描画
	void drawTitle() const;
	void drawStageInfo() const;
	void drawStarResult() const;
	void drawGameStats() const;
	void drawButtons() const;
	void drawParticles() const;        // 新規: パーティクル描画
	void drawFireworks() const;        // 新規: 花火描画
	void drawStarGlitter(const Vec2& starPos, int starIndex) const; // 新規: 星キラキラ

	// ユーティリティ
	void executeButton(int buttonIndex);
	String getPlayerColorName(PlayerColor color) const;
	ColorF getPlayerColorTint(PlayerColor color) const; // 新規: プレイヤーカラーのティント
	String formatTime(double seconds) const;
	ColorF getStarColor(int starIndex) const;
	String getStarRating() const;
	String getPerformanceRating() const;   // 新規: パフォーマンス評価
};
