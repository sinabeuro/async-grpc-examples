#include <unistd.h>
#include "client.hpp"

void callback(const grpc::Status& s, const GetSquareResponse* r) {
};

int main(void) {
    std::shared_ptr<Math::Stub> stub = Math::NewStub(grpc::CreateChannel(
            "localhost:50051", grpc::InsecureChannelCredentials()));
    AsyncClient client(AsyncClient(stub, callback));

    GetSquareRequest request = GetSquareRequest();

    for (int i = 0; ; i++) {
        request.set_input(i);
        client.WriteAsync(request);
        usleep(1000);
    }

    return 0;
}
