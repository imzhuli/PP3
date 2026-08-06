#pragma once
#include <pp_common/_.hpp>

static constexpr const size_t   MAX_RELAY_SERVER_LIST_SIZE             = 20'0000;
static constexpr const uint64_t RELAY_HEARTBEAT_INTERVAL_MS            = 5 * 60'000;
static constexpr const uint64_t RELAY_HEARTBEAT_TIMEOUT_MS             = RELAY_HEARTBEAT_INTERVAL_MS + 60'000;
//
static constexpr const size_t   MAX_TOTAL_DEVICE_CONNECTION_COUNT      = 30'0000;
static constexpr const size_t   MAX_TOTAL_DEVICE_UDPCHANNEL_COUNT      = 15'0000;
static constexpr const size_t   MAX_REMOTE_DEVICE_COUNT_PER_SERVER     = 25'0000;
static constexpr const size_t   MAX_LOCAL_DEVICE_COUNT_PER_SERVER      = 1'0000;
//
static constexpr const uint64_t CLIENT_CHALLENGE_RETRY_TIMEOUT_MS      = 10 * 60'000;
//
static constexpr const uint64_t CLIENT_CONNECTION_IDLE_TIMEOUT_MS      = 125'000;
static constexpr const uint64_t CLIENT_UDPCHANNEL_IDLE_TIMEOUT_MS      = 125'000;
static constexpr const uint64_t TARGET_CONNECTION_IDLE_TIMEOUT_MS      = 125'000;
static constexpr const uint64_t TARGET_UDPCHANNEL_IDLE_TIMEOUT_MS      = 125'000;
//
static constexpr const uint64_t TARGET_CONNECTION_ESTABLISH_TIMEOUT_MS = 5'000;
static constexpr const uint64_t TARGET_UDPCHANNEL_READY_TIMEOUT_MS     = 5'000;
//
static constexpr const uint64_t DEVICE_ENTRY_DEFAULT_INIT_DELAY_MS     = 6 * 60'000;
//
static constexpr const uint64_t PA_RL_READ_BUFFER_SIZE                 = 96'000;
static constexpr const uint64_t PA_RL_WRITE_BUFFER_SIZE                = 32'000;
