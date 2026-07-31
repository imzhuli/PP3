#pragma once
#include "./_.hpp"

// c++ headers
#include <filesystem>
#include <xel/core/core_os.hpp>

struct xRuntimeEnv {
    std::filesystem::path HomeDir;
    std::filesystem::path BinDir;
    std::filesystem::path ConfigDir;
    std::filesystem::path DataDir;
    std::filesystem::path CacheDir;  //

    std::string           ProgramName;
    std::filesystem::path DefaultConfigFilePath;
    std::filesystem::path DefaultAuditLoggerFilePath;
    std::filesystem::path DefaultLoggerFilePath;
    std::filesystem::path DefaultLocalServerIdFilePath;
    std::filesystem::path DefaultLockFilePath;

    std::filesystem::path GetBinaryPath(const std::filesystem::path & Filename) const;
    std::filesystem::path GetConfigPath(const std::filesystem::path & Filename) const;
    std::filesystem::path GetDataPath(const std::filesystem::path & Filename) const;
    std::filesystem::path GetCachePath(const std::filesystem::path & Filename) const;

    /// @brief
    /// @return
    xConfigLoader  LoadConfig() const { return xConfigLoader(DefaultConfigFilePath); }
    xel::xFileLock Lock() const { return xel::xFileLock(DefaultLockFilePath); }

    static xRuntimeEnv FromCommandLine(int CmdArgc, char ** CmdArgv);
};
std::string ToString(const xRuntimeEnv & Env);
