#pragma once
#include <pp_common/_.hpp>

static constexpr const size_t   MAX_TOTAL_DEVICE_CONNECTION_COUNT      = 20'0000;
static constexpr const size_t   MAX_TOTAL_DEVICE_UDPCHANNEL_COUNT      = 10'0000;
static constexpr const size_t   MAX_REMOTE_DEVICE_COUNT_PER_SERVER     = 25'0000;
static constexpr const size_t   MAX_LOCAL_DEVICE_COUNT_PER_SERVER      = 1'0000;
//
static constexpr const uint64_t CLIENT_CONNECTION_IDLE_TIMEOUT_MS      = 125'000;
static constexpr const uint64_t CLIENT_UDPCHANNEL_IDLE_TIMEOUT_MS      = 125'000;
static constexpr const uint64_t TARGET_CONNECTION_IDLE_TIMEOUT_MS      = 125'000;
static constexpr const uint64_t TARGET_UDPCHANNEL_IDLE_TIMEOUT_MS      = 125'000;
//
static constexpr const uint64_t TARGET_CONNECTION_ESTABLISH_TIMEOUT_MS = 5'000;
static constexpr const uint64_t TARGET_UDPCHANNEL_READY_TIMEOUT_MS     = 5'000;
