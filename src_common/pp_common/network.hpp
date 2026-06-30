#pragma once
#include "./_.hpp"

extern bool        IsLocalNetAddress(const xNetAddress & Addr);
extern std::string SignAndPackAddress(uint64_t Timestamp, const std::string & SecretKey, const xNetAddress & Address);
extern xNetAddress ExtractAddressFromPack(const std::string & SignedIp, const std::string & SecretKey);
