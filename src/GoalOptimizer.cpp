#include "GoalOptimizer.h"

#include <atomic>
#include <cstdint>
#include <filesystem>

#include <ll/api/chrono/GameChrono.h>
#include <ll/api/coro/CoroTask.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/mod/RegisterHelper.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include <mc/entity/components/ActorOwnerComponent.h>
#include <mc/entity/systems/GoalSelectorSystem.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#pragma warning(push)
#pragma warning(disable : 4996)

namespace goal_optimizer {

static Config                          config;
static std::shared_ptr<ll::io::Logger> log;
static bool                            hookInstalled    = false;
static std::atomic<bool>               debugTaskRunning = false;

static uint64_t lastTickId   = 0;
static int      currentPhase = 0;

static size_t totalProcessed = 0;
static size_t totalSkipped   = 0;

static ll::io::Logger& getLogger() {
    if (!log) {
        log = ll::io::LoggerRegistry::getInstance().getOrCreate("GoalOptimizer");
    }
    return *log;
}

Config& getConfig() { return config; }

bool loadConfig() {
    auto path   = GoalOptimizer::getInstance().getSelf().getConfigDir() / "config.json";
    bool loaded = ll::config::loadConfig(config, path);
    if (config.phaseCount < 1) config.phaseCount = 1;
    return loaded;
}

bool saveConfig() {
    auto path = GoalOptimizer::getInstance().getSelf().getConfigDir() / "config.json";
    return ll::config::saveConfig(config, path);
}

static void resetStats() {
    totalProcessed = 0;
    totalSkipped   = 0;
}

static void startDebugTask() {
    if (debugTaskRunning.exchange(true)) return;

    ll::coro::keepThis([]() -> ll::coro::CoroTask<> {
        while (debugTaskRunning.load()) {
            co_await std::chrono::seconds(5);
            ll::thread::ServerThreadExecutor::getDefault().execute([] {
                if (!config.debug) return;
                size_t total    = totalProcessed + totalSkipped;
                double skipRate = total > 0 ? (100.0 * totalSkipped / total) : 0.0;
                getLogger().info(
                    "Goal stats (5s): processed={}, skipped={}, skipRate={:.1f}%, phaseCount={}",
                    totalProcessed,
                    totalSkipped,
                    skipRate,
                    config.phaseCount
                );
                resetStats();
            });
        }
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

static void stopDebugTask() { debugTaskRunning.store(false); }

} // namespace goal_optimizer

// ====================== Hook ======================

LL_STATIC_HOOK(
    GoalSelectorTickHook,
    ll::memory::HookPriority::Normal,
    GoalSelectorSystem::_tickGoalSelectorComponent,
    void,
    ActorOwnerComponent& actorOwnerComponent
) {
    using namespace goal_optimizer;

    if (!config.enabled) {
        origin(actorOwnerComponent);
        return;
    }

    Actor& actor = *actorOwnerComponent.mActor;

    if (actor.isPlayer()) {
        origin(actorOwnerComponent);
        ++totalProcessed;
        return;
    }

    auto&    level       = actor.getLevel();
    uint64_t currentTick = level.getCurrentTick().tickID;

    if (currentTick != lastTickId) {
        lastTickId   = currentTick;
        currentPhase = static_cast<int>(currentTick % config.phaseCount);
    }

    // rawID is int64_t; reinterpret as uint64_t so modulo works correctly for negative IDs
    uint64_t uId         = static_cast<uint64_t>(actor.getOrCreateUniqueID().rawID);
    int      entityPhase = static_cast<int>(uId % config.phaseCount);

    if (entityPhase != currentPhase) {
        ++totalSkipped;
        return;
    }

    origin(actorOwnerComponent);
    ++totalProcessed;
}

#pragma warning(pop)

// ====================== 生命周期 ======================

namespace goal_optimizer {

GoalOptimizer& GoalOptimizer::getInstance() {
    static GoalOptimizer instance;
    return instance;
}

bool GoalOptimizer::load() {
    std::filesystem::create_directories(getSelf().getConfigDir());
    if (!loadConfig()) {
        getLogger().warn("Failed to load config, using defaults and saving");
        saveConfig();
    }
    getLogger().info(
        "GoalOptimizer loaded. enabled: {}, debug: {}, phaseCount: {}",
        config.enabled,
        config.debug,
        config.phaseCount
    );
    return true;
}

bool GoalOptimizer::enable() {
    if (!hookInstalled) {
        GoalSelectorTickHook::hook();
        hookInstalled = true;
    }
    if (config.debug) startDebugTask();
    getLogger().info("GoalOptimizer enabled");
    return true;
}

bool GoalOptimizer::disable() {
    stopDebugTask();
    if (hookInstalled) {
        GoalSelectorTickHook::unhook();
        hookInstalled = false;
        lastTickId    = 0;
        currentPhase  = 0;
        resetStats();
    }
    getLogger().info("GoalOptimizer disabled");
    return true;
}

} // namespace goal_optimizer

LL_REGISTER_MOD(goal_optimizer::GoalOptimizer, goal_optimizer::GoalOptimizer::getInstance());
