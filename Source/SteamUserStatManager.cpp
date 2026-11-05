#include "SteamUserStatManager.h"

TUniquePtr<FSteamUserStatManager> FSteamUserStatManager::Instance;

#if STEAMWORKS_ENABLE
DEFINE_LOG_CATEGORY(LogSteamAchievement);

namespace
{
	ISteamUserStats* UserStats()
	{
		ISteamUser* User = SteamUser();
		return (User != nullptr && User->BLoggedOn()) ? SteamUserStats() : nullptr;
	}
}

FSteamUserStatManager::FSteamUserStatManager()
{
	// STEAM_CALLBACK members register here, so this must run after SteamAPI_Init().
	if (ISteamUserStats* Stats = UserStats())
	{
		Stats->RequestCurrentStats();
	}
}
#endif

void FSteamUserStatManager::Startup()
{
#if STEAMWORKS_ENABLE
	if (!Instance.IsValid())
	{
		Instance = TUniquePtr<FSteamUserStatManager>(new FSteamUserStatManager());
	}
#endif
}

void FSteamUserStatManager::Shutdown()
{
	Instance.Reset();
}

#if STEAMWORKS_ENABLE
void FSteamUserStatManager::SyncMission(const FMissionProgress& Progress)
{
	if (!bStatsReceived || UserStats() == nullptr)
	{
		return; // re-synced in bulk from OnUserStatsReceived by the mission system
	}
	SetStatIfHigher(Progress);
	UnlockIfComplete(Progress);
}

void FSteamUserStatManager::SetStatIfHigher(const FMissionProgress& Progress)
{
	ISteamUserStats* Stats = UserStats();
	const auto StatName = StringCast<ANSICHAR>(*Progress.StatName);

	int32 SteamValue = 0;
	if (!Stats->GetStat(StatName.Get(), &SteamValue))
	{
		UE_LOG(LogSteamAchievement, Warning, TEXT("Stat not registered: %s"), *Progress.StatName);
		return;
	}

	if (Progress.CurrentCount > SteamValue && Stats->SetStat(StatName.Get(), Progress.CurrentCount))
	{
		bDirty = true;
	}
}

void FSteamUserStatManager::UnlockIfComplete(const FMissionProgress& Progress)
{
	if (!Progress.IsComplete() || GetAchievementState(Progress.AchievementApiName) != ESteamAchievementState::Locked)
	{
		return;
	}
	if (UserStats()->SetAchievement(StringCast<ANSICHAR>(*Progress.AchievementApiName).Get()))
	{
		bDirty = true;
	}
}

ESteamAchievementState FSteamUserStatManager::GetAchievementState(const FString& ApiName) const
{
	ISteamUserStats* Stats = UserStats();
	bool bUnlocked = false;
	if (!bStatsReceived || Stats == nullptr || !Stats->GetAchievement(StringCast<ANSICHAR>(*ApiName).Get(), &bUnlocked))
	{
		return ESteamAchievementState::Unknown;
	}
	return bUnlocked ? ESteamAchievementState::Unlocked : ESteamAchievementState::Locked;
}

void FSteamUserStatManager::Flush()
{
	ISteamUserStats* Stats = UserStats();
	if (bDirty && Stats != nullptr && Stats->StoreStats())
	{
		bDirty = false;
	}
}

void FSteamUserStatManager::ResetAll(bool bIncludeAchievements)
{
	// Debug/cheat use only.
	if (ISteamUserStats* Stats = UserStats())
	{
		Stats->ResetAllStats(bIncludeAchievements);
		bDirty = true;
		Flush();
	}
}

void FSteamUserStatManager::OnUserStatsReceived(UserStatsReceived_t* Callback)
{
	if (Callback->m_nGameID != SteamUtils()->GetAppID())
	{
		return; // callbacks for other apps are broadcast too
	}
	bStatsReceived = (Callback->m_eResult == k_EResultOK);
	if (!bStatsReceived)
	{
		UE_LOG(LogSteamAchievement, Error, TEXT("UserStatsReceived failed: %d"), static_cast<int32>(Callback->m_eResult));
	}
}

void FSteamUserStatManager::OnUserStatsStored(UserStatsStored_t* Callback)
{
	if (Callback->m_eResult != k_EResultOK)
	{
		bDirty = true; // retry on next Flush()
		UE_LOG(LogSteamAchievement, Error, TEXT("UserStatsStored failed: %d"), static_cast<int32>(Callback->m_eResult));
	}
}

void FSteamUserStatManager::OnUserAchievementStored(UserAchievementStored_t* Callback)
{
	UE_LOG(LogSteamAchievement, Log, TEXT("Achievement stored: %hs"), Callback->m_rgchAchievementName);
}
#endif
