#include <pp_protocol/p_relay_register.hpp>

int main(int, char **) {

    auto R        = xPP_RelayDispatcherSlaveRegister();
    R.TimestampMS = xel::GetTimestampMS();

    ubyte Buffer[xel::MaxPacketSize];
    auto  RSize = R.Serialize(Buffer, sizeof(Buffer));
    cout << HexShow(Buffer, RSize) << endl;

    auto RR = xPP_RelayDispatcherSlaveRegister();
    if (!RR.Deserialize(Buffer, RSize)) {
        cerr << "failed to deserialize data" << endl;
        return -1;
    }
    cout << RR.TimestampMS;
    return 0;
}
