#include <thread>
#include <queue>

#include <grpcpp/grpcpp.h>

#include "proto/math_service.pb.h"
#include "proto/math_service.grpc.pb.h"

using RequestType = GetSquareRequest;
using ResponseType = GetSquareResponse;
using CallbackType =
    std::function<void(const grpc::Status&, const ResponseType*)>;

class AsyncClient;

class CompletionQueue {
 public:
  struct ClientEvent : std::enable_shared_from_this<ClientEvent> {
    friend class CompletionQueue;
   public:
    enum class Event { FINISH = 0, READ = 1, WRITE = 2 };
    Event event;
    AsyncClient* async_client;
    grpc::ClientContext client_context_;
    grpc::Status status_;
    ResponseType response_;
    std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>> response_reader_;
    bool ok = false;
   private:
    ClientEvent(Event event, AsyncClient* async_client)
        : event(event), async_client(async_client) {}
  };

 public:
  CompletionQueue() = default;
  ~CompletionQueue() = default;

  void Start();
  void Shutdown();
  static CompletionQueue *GetCompletionQueue();
  std::shared_ptr<ClientEvent> GetClientEvent(
    ClientEvent::Event event, AsyncClient* async_client);

  grpc::CompletionQueue* grpc_completion_queue() { return &grpc_completion_queue_; }

 private:
  void Initialize();
  void RunCompletionQueue();

  static CompletionQueue* completion_queue();
  grpc::CompletionQueue grpc_completion_queue_;
  std::unique_ptr<std::thread> thread_;
  bool initialized_ = false;
};

class AsyncClient
{
 public:
  AsyncClient(std::shared_ptr<Math::Stub> stub, CallbackType callback)
      : stub_(stub),
        callback_(callback),
        completion_queue_(CompletionQueue::GetCompletionQueue()),
        m_(std::make_shared<std::mutex>()) {}

  ~AsyncClient() { Stop(); };

  void WriteAsync(const RequestType& request) {
    auto finish_event_ = completion_queue_->GetClientEvent(
      CompletionQueue::ClientEvent::Event::FINISH, this);
    gone_.push(std::make_pair(request.input(), finish_event_));
    finish_event_->response_reader_ =
        std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>>(
            stub_->PrepareAsyncGetSquare(&finish_event_->client_context_, request,
              completion_queue_->grpc_completion_queue()));
    finish_event_->response_reader_->StartCall();
    finish_event_->response_reader_->Finish(&finish_event_->response_,
      &finish_event_->status_, (void*)finish_event_.get());
  }

  void HandleEvent(const CompletionQueue::ClientEvent& client_event) {
    switch (client_event.event) {
      case CompletionQueue::ClientEvent::Event::FINISH:
        HandleFinishEvent(client_event);
        break;
      default:
        ;
    }
  }

  void HandleFinishEvent(const CompletionQueue::ClientEvent& client_event) {
    if (callback_) {
      auto status = client_event.status_;
      auto response = client_event.response_;
      callback_(status, status.ok() ? &response : nullptr);

      std::lock_guard<std::mutex> lock_guard(*client_event.async_client->m_);
      returned_.push(response.output());
    }
  }

  int GetResult() {
    std::lock_guard<std::mutex> lock_guard(*m_);
    if (returned_.empty() || gone_.empty())
      return -1;

    if (returned_.top() == gone_.front().first) {
      auto ret = gone_.front().first;
      returned_.pop();
      gone_.pop();
      return ret;
    } else {
      return -1;
    }
  }

  void Stop() {
    completion_queue_->Shutdown();
  }

 private:
  std::queue<std::pair<int, std::shared_ptr<CompletionQueue::ClientEvent>>> gone_;
  std::priority_queue<int, std::vector<int>, std::greater<int>> returned_;
  std::shared_ptr<Math::Stub> stub_;
  CompletionQueue* completion_queue_;
  CallbackType callback_;
  std::shared_ptr<std::mutex> m_;
};
