#include "./audit_abstract.hpp"

std::string xAuditUsage::ToString() const {
    auto OS = std::ostringstream();
    OS << "xAuditUsage" << endl;
    OS << "\tAuthId:" << AuthId << endl;
    OS << "\tStartTimestampMS:" << StartTimestampMS << endl;
    OS << "\tPeriodMS:" << PeriodMS << endl;
    OS << "\tTotalTcpConnections:" << TotalTcpConnections << endl;
    OS << "\tTotalTcpBytesFromClient:" << TotalTcpBytesFromClient << endl;
    OS << "\tTotalTcpBytesToClient:" << TotalTcpBytesToClient << endl;
    OS << "\tTotalUdpChannels:" << TotalUdpChannels << endl;
    OS << "\tTotalUdpBytesFromClient:" << TotalUdpBytesFromClient << endl;
    OS << "\tTotalUdpBytesToClient:" << TotalUdpBytesToClient << endl;
    return OS.str();
}

std::string xAuditBlockAccount::ToString() const {
    auto OS = std::ostringstream();
    OS << "xAuditUsage" << endl;
    OS << "\tAuthId:" << AuthId << endl;
    OS << "\tStartTimestampMS:" << StartTimestampMS << endl;
    OS << "\tPeriodMS:" << PeriodMS << endl;
    OS << "\tReason:" << (uint32_t)Reason << endl;
    OS << "\tThreshold:" << Threshold << endl;
    OS << "\tTriggerValue:" << TriggerValue << endl;
    return OS.str();
}