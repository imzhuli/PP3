#include "./block_account.hpp"

void xPPB_BlockAccount::SerializeMembers() {
    W(StartTimestampMS);
    W(AuditId);
    W(BlockType);  // 1. connection count, 2 bandwith
    W(BlockThresholdValue);
    W(BlockPeriodMS);
    W(Action);
}

void xPPB_BlockAccount::DeserializeMembers() {
    R(StartTimestampMS);
    R(AuditId);
    R(BlockType);  // 1. connection count, 2 bandwith
    R(BlockThresholdValue);
    R(BlockPeriodMS);
    R(Action);
}
