#include "./target_collect.hpp"

void xPPB_TargetCollect::SerializeMembers() {
    W(TimeMs);
    W(AuditId);
    W(Domain);
    W(IpAddress);
    W(Port);
    W(TimePeriodMs);
    W(TotalRequestCount);
}

void xPPB_TargetCollect::DeserializeMembers() {
    R(TimeMs);
    R(AuditId);
    R(Domain);
    R(IpAddress);
    R(Port);
    R(TimePeriodMs);
    R(TotalRequestCount);
}
