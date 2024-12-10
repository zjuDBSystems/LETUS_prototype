#include <vector>
#include <future>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

class AsyncVerifier {
private:
    struct VerifyTask {
        std::vector<std::string> keys;
        std::promise<bool> promise;
    };

    Client* client_;
    std::thread worker_thread_;
    std::queue<VerifyTask> task_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;

    void WorkerThread() {
        while (true) {
            VerifyTask task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { 
                    return !task_queue_.empty() || stop_; 
                });
                
                if (stop_ && task_queue_.empty()) {
                    return;
                }
                
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
            
            bool result = client_->Verify(task.keys);
            task.promise.set_value(result);
        }
    }

public:
    AsyncVerifier(Client* client) : client_(client) {
        worker_thread_ = std::thread(&AsyncVerifier::WorkerThread, this);
    }

    ~AsyncVerifier() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_one();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    std::future<bool> AsyncVerify(const std::vector<std::string>& keys) {
        VerifyTask task;
        task.keys = keys;
        std::future<bool> future = task.promise.get_future();
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            task_queue_.push(std::move(task));
        }
        cv_.notify_one();
        
        return future;
    }
};

// 使用示例：
void YourMainFunction() {
    Client* client = new Client();
    AsyncVerifier verifier(client);
    
    std::vector<std::future<bool>> pending_results;
    
    for (int i = 0; i < num_iterations; i++) {
        // 你的其他处理逻辑
        // ...
        
        // 异步提交verify任务
        std::vector<std::string> unverified_keys = /* 获取需要验证的keys */;
        std::future<bool> future = verifier.AsyncVerify(unverified_keys);
        pending_results.push_back(std::move(future));
        
        // 检查前一个verify是否完成
        if (i > 0) {
            bool verify_result = pending_results[i-1].get();
            if (!verify_result) {
                // 处理验证失败的情况
            }
        }
    }
    
    // 等待最后一个验证完成
    if (!pending_results.empty()) {
        bool last_result = pending_results.back().get();
        if (!last_result) {
            // 处理验证失败的情况
        }
    }
    
    delete client;
}