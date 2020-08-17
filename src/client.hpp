#include <thread>

#include <grpcpp/grpcpp.h>

#include "proto/math_service.pb.h"
#include "proto/math_service.grpc.pb.h"

using RequestType = GetSquareRequest;
using ResponseType = GetSquareResponse;
using CallbackType =
    std::function<void(const grpc::Status&, const ResponseType*)>;

class AsyncClient;
struct RequestContext;

class CompletionQueue {
 public:
  struct ClientEvent {
    enum class Event { FINISH = 0, READ = 1, WRITE = 2 };
    ClientEvent(Event event, AsyncClient* async_client, RequestContext *req_ctx)
        : event(event), async_client(async_client), req_ctx(req_ctx){}
    Event event;
    AsyncClient* async_client;
    RequestContext *req_ctx;
    bool ok = false;
  };

 public:
  CompletionQueue() = default;

  void Start();
  void Shutdown();
  static CompletionQueue *GetCompletionQueue();

  grpc::CompletionQueue* grpc_completion_queue() { return &grpc_completion_queue_; }

 private:
  void Initialize();
  void RunCompletionQueue();

  static CompletionQueue* completion_queue();
  grpc::CompletionQueue grpc_completion_queue_;
  std::unique_ptr<std::thread> thread_;
  bool initialized_ = false;
};

struct RequestContext {
  grpc::ClientContext client_context_;
  grpc::Status status_;
  ResponseType response_;
  std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>> response_reader_;
};

class AsyncClient
{
  friend class CompletionQueue;
 public:
  AsyncClient(std::shared_ptr<Math::Stub> stub, CallbackType callback)
      : stub_(stub),
        callback_(callback),
        completion_queue_(CompletionQueue::GetCompletionQueue()) {}

  ~AsyncClient() {};

  void WriteAsync(const RequestType& request) {
    auto req_ctx = new RequestContext;
    auto finish_event_ = new CompletionQueue::ClientEvent(
      CompletionQueue::ClientEvent::Event::FINISH, this, req_ctx);
    req_ctx->response_reader_ =
        std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>>(
            stub_->PrepareAsyncGetSquare(&req_ctx->client_context_, request,
              completion_queue_->grpc_completion_queue()));
    req_ctx->response_reader_->StartCall();
    req_ctx->response_reader_->Finish(&req_ctx->response_, &req_ctx->status_,
      (void*)finish_event_);
  }

  void HandleEvent(const CompletionQueue::ClientEvent& client_event) {
    switch (client_event.event) {
      case CompletionQueue::ClientEvent::Event::FINISH:
        HandleFinishEvent(client_event);
        break;
      default:
        ;
    }
    delete client_event.req_ctx;
  }

  void HandleFinishEvent(const CompletionQueue::ClientEvent& client_event) {
    if (callback_) {
      auto status = client_event.req_ctx->status_;
      auto response = client_event.req_ctx->response_;
      callback_(status, status.ok() ? &response : nullptr);
    }
  }

 private:
  std::shared_ptr<Math::Stub> stub_;
  CompletionQueue* completion_queue_;
  CallbackType callback_;
};
