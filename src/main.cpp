#include <unistd.h>
#include "client.hpp"

void callback(const grpc::Status& s, const GetSquareResponse* r) {
};

int main(void) {
    std::shared_ptr<Math::Stub> stub = Math::NewStub(grpc::CreateChannel(
            "localhost:50051", grpc::InsecureChannelCredentials()));
    AsyncClient client(AsyncClient(stub, callback));

    GetSquareRequest request = GetSquareRequest();

    int result = 0;
    for (int i = 0; ; i++) {
        request.set_input(i);
        client.WriteAsync(request);
        while ((result = client.GetResult()) >= 0)
            std::cout << result << std::endl;
        usleep(1000);
    }

    return 0;
}
