#include "./p_block_account.hpp"

void xPP_BlockAccount::SerializeMembers() {
    W(StartTimestampMS);
    W(GlobalAuthId);
    W(BlockReason);
    W(BlockThreshold);
    W(BlockTriggerValue);
    W(BlockPeriodMS);
}

void xPP_BlockAccount::DeserializeMembers() {
    R(StartTimestampMS);
    R(GlobalAuthId);
    R(BlockReason);
    W(BlockThreshold);
    R(BlockTriggerValue);
    R(BlockPeriodMS);
}
