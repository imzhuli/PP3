#include "./runtime_env.hpp"

#include <filesystem>
#include <sstream>

std::string ToString(const xRuntimeEnv & Env) {
    auto OS = std::ostringstream();
    OS << "Home: " << Env.HomeDir << endl;
    OS << "Bin: " << Env.BinDir << endl;
    OS << "Conf: " << Env.ConfigDir << endl;
    OS << "Data: " << Env.DataDir << endl;
    OS << "Cache: " << Env.CacheDir << endl;

    OS << "DefaultConfigFilePath: " << Env.DefaultConfigFilePath << endl;
    return OS.str();
}

xRuntimeEnv xRuntimeEnv::FromCommandLine(int CmdArgc, char ** CmdArgv) {
    auto Env = xRuntimeEnv{};
    auto CL  = xCommandLine(
        CmdArgc, CmdArgv,
        {
            { 'c', "config", "config-file", true },
            { 'h', "home", "home_dir", true },
        }
    );
    if (!CmdArgc) {
        return Env;
    }
    auto ProgramPath = std::filesystem::path(CmdArgv[0]);
    if (!ProgramPath.empty()) {
        Env.ProgramName = ProgramPath.filename();
    }
    if (Env.ProgramName.empty()) {
        Env.ProgramName = "unnamed_application";
    }

    auto DefaultBinPath = std::filesystem::current_path();
    auto HomeOpt        = CL["home_dir"];
    if (!HomeOpt) {
        Env.HomeDir   = std::filesystem::weakly_canonical(DefaultBinPath);
        Env.BinDir    = Env.HomeDir;
        Env.ConfigDir = Env.HomeDir;
        Env.DataDir   = Env.HomeDir;
        Env.CacheDir  = Env.HomeDir;
    } else {
        Env.HomeDir   = std::filesystem::weakly_canonical(*HomeOpt);
        Env.BinDir    = Env.HomeDir / "bin";
        Env.ConfigDir = Env.HomeDir / "conf";
        Env.DataDir   = Env.HomeDir / "data";
        Env.CacheDir  = Env.HomeDir / "cache";
    }

    auto ConfigFilename    = std::string("");
    auto ConfigFilenameOpt = CL["config-file"];
    if (ConfigFilenameOpt) {
        Env.DefaultConfigFilePath = Env.GetConfigPath(*ConfigFilenameOpt);
    } else {
        Env.DefaultConfigFilePath = Env.GetConfigPath("default");
    }

    Env.DefaultAuditLoggerFilePath   = Env.CacheDir / (Env.ProgramName + ".adt");
    Env.DefaultLoggerFilePath        = Env.CacheDir / (Env.ProgramName + ".log");
    Env.DefaultLocalServerIdFilePath = Env.CacheDir / (Env.ProgramName + ".sid");

    return Env;
}

static inline bool IsRelativePath(const std::filesystem::path & Path) {
    auto ParentPath = Path.parent_path();
    return ParentPath.empty();
}

std::filesystem::path xRuntimeEnv::GetBinaryPath(const std::filesystem::path & Filename) const {
    RuntimeAssert(!Filename.empty(), "filename should not be empty");
    return IsRelativePath(Filename) ? (BinDir / Filename) : Filename;
}

std::filesystem::path xRuntimeEnv::GetConfigPath(const std::filesystem::path & Filename) const {
    RuntimeAssert(!Filename.empty(), "filename should not be empty");
    return IsRelativePath(Filename) ? (ConfigDir / Filename) : Filename;
}

std::filesystem::path xRuntimeEnv::GetDataPath(const std::filesystem::path & Filename) const {
    RuntimeAssert(!Filename.empty(), "filename should not be empty");
    return IsRelativePath(Filename) ? (DataDir / Filename) : Filename;
}

std::filesystem::path xRuntimeEnv::GetCachePath(const std::filesystem::path & Filename) const {
    RuntimeAssert(!Filename.empty(), "filename should not be empty");
    return IsRelativePath(Filename) ? (CacheDir / Filename) : Filename;
}

/////

xConfigLoader xRuntimeEnv::LoadConfig() const {
    return xConfigLoader(DefaultConfigFilePath);
}
