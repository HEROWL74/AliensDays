#pragma once
#include <Siv3D.hpp>

class SoundManager
{
public:
	enum class SoundType
	{
		BGM_TITLE,
		BGM_GAME,
		BGM_CLEAR,
		SFX_SELECT,
		SFX_BUMP,
		SFX_HURT,
		SFX_JUMP,
		SFX_COIN,
		SFX_DISAPPEAR,
		SFX_BREAK_BLOCK,
		SFX_HIT
	};

	static SoundManager& GetInstance()
	{
		static SoundManager instance;
		return instance;
	}

	void init();
	void cleanup();

	// BGM制御
	void playBGM(SoundType bgm, bool loop = true);
	void stopBGM();
	void pauseBGM();
	void resumeBGM();

	// BGM状態チェック
	bool isBGMPlaying() const;
	bool isBGMPlaying(SoundType bgm) const;

	// SE制御
	void playSE(SoundType se);
	void stopSE(SoundType se);
	void stopAllSE();

	// ボリューム制御
	void setBGMVolume(double volume);  // 0.0 ~ 1.0
	void setSEVolume(double volume);   // 0.0 ~ 1.0
	void setMasterVolume(double volume); // 0.0 ~ 1.0

	double getBGMVolume() const { return m_bgmVolume; }
	double getSEVolume() const { return m_seVolume; }
	double getMasterVolume() const { return m_masterVolume; }

private:
	SoundManager()
		: m_currentBGMType(SoundType::BGM_TITLE)
		, m_bgmVolume(0.7)
		, m_seVolume(0.8)
		, m_masterVolume(0.5)
		, m_isBGMPlaying(false)
		, m_isBGMPaused(false)
	{
	}

	~SoundManager() = default;
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

	// サウンドデータ管理
	HashTable<SoundType, Audio> m_audioMap;
	HashTable<SoundType, String> m_soundPaths;

	// BGM状態管理
	SoundType m_currentBGMType;
	bool m_isBGMPlaying;
	bool m_isBGMPaused;

	// ボリューム設定
	double m_bgmVolume;
	double m_seVolume;
	double m_masterVolume;

	// 内部メソッド
	void loadSound(SoundType type, const String& filePath);
	void updateVolumes();
	double calculateVolume(double baseVolume) const;
	void setupSoundPaths();
};
