#include "./p_target_collect.hpp"

void xPP_TargetCollect::SerializeMembers() {
    Hash = 0;

    auto Writer    = GetWriter();
    auto HashStart = Writer->GetCurrentPosition();
    W(GlobalAuthId);    //
    W(TargetAddress);   //
    W(TargetHostView);  //
    if (Writer->HasError()) {
        return;
    }
    auto HashEnd = Writer->GetCurrentPosition();
    this->Hash   = (uint32_t)std::hash<std::string_view>{}({ (const char *)HashStart, (size_t)(HashEnd - HashStart) });

    W(StartTimestampMS);  //
    W(PeriodMS);          //
    W(Count);             //
}

void xPP_TargetCollect::DeserializeMembers() {
    R(GlobalAuthId);      //
    R(TargetAddress);     //
    R(TargetHostView);    //
    R(StartTimestampMS);  //
    R(PeriodMS);          //
    R(Count);             //

    Hash = 0;
}