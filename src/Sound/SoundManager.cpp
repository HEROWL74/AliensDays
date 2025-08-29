#include "SoundManager.hpp"

void SoundManager::init()
{
	// サウンドファイルパスの設定
	setupSoundPaths();

	// BGMの読み込み
	loadSound(SoundType::BGM_TITLE, m_soundPaths[SoundType::BGM_TITLE]);
	loadSound(SoundType::BGM_GAME, m_soundPaths[SoundType::BGM_GAME]);
	loadSound(SoundType::BGM_CLEAR, m_soundPaths[SoundType::BGM_CLEAR]);

	// SEの読み込み
	loadSound(SoundType::SFX_SELECT, m_soundPaths[SoundType::SFX_SELECT]);
	loadSound(SoundType::SFX_BUMP, m_soundPaths[SoundType::SFX_BUMP]);
	loadSound(SoundType::SFX_HURT, m_soundPaths[SoundType::SFX_HURT]);
	loadSound(SoundType::SFX_JUMP, m_soundPaths[SoundType::SFX_JUMP]);
	loadSound(SoundType::SFX_COIN, m_soundPaths[SoundType::SFX_COIN]);
	loadSound(SoundType::SFX_DISAPPEAR, m_soundPaths[SoundType::SFX_DISAPPEAR]);
	loadSound(SoundType::SFX_BREAK_BLOCK, m_soundPaths[SoundType::SFX_BREAK_BLOCK]);
	loadSound(SoundType::SFX_HIT, m_soundPaths[SoundType::SFX_HIT]);

	// 初期ボリューム設定
	updateVolumes();

}

void SoundManager::cleanup()
{
	// BGM停止
	stopBGM();

	// 全SE停止
	stopAllSE();

	// オーディオデータをクリア
	m_audioMap.clear();
	m_soundPaths.clear();

	m_isBGMPlaying = false;
	m_isBGMPaused = false;

	
}

void SoundManager::setupSoundPaths()
{
	// サウンドファイルパスの設定
	m_soundPaths[SoundType::BGM_TITLE] = U"Sounds/title_bgm.ogg";
	m_soundPaths[SoundType::BGM_GAME] = U"Sounds/game_bgm.ogg";
	m_soundPaths[SoundType::BGM_CLEAR] = U"Sounds/clear_bgm.ogg";
	m_soundPaths[SoundType::SFX_SELECT] = U"Sounds/sfx_select.ogg";
	m_soundPaths[SoundType::SFX_BUMP] = U"Sounds/sfx_bump.ogg";
	m_soundPaths[SoundType::SFX_HURT] = U"Sounds/sfx_hurt.ogg";
	m_soundPaths[SoundType::SFX_JUMP] = U"Sounds/sfx_jump.ogg";
	m_soundPaths[SoundType::SFX_COIN] = U"Sounds/sfx_coin.ogg";
	m_soundPaths[SoundType::SFX_DISAPPEAR] = U"Sounds/sfx_disappear.ogg";
	m_soundPaths[SoundType::SFX_BREAK_BLOCK] = U"Sounds/sfx_break.ogg";
	m_soundPaths[SoundType::SFX_HIT] = U"Sounds/sfx_hit.ogg";
}

void SoundManager::loadSound(SoundType type, const String& filePath)
{
	Audio audio(filePath);

	if (audio)
	{
		m_audioMap[type] = audio;
	}
	
}

void SoundManager::playBGM(SoundType bgm, bool loop)
{
	// 既に同じBGMが再生中の場合は何もしない
	if (m_isBGMPlaying && m_currentBGMType == bgm)
	{
		return;
	}

	// 現在のBGMを停止
	if (m_isBGMPlaying)
	{
		stopBGM();
	}

	// 新しいBGMを再生
	if (m_audioMap.contains(bgm))
	{
		Audio& audio = m_audioMap[bgm];
		audio.setVolume(calculateVolume(m_bgmVolume));

		if (loop)
		{
			audio.setLoop(true);
		}

		audio.play();
		m_currentBGMType = bgm;
		m_isBGMPlaying = true;
		m_isBGMPaused = false;
	}
}

void SoundManager::stopBGM()
{
	if (m_isBGMPlaying && m_audioMap.contains(m_currentBGMType))
	{
		m_audioMap[m_currentBGMType].stop();
		m_isBGMPlaying = false;
		m_isBGMPaused = false;
	}
}

void SoundManager::pauseBGM()
{
	if (m_isBGMPlaying && m_audioMap.contains(m_currentBGMType))
	{
		m_audioMap[m_currentBGMType].pause();
		m_isBGMPaused = true;
	}
}

void SoundManager::resumeBGM()
{
	if (m_isBGMPaused && m_audioMap.contains(m_currentBGMType))
	{
		m_audioMap[m_currentBGMType].play();
		m_isBGMPaused = false;
	}
}

bool SoundManager::isBGMPlaying() const
{
	if (!m_isBGMPlaying || !m_audioMap.contains(m_currentBGMType))
	{
		return false;
	}

	return m_audioMap.at(m_currentBGMType).isPlaying();
}

bool SoundManager::isBGMPlaying(SoundType bgm) const
{
	if (!m_isBGMPlaying || m_currentBGMType != bgm)
	{
		return false;
	}

	if (!m_audioMap.contains(bgm))
	{
		return false;
	}

	return m_audioMap.at(bgm).isPlaying();
}

void SoundManager::playSE(SoundType se)
{
	if (m_audioMap.contains(se))
	{
		Audio& audio = m_audioMap[se];

		// SEが既に再生中の場合は停止してから再生
		if (audio.isPlaying())
		{
			audio.stop();
		}

		audio.setVolume(calculateVolume(m_seVolume));
		audio.setLoop(false);
		audio.play();
	}
}

void SoundManager::stopSE(SoundType se)
{
	if (m_audioMap.contains(se))
	{
		m_audioMap[se].stop();
	}
}

void SoundManager::stopAllSE()
{
	for (auto& [type, audio] : m_audioMap)
	{
		// BGM以外を停止
		if (type != m_currentBGMType)
		{
			audio.stop();
		}
	}
}

void SoundManager::setBGMVolume(double volume)
{
	m_bgmVolume = Math::Clamp(volume, 0.0, 1.0);

	// 現在再生中のBGMにボリュームを適用
	if (m_isBGMPlaying && m_audioMap.contains(m_currentBGMType))
	{
		m_audioMap[m_currentBGMType].setVolume(calculateVolume(m_bgmVolume));
	}
}

void SoundManager::setSEVolume(double volume)
{
	m_seVolume = Math::Clamp(volume, 0.0, 1.0);
	updateVolumes();
}

void SoundManager::setMasterVolume(double volume)
{
	m_masterVolume = Math::Clamp(volume, 0.0, 1.0);
	updateVolumes();
}

void SoundManager::updateVolumes()
{
	// 現在再生中のBGMにボリュームを適用
	if (m_isBGMPlaying && m_audioMap.contains(m_currentBGMType))
	{
		m_audioMap[m_currentBGMType].setVolume(calculateVolume(m_bgmVolume));
	}

	// 全SEにボリュームを適用（BGM以外）
	for (auto& [type, audio] : m_audioMap)
	{
		if (type != m_currentBGMType)
		{
			audio.setVolume(calculateVolume(m_seVolume));
		}
	}
}

double SoundManager::calculateVolume(double baseVolume) const
{
	// マスターボリュームと個別ボリュームを組み合わせて計算
	return baseVolume * m_masterVolume;
}
