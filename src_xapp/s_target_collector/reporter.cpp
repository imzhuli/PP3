#include "./reporter.hpp"

#include <lib_util/rdkafka_wrapper.hpp>
#include <pp_common/service_runtime.hpp>
#include <pp_protocol/_backend/kfk/target_collect.hpp>
#include <pp_protocol/command.hpp>

static constexpr size_t MAX_AUDIT_REPORTER_COUNT = 20'0000;

struct xKfkContext {
    xRdKfkOneTopicProducer KR                  = {};
    std::string            SecurityProtocol    = {};
    std::string            SaslMechanism       = {};
    std::string            SaslUsername        = {};
    std::string            SaslPassword        = {};
    std::string            BootstrapServerList = {};
    std::string            Topic               = {};
};

std::string xTargetCollectReporter::xTargetCollectNode::ToString() const {
    auto OS = std::ostringstream();
    OS << "xTargetCollectNode:" << endl;
    OS << "\tGlobalAuthId=" << GlobalAuthId << endl;
    OS << "\tTargetAddress=" << TargetAddress.ToString() << endl;
    OS << "\tTargetHost=" << TargetHost << endl;
    OS << "\tCount=" << Count << endl;
    return OS.str();
}

bool xTargetCollectReporter::Init(const std::string & ConfigFilename) {

    if (!NodePool.Init({ .InitSize = MAX_AUDIT_REPORTER_COUNT, .MaxPoolSize = MAX_AUDIT_REPORTER_COUNT })) {
        return false;
    }

    KfkContext = new xKfkContext();
    auto CL    = xel::xConfigLoader(ConfigFilename);

    CL.Require(KfkContext->SecurityProtocol, "SecurityProtocol");
    CL.Require(KfkContext->SaslMechanism, "SaslMechanism");
    CL.Require(KfkContext->SaslUsername, "SaslUsername");
    CL.Require(KfkContext->SaslPassword, "SaslPassword");
    CL.Require(KfkContext->BootstrapServerList, "BootstrapServerList");
    CL.Require(KfkContext->Topic, "Topic");

    if (!KfkContext->KR.Init(
            KfkContext->Topic,
            {
                { "security.protocol", KfkContext->SecurityProtocol },
                { "sasl.mechanism", KfkContext->SaslMechanism },
                { "sasl.username", KfkContext->SaslUsername },
                { "sasl.password", KfkContext->SaslPassword },
                { "bootstrap.servers", KfkContext->BootstrapServerList },
            }
        )) {
        delete Steal(KfkContext);
        NodePool.Clean();
        return false;
    }

    RunState.Start();
    KfkThread = std::thread([this] {
        KfkThreadFunc();
    });
    return true;
}

void xTargetCollectReporter::Clean() {
    RunState.Stop();
    Steal(KfkThread).join();
    do {  // clear resources
        xTargetCollectList FreeCollectionList;
        FreeCollectionList.GrabListTail(PostCollectionList);
        FreeCollectionList.GrabListTail(PendingCollectionList);
        FreeCollectionList.GrabListTail(FinishedCollectionList);
        while (auto Node = FreeCollectionList.PopHead()) {
            NodePool.Destroy(Node);
        }
    } while (false);
    KfkContext->KR.Clean();
    delete Steal(KfkContext);
    NodePool.Clean();
}

void xTargetCollectReporter::Tick(uint64_t NowMS) {
    xTargetCollectList FreeCollectionList;
    do {
        X_VAR xSpinlockGuard(SwitchListLock);
        PendingCollectionList.GrabListTail(PostCollectionList);
        FreeCollectionList.GrabListTail(FinishedCollectionList);
    } while (false);
    while (auto Node = FreeCollectionList.PopHead()) {
        NodePool.Destroy(Node);
    }
}

void xTargetCollectReporter::PostTargetCollect(uint64_t GlobalAuthId, const xel::xNetAddress & TargetAddress, const std::string_view & TargetHost, size_t Count) {
    auto Node = NodePool.Create();
    if (!Node) {
        DEBUG_LOG("%s", "NodePool OOM");
        return;
    }
    Node->GlobalAuthId  = GlobalAuthId;
    Node->TargetAddress = TargetAddress;
    Node->TargetHost    = TargetHost;
    Node->Count         = Count;
    PostCollectionList.AddTail(*Node);

    PostCollectEvent.Notify();
}

void xTargetCollectReporter::KfkThreadFunc() {
    xTargetCollectList TempPostList;
    xTargetCollectList TempFinishedList;

    auto  NowMS = xel::GetTimestampMS();
    ubyte Buffer[MaxPacketSize];
    while (RunState) {
        assert(TempFinishedList.IsEmpty());
        while (auto PNode = TempPostList.PopHead()) {
            // do post:
            DEBUG_LOG("%s", PNode->ToString().c_str());

            auto R              = xPPB_TargetCollect();
            R.TimeMs            = NowMS;
            R.AuditId           = PNode->GlobalAuthId;
            R.Domain            = PNode->TargetHost;
            R.IpAddress         = PNode->TargetAddress;
            R.Port              = 0;
            R.TimePeriodMs      = 1;
            R.TotalRequestCount = PNode->Count;
            auto MSize          = WriteMessage(Buffer, Cmd_BackendTargetReport, R);

            auto MsgKey = std::to_string(PNode->GlobalAuthId);
            KfkContext->KR.Post(MsgKey, Buffer, MSize);

            TempFinishedList.AddTail(*PNode);
        }

        do {
            PostCollectEvent.WaitFor(50ms);
            X_VAR xSpinlockGuard(SwitchListLock);
            FinishedCollectionList.GrabListTail(TempFinishedList);
            TempPostList.GrabListTail(PendingCollectionList);
        } while (false);
    }
}

std::string xTargetCollectReporter::GetAuditOutput() const {
    if (!KfkContext) {
        return {};
    }
    return KfkContext->KR.GetAuditOutput();
}
