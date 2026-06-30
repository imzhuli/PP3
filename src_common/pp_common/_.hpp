#pragma once
#include <xel/config/config.hpp>
#include <xel/core/core_min.hpp>
#include <xel/core/core_os.hpp>
#include <xel/core/core_stream.hpp>
#include <xel/core/core_time.hpp>
#include <xel/core/core_value_util.hpp>
#include <xel/core/executable.hpp>
#include <xel/core/functional.hpp>
#include <xel/core/indexed_storage.hpp>
#include <xel/core/list.hpp>
#include <xel/core/logger.hpp>
#include <xel/core/memory.hpp>
#include <xel/core/memory_pool.hpp>
#include <xel/core/optional.hpp>
#include <xel/core/string.hpp>
#include <xel/crypto_wrapper/base64.hpp>
#include <xel/crypto_wrapper/hmac.hpp>
#include <xel/crypto_wrapper/md5.hpp>
#include <xel/crypto_wrapper/sha.hpp>
#include <xel/network/net_address.hpp>
#include <xel/network/tcp_connection.hpp>
#include <xel/network/tcp_server.hpp>
#include <xel/network/udp_channel.hpp>
#include <xel/object/object.hpp>
#include <xel/server_arch/message.hpp>
#include <xel/server_arch/tcp_client.hpp>
#include <xel/server_arch/tcp_client_pool.hpp>
#include <xel/server_arch/tcp_client_wrapper.hpp>
#include <xel/server_arch/tcp_service.hpp>
#include <xel/server_arch/udp_service.hpp>
#include <xel/service/base/def.hpp>

//
#include <cinttypes>
#include <filesystem>
namespace stdfs = std::filesystem;

//
using namespace xel::common;
using namespace std::chrono_literals;

// consts
using xel::InvalidDataSize;
using xel::MaxPacketPayloadSize;
using xel::MaxPacketSize;
using xel::PacketHeaderSize;

// type-defs
using xel::eLogLevel;
using xel::xAbstract;
using xel::xAutoHolder;
using xel::xBaseLogger;
using xel::xBinaryMessage;
using xel::xCommandLine;
using xel::xConfigLoader;
using xel::xHmacSha256Result;
using xel::xIndexedStorage;
using xel::xIndexId;
using xel::xIndexIdPool;
using xel::xIoContext;
using xel::xList;
using xel::xListNode;
using xel::xLogger;
using xel::xMd5Result;
using xel::xMemoryPool;
using xel::xMemoryPoolOptions;
using xel::xNetAddress;
using xel::xNoReentry;
using xel::xObjectIdManager;
using xel::xObjectIdManagerMini;
using xel::xOptional;
using xel::xPacket;
using xel::xPacketCommandId;
using xel::xPacketHeader;
using xel::xPacketRequestId;
using xel::xResourceGuard;
using xel::xScopeCleaner;
using xel::xScopeGuard;
using xel::xSocket;
using xel::xSpinlock;
using xel::xSpinlockGuard;
using xel::xStreamReader;
using xel::xStreamWriter;
using xel::xTcpClient;
using xel::xTcpClientPool;
using xel::xTcpClientPoolConnectionHandle;
using xel::xTcpClientWrapper;
using xel::xTcpConnection;
using xel::xTcpServer;
using xel::xTcpService;
using xel::xTcpServiceClientConnectionHandle;
using xel::xTicker;
using xel::xTimer;
using xel::xUdpChannel;
using xel::xUdpService;
using xel::xUdpServiceChannelHandle;

using xel::service::xServerGroup;
using xel::service::xServerId;

// functions
using xel::Base64Decode;
using xel::Base64Encode;
using xel::BuildPacket;
using xel::Daemonize;
using xel::Delegate;
using xel::FileToLines;
using xel::FileToStr;
using xel::GetTimestampMS;
using xel::GetUnixTimestamp;
using xel::HexShow;
using xel::HexToStr;
using xel::JoinStr;
using xel::Md5;
using xel::Noop;
using xel::Pass;
using xel::Pure;
using xel::RuntimeAssert;
using xel::Split;
using xel::Steal;
using xel::StrToHex;
using xel::StrToHexLower;
using xel::Todo;
using xel::Trim;
using xel::Unreachable;
using xel::WriteMessage;
using xel::ZeroFill;

// std-lib:
#include <functional>
#include <iostream>
using std::cerr;
using std::cout;
using std::endl;
using std::flush;
using std::function;

// min_defs:
using xVersion = uint32_t;

constexpr const xServerGroup ST_SERVER_LIST            = 0x00;
constexpr const xServerGroup ST_TARGET_COLLECTOR       = 0x01;
constexpr const xServerGroup ST_AUDIT_COLLECTOR        = 0x02;
constexpr const xServerGroup ST_RELAY_INFO_DISPATCHER  = 0x03;
constexpr const xServerGroup ST_DEVICE_INFO_DISPATCHER = 0x04;

struct xServerInfo {
    xServerId   ServerId = {};
    xNetAddress Address  = {};

    std::strong_ordering operator<=>(const xServerInfo &) const = default;
    static bool          LessById(const xServerInfo & LHS, const xServerInfo & RHS) { return LHS.ServerId < RHS.ServerId; }
};
constexpr const size_t MAX_SMALL_SERVER_LIST_SIZE = 128;

namespace __pp_common_detail__ {

    template <typename T>
    inline std::enable_if_t<std::is_function_v<T>> __TickOne__(uint64_t NowMS, T * F) {
        (*F)(NowMS);
    }

    template <typename T>
    inline std::enable_if_t<std::is_function_v<T>> __TickOne__(uint64_t NowMS, T & F) {
        F(NowMS);
    }

    template <typename T>
    inline std::enable_if_t<std::is_object_v<T>> __TickOne__(uint64_t NowMS, T & FO) {
        FO.Tick(NowMS);
    }

    inline void __TickAll__(uint64_t) {  // iteration finishes here
    }
    template <typename T, typename... TOthers>
    inline void __TickAll__(uint64_t NowMS, T && First, TOthers &&... Others) {
        __TickOne__(NowMS, std::forward<T>(First));
        __TickAll__(NowMS, std::forward<TOthers>(Others)...);
    }

}  // namespace __pp_common_detail__

template <typename... T>
inline void TickAll(uint64_t NowMS, T &&... All) {
    __pp_common_detail__::__TickAll__(NowMS, std::forward<T>(All)...);
}

class xPPNone {};
class xPPObjectBase;
class xPPObjectManagerBase;
template <typename T>
class xPPObjectHandle;

struct xPPObjectBase {
protected:
    friend class xPPObjectManagerBase;
    template <typename T>
    friend class xPPObjectHandle;

    uint64_t               Id      = 0;
    xPPObjectManagerBase * Manager = nullptr;
};

struct xPPObjectManagerBase {
    virtual xPPObjectBase * GetObjectById(uint64_t Id) = 0;
};

template <typename T>
class xPPObjectHandle final : std::enable_if_t<std::is_convertible_v<xPPObjectBase *, T *>, xPPNone> {
public:
    xPPObjectHandle(T * O)
        : Object(O), ObjectId(O->Id), Manager(O->Manager) {}

    inline uint64_t GetObjectId() const { return ObjectId; }
    inline bool     IsValid() const { return Manager->GetObjectById(ObjectId) == Object; }

    T * operator->() const { return Object; }

private:
    T *                    Object   = nullptr;
    uint64_t               ObjectId = 0;
    xPPObjectManagerBase * Manager  = nullptr;
};

// clang-format off

static inline uint32_t High32(uint64_t U) { return (uint32_t)(U >> 32); }
static inline uint32_t Low32(uint64_t U)  { return (uint32_t)(U); }
static inline uint64_t Make64(uint32_t H32, uint32_t L32) { return (static_cast<uint64_t>(H32) << 32) + L32; }

static inline uint16_t High16(uint64_t U) { return (uint16_t)(U >> 48); }
static inline uint64_t Low48(uint64_t U)  { return U & 0x0000'FFFF'FFFF'FFFFu; }
static inline uint64_t Make64_H16L48(uint16_t H16, uint64_t L48) { return (static_cast<uint64_t>(H16) << 48) + L48; }

// clang-format on

// clang-format off
// use Shutdown in callback setup, to avoid function overriding issue
static inline void Shutdown() { QuickExit(); }

template<typename T>
std::unique_ptr<T> P2U(T * && Ptr) { return std::unique_ptr<T>(std::move(Ptr)); }

extern uint32_t HashString(const char * S);
extern uint32_t HashString(const char * S, size_t Len);
extern uint32_t HashString(const std::string_view & S);


extern std::string AppSign(uint64_t Timestamp, const std::string & SecretKey, const void * DataPtr, size_t Size);
static inline std::string AppSign(uint64_t Timestamp, const std::string & SecretKey, const std::string_view& V) { return AppSign(Timestamp, SecretKey, V.data(), V.size()); }

extern bool ValidateAppSign(const std::string & Sign, const std::string & SecretKey, const void * DataPtr, size_t Size);
static inline bool ValidateAppSign(const std::string & Sign, const std::string & SecretKey, const std::string_view& V) { return ValidateAppSign(Sign, SecretKey, V.data(), V.size()); }

extern uint32_t ExtractIndexFromServerId(uint64_t ServerId);

// clang-format on
extern std::string    ToLower(const std::string & Src);
extern std::ostream & operator<<(std::ostream & OS, const xNetAddress & Address);

#define X_THREAD(...)                                                              \
    auto X_CONCAT_FORCE_EXPAND(__X_Thread__, __LINE__) = std::thread(__VA_ARGS__); \
    X_SCOPE_GUARD([&] { X_CONCAT_FORCE_EXPAND(__X_Thread__, __LINE__).join(); })

#ifndef NDEBUG
#define X_MAYBE_UNUSED [[maybe_unused]]
#else
#define X_MAYBE_UNUSED
#endif
