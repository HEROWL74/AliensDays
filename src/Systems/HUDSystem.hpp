#pragma once
#include <Siv3D.hpp>
#include "../Player/PlayerColor.hpp"

class HUDSystem
{
public:
	// ハートの状態
	enum class HeartState
	{
		Full,      // 満タン
		Half,      // 半分
		Empty      // 空
	};

	HUDSystem();
	~HUDSystem() = default;

	void init();
	void update();
	void draw() const;


	// ライフ管理
	void setMaxLife(int maxLife) { m_maxLife = maxLife; }
	void setCurrentLife(int currentLife);
	void addLife(int amount);
	void subtractLife(int amount);
	int getCurrentLife() const { return m_currentLife; }
	int getMaxLife() const { return m_maxLife; }

	// プレイヤーアイコン設定
	void setPlayerCharacter(PlayerColor color);
	void setPlayerCharacter(int characterIndex);

	// コイン管理
	void setCoins(int coins) { m_coins = coins; }
	void addCoins(int amount) { m_coins += amount; }
	void subtractCoins(int amount) { m_coins = Math::Max(0, static_cast<float>(m_coins) - amount); }
	int getCoins() const { return m_coins; }

	// スター管理（新機能）
	void setCollectedStars(int collected) { m_collectedStars = collected; }
	void setTotalStars(int total) { m_totalStars = total; }
	int getCollectedStars() const { return m_collectedStars; }
	int getTotalStars() const { return m_totalStars; }

	// HUD表示設定
	void setPosition(const Vec2& position) { m_hudPosition = position; }
	void setVisible(bool visible) { m_visible = visible; }
	bool isVisible() const { return m_visible; }

	// ダメージ通知
	void notifyDamage();

	//ファイアボール残機管理
	void setFireballCount(int remaining) { m_remainingFireballs = remaining; }
	int getFireballCount() const { return m_remainingFireballs; }

private:
	// テクスチャハンドル
	struct HeartTextures
	{
		Texture full;
		Texture half;
		Texture empty;
	} m_heartTextures;

	struct PlayerIconTextures
	{
		Texture beige;
		Texture green;
		Texture pink;
		Texture purple;
		Texture yellow;
	} m_playerIconTextures;

	struct CoinTextures
	{
		Texture coin;
		Array<Texture> numbers; // 0-9の数字
	} m_coinTextures;

	// スターテクスチャ（新機能）
	struct StarTextures
	{
		Texture starOutline;    // star_outline_depth.png
		Texture starFilled;     // star.png
	} m_starTextures;

	// ゲーム状態
	int m_maxLife;                  // 最大ライフ (通常は6 = 3ハート × 2)
	int m_currentLife;              // 現在のライフ
	int m_coins;                    // コイン数
	PlayerColor m_currentPlayerColor; // 現在のプレイヤーキャラクター

	// スターの状態（新機能）
	int m_collectedStars;           // 収集済みスター数
	int m_totalStars;               // 総スター数（通常は3）

	// HUD表示設定
	Vec2 m_hudPosition;             // HUDの基準位置
	bool m_visible;                 // HUD表示フラグ

	// 新追加：ハート揺れアニメーション用変数
	int m_previousLife;             // 前フレームのライフ（ダメージ検出用）
	double m_heartShakeTimer;       // ハート揺れタイマー
	double m_heartShakeIntensity;   // ハート揺れ強度 (0.0〜1.0)
	double m_heartShakePhase;       // ハート揺れ位相

	// ハート揺れ関数定数
	static constexpr double HEART_SHAKE_DURATION = 1.0;  // 揺れ持続時間（秒）
	static constexpr double HEART_SHAKE_AMOUNT = 3.0;    // 揺れの強度（ピクセル）

	// レイアウト定数（拡張版）
	static constexpr int HEART_SIZE = 64;           // 32 → 64に拡大
	static constexpr int PLAYER_ICON_SIZE = 80;     // 48 → 80に拡大
	static constexpr int COIN_ICON_SIZE = 48;       // 32 → 48に拡大
	static constexpr int NUMBER_SIZE = 36;          // 24 → 36に拡大
	static constexpr int STAR_SIZE = 48;            // スターのサイズ（新機能）
	static constexpr int ELEMENT_SPACING = 20;      // 16 → 20に拡大

	// フォント
	Font m_numberFont;

	// ヘルパー関数
	void loadTextures();
	void drawHearts() const;
	void drawPlayerIcon() const;
	void drawCoins() const;
	void drawStars() const;  // スター描画関数（新機能）
	void drawFireballs() const;

	void drawNumber(int number, const Vec2& position) const;
	HeartState getHeartState(int heartIndex) const;
	Texture getPlayerIconTexture() const;
	String getCharacterColorName(PlayerColor color) const;

	int m_remainingFireballs; // ファイアボール残数
};
