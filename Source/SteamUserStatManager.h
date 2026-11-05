#pragma once

#include "CoreMinimal.h"

#if STEAMWORKS_ENABLE
#include "steam/steam_api.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamAchievement, Log, All);
#endif

/** Game-side progress of one mission, mapped 1:1 to a Steam stat + achievement. */
struct FMissionProgress
{
	FString StatName;            // Steam stat API name (e.g. "1001_KillMonster")
	FString AchievementApiName;  // Steam achievement API name
	int32   CurrentCount  = 0;
	int32   RequiredCount = 0;

	bool IsComplete() const { return RequiredCount > 0 && CurrentCount >= RequiredCount; }
};

enum class ESteamAchievementState : uint8
{
	Unknown,   // stats not received yet, or API name not registered on Steamworks
	Locked,
	Unlocked,
};

/**
 * Bridges the existing in-game mission system to Steam stats/achievements.
 * - Progress never goes backwards: Steam stat = max(local, Steam).
 * - Nothing is written before UserStatsReceived_t arrives (avoids overwriting server data).
 * - Multiple changes are batched; call Flush() once per frame/batch to StoreStats().
 */
class FSteamUserStatManager
{
public:
	static void Startup();   // call after SteamAPI_Init()
	static void Shutdown();
	static FSteamUserStatManager* Get() { return Instance.Get(); }

#if STEAMWORKS_ENABLE
	void SyncMission(const FMissionProgress& Progress);
	void Flush();
	void ResetAll(bool bIncludeAchievements);

	bool IsStatsReady() const { return bStatsReceived; }
	ESteamAchievementState GetAchievementState(const FString& ApiName) const;

private:
	FSteamUserStatManager();

	void SetStatIfHigher(const FMissionProgress& Progress);
	void UnlockIfComplete(const FMissionProgress& Progress);

	STEAM_CALLBACK(FSteamUserStatManager, OnUserStatsReceived, UserStatsReceived_t);
	STEAM_CALLBACK(FSteamUserStatManager, OnUserStatsStored, UserStatsStored_t);
	STEAM_CALLBACK(FSteamUserStatManager, OnUserAchievementStored, UserAchievementStored_t);

	bool bStatsReceived = false;
	bool bDirty = false;
#endif

private:
	static TUniquePtr<FSteamUserStatManager> Instance;
	friend struct TDefaultDelete<FSteamUserStatManager>;
};
