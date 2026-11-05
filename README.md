# Steam Achievement Bridge (Unreal Engine 5 / Steamworks)

A sample of the Steam stats/achievement integration layer I designed and implemented for a commercial UE5 open-world action RPG shipped on Steam / Steam Deck (2026).

The game already had its own mission/achievement system. This layer connects it to Steam stats and achievements without changing that system.

> This is a cleaned-up rewrite of my production code: project-specific types and names are removed and replaced with a small `FMissionProgress` struct. It is published with permission and does not include any other part of the game.

## Design

| Problem | Decision |
|---|---|
| Progress can be lost or go backwards when local save and Steam disagree (reinstall, cloud conflict) | Steam stat = `max(local, Steam)`; never lower a stat |
| Writing before `UserStatsReceived_t` arrives overwrites the player's server data | All writes are gated on `bStatsReceived` |
| Calling `StoreStats()` on every mission tick is rate-limited by Steam | Changes mark `bDirty`; `Flush()` stores once per batch, retries after a failed store |
| Steam must be optional (editor, non-Steam platforms) | Everything compiles out with `STEAMWORKS_ENABLE`; `UserStats()` returns null when not logged on |
| QA needs to reset/replay achievements | `ResetAll()` for cheat commands |

## Changes from the production version

Found while reviewing my own code for this sample:

- The singleton pointer was never assigned → replaced with an owned `TUniquePtr` and explicit `Startup()`/`Shutdown()`.
- The callback result check was inverted (logged errors on success).
- `TSharedFromThis` had a wrong template argument (copy-paste) → removed, not needed.
- `assert`-only null checks → real checks; `GetStat` failure is now handled; repeated `weak_ptr::lock()` removed.
- `StoreStats()` per change → batched with a dirty flag.

## Files

- `Source/SteamUserStatManager.h`
- `Source/SteamUserStatManager.cpp`
