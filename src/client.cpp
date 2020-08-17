#include <memory>

#include "client.hpp"

void CompletionQueue::Initialize() {
  if (initialized_) {
    return;
  }
  Start();

  initialized_ = true;
}

CompletionQueue* CompletionQueue::completion_queue() {
  static CompletionQueue* const kInstance = new CompletionQueue();
  return kInstance;
}

CompletionQueue* CompletionQueue::GetCompletionQueue() {
  CompletionQueue *cq = completion_queue();
  cq->Initialize();
  return cq;
}
AsyncClient::ClientEvent *AsyncClient::GetClientEvent(
    ClientEvent::Event event, AsyncClient* async_client) {
    return new ClientEvent(event, async_client);
}

void CompletionQueue::Start() {
  thread_ =
      std::make_unique<std::thread>([this]() { RunCompletionQueue(); });
}

void CompletionQueue::Shutdown() {
  grpc_completion_queue_.Shutdown();
  thread_->join();
}

void CompletionQueue::RunCompletionQueue() {
  bool ok;
  void* tag;
  while (grpc_completion_queue_.Next(&tag, &ok)) {
    auto client_event = static_cast<AsyncClient::ClientEvent *>(tag);
    client_event->ok = ok;
    client_event->async_client->HandleEvent(*client_event);
  }
}
