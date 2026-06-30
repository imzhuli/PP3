#include "./service_runtime.hpp"

#include <fstream>
#include <mutex>

/// @brief Common code in ////////

static xIoContext      ServiceIoContextInstance;
static xel::xStdLogger StdLogger;
static xel::xNonLogger NonLogger;

xRuntimeEnv  ServiceEnvironment         = {};
xLogger *    Logger                     = &NonLogger;
xLogger *    AuditLogger                = &NonLogger;
xLogger *    ConsoleLogger              = &StdLogger;
xIoContext * ServiceIoContext           = nullptr;
uint64_t     ServiceIoLoopOnceTimeoutMS = 10;
xTicker      ServiceTicker              = {};
xRunState    ServiceRunState            = {};

static void InitLogger() {
    Logger = new xBaseLogger();
    X_RUNTIME_ASSERT(static_cast<xBaseLogger *>(Logger)->Init(std::string(ServiceEnvironment.DefaultLoggerFilePath).c_str()));
    Logger->SetLogLevel(eLogLevel::Debug);

    AuditLogger = new xBaseLogger();
    X_RUNTIME_ASSERT(static_cast<xBaseLogger *>(AuditLogger)->Init(std::string(ServiceEnvironment.DefaultAuditLoggerFilePath).c_str()));
}

static void CleanLogger() {
    X_RUNTIME_ASSERT(AuditLogger);
    static_cast<xBaseLogger *>(AuditLogger)->Clean();
    delete Steal(AuditLogger, &NonLogger);

    X_RUNTIME_ASSERT(Logger);
    static_cast<xBaseLogger *>(Logger)->Clean();
    delete Steal(Logger, &NonLogger);
}

static auto Instance = (xServiceEnvironmentGuard *){};
static auto EnvMutex = std::mutex();

void AssertServiceEnviromentReady() {
    X_RUNTIME_ASSERT(Instance);
}

xServiceEnvironmentGuard::xServiceEnvironmentGuard(int argc, char ** argv)
    : EnableLogger(true) {
    auto G = std::lock_guard(EnvMutex);
    X_RUNTIME_ASSERT(!Instance);

    ServiceEnvironment = xRuntimeEnv::FromCommandLine(argc, argv);
    if (EnableLogger) {
        InitLogger();
    }
    ServiceIoContext = &ServiceIoContextInstance;
    X_RUNTIME_ASSERT(ServiceIoContext->Init());
    X_RUNTIME_ASSERT(ServiceRunState.Start());
    Instance = this;
}

xServiceEnvironmentGuard::xServiceEnvironmentGuard(int argc, char ** argv, const decltype(ServiceNoLogger) &)
    : EnableLogger(false) {
    auto G = std::lock_guard(EnvMutex);
    X_RUNTIME_ASSERT(!Instance);

    ServiceEnvironment = xRuntimeEnv::FromCommandLine(argc, argv);
    ServiceIoContext   = &ServiceIoContextInstance;
    X_RUNTIME_ASSERT(ServiceIoContext->Init());
    X_RUNTIME_ASSERT(ServiceRunState.Start());
    Instance = this;
}

xServiceEnvironmentGuard::xServiceEnvironmentGuard(int argc, char ** argv, const decltype(ServiceConsoleLogger) &)
    : EnableLogger(false) {
    auto G = std::lock_guard(EnvMutex);
    X_RUNTIME_ASSERT(!Instance);

    ServiceEnvironment = xRuntimeEnv::FromCommandLine(argc, argv);
    Reset(AuditLogger, &StdLogger);
    Reset(Logger, &StdLogger);

    ServiceIoContext = &ServiceIoContextInstance;
    X_RUNTIME_ASSERT(ServiceIoContext->Init());
    X_RUNTIME_ASSERT(ServiceRunState.Start());
    Instance = this;
}

xServiceEnvironmentGuard::~xServiceEnvironmentGuard() {
    auto G = std::lock_guard(EnvMutex);
    X_RUNTIME_ASSERT(this == Instance);

    ServiceRunState.Finish();
    Steal(ServiceIoContext)->Clean();
    if (EnableLogger) {
        CleanLogger();
    } else {  // maybe console output
        Reset(AuditLogger, &NonLogger);
        Reset(Logger, &NonLogger);
    }
    Reset(ServiceEnvironment);
    Reset(Instance);
}

/////////////////////////

bool xServiceRequestContextPool::Init(size_t PoolSize) {
    return Pool.Init(PoolSize);
}

void xServiceRequestContextPool::Clean() {
    Pool.Clean();
    assert(TimeoutList.IsEmpty());
}

auto xServiceRequestContextPool::Acquire(xVariable RequestContext, xVariable RequestContextEx) -> const xServiceRequestContext * {
    auto Id = Pool.Acquire();
    if (!Id) {
        return nullptr;
    }
    auto & N             = Pool[Id];
    N.RequestId          = Id;
    N.RequestTimestampMS = ServiceTicker();
    TimeoutList.AddTail(N);

    N.RequestContext   = RequestContext;
    N.RequestContextEx = RequestContextEx;

    return &N;
}

auto xServiceRequestContextPool::CheckAndGet(uint64_t RequestId) -> const xServiceRequestContext * {
    return Pool.CheckAndGet(RequestId);
}

void xServiceRequestContextPool::Release(const xServiceRequestContext * RCP) {
    assert(Pool.CheckAndGet(RCP->RequestId) == RCP);
    Pool.Release(RCP->RequestId);
}
