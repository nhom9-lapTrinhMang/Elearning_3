#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <atomic>
#include <algorithm>
#include <bits/stdc++.h>
using namespace std;



// -------------------- 1. Async/Await (I/O) --------------------
string giaLapTaiDuLieu(const string& url) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(300, 1000);
    int delay = dist(gen);
    this_thread::sleep_for(chrono::milliseconds(delay));
    return "[OK] " + url + " (chờ " + to_string(delay) + " ms)";
}

void demoAsyncAwait() {
    cout << "Demo 1: async/await — mô phỏng tải dữ liệu I/O\n";
    vector<string> dsUrl;
    for (int i = 1; i <= 5; ++i)
        dsUrl.push_back("https://api/" + to_string(i));

    auto start = chrono::high_resolution_clock::now();
    vector<future<string>> futures;
    for (auto& u : dsUrl)
        futures.push_back(async(launch::async, giaLapTaiDuLieu, u));

    for (auto& f : futures)
        cout << f.get() << endl;

    auto end = chrono::high_resolution_clock::now();
    cout << "Hoàn thành trong "
         << chrono::duration_cast<chrono::milliseconds>(end - start).count()
         << " ms\n";
}

// -------------------- 2. Parallel.For (CPU) --------------------
void demoParallelFor() {
    cout << "Demo 2: Parallel.For — chạy nhiều tác vụ CPU song song\n";
    const int n = 20'000'000;
    double tong = 0.0;
    mutex mtx;

    auto start = chrono::high_resolution_clock::now();
    int numThreads = thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    int chunk = n / numThreads;

    vector<thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        int startIdx = t * chunk + 1;
        int endIdx = (t == numThreads - 1) ? n : startIdx + chunk - 1;
        threads.emplace_back([&, startIdx, endIdx]() {
            double localSum = 0;
            for (int i = startIdx; i <= endIdx; ++i)
                localSum += sqrt(i);
            lock_guard<mutex> lock(mtx);
            tong += localSum;
        });
    }
    for (auto& th : threads) th.join();

    auto end = chrono::high_resolution_clock::now();
    cout << "Kết quả ~ " << tong << " (mất "
         << chrono::duration_cast<chrono::milliseconds>(end - start).count()
         << " ms)\n";
}

// -------------------- 3. Task.WhenAll --------------------
int congViecGiaLap(int id) {
    this_thread::sleep_for(chrono::milliseconds(50));
    return id * id;
}

void demoTasksWhenAll() {
    cout << "Demo 3: Task.WhenAll — tạo nhiều tác vụ chạy đồng thời\n";
    auto start = chrono::high_resolution_clock::now();
    vector<future<int>> futures;

    for (int i = 0; i < 15; ++i)
        futures.push_back(async(launch::async, congViecGiaLap, i));

    vector<int> kq;
    for (auto& f : futures) kq.push_back(f.get());

    auto end = chrono::high_resolution_clock::now();
    cout << "Hoàn thành " << kq.size() << " tác vụ trong "
         << chrono::duration_cast<chrono::milliseconds>(end - start).count()
         << " ms\n";
    cout << "Kết quả: ";
    for (auto v : kq) cout << v << " ";
    cout << endl;
}

// -------------------- 4. Producer - Consumer --------------------
class BlockingQueue {
    queue<int> q;
    mutex mtx;
    condition_variable cv;
    bool closed = false;
public:
    void push(int val) {
        unique_lock<mutex> lock(mtx);
        q.push(val);
        cv.notify_one();
    }
    bool pop(int& val) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return !q.empty() || closed; });
        if (q.empty()) return false;
        val = q.front();
        q.pop();
        return true;
    }
    void close() {
        unique_lock<mutex> lock(mtx);
        closed = true;
        cv.notify_all();
    }
};

void demoProducerConsumer() {
    cout << "Demo 4: Producer - Consumer với BlockingQueue\n";
    BlockingQueue q;

    auto producer = thread([&]() {
        for (int i = 1; i <= 20; ++i) {
            q.push(i);
            cout << "Tạo " << i << endl;
            this_thread::sleep_for(chrono::milliseconds(70));
        }
        q.close();
    });

    vector<thread> consumers;
    for (int id = 1; id <= 3; ++id) {
        consumers.emplace_back([&, id]() {
            int val;
            while (q.pop(val)) {
                cout << "  Người " << id << " xử lý " << val << endl;
                this_thread::sleep_for(chrono::milliseconds(200));
            }
            cout << "  Người " << id << " xong việc\n";
        });
    }

    producer.join();
    for (auto& c : consumers) c.join();
    cout << "Kết thúc mô phỏng Producer-Consumer\n";
}

// -------------------- 5. Channel bất đồng bộ --------------------
class AsyncChannel {
    queue<int> q;
    mutex mtx;
    condition_variable cv;
    bool done = false;
public:
    void write(int v) {
        unique_lock<mutex> lock(mtx);
        q.push(v);
        cv.notify_all();
    }
    bool read(int& out) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return !q.empty() || done; });
        if (q.empty()) return false;
        out = q.front();
        q.pop();
        return true;
    }
    void complete() {
        unique_lock<mutex> lock(mtx);
        done = true;
        cv.notify_all();
    }
};

void demoChannelAsync() {
    cout << "Demo 5: Channel — luồng dữ liệu bất đồng bộ\n";
    AsyncChannel chan;

    auto writer = thread([&]() {
        for (int i = 1; i <= 15; ++i) {
            chan.write(i);
            cout << "[Ghi] " << i << endl;
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        chan.complete();
    });

    auto reader = thread([&]() {
        int v;
        while (chan.read(v)) {
            cout << "  [Đọc] " << v << endl;
            this_thread::sleep_for(chrono::milliseconds(250));
        }
    });

    writer.join();
    reader.join();
    cout << "Kết thúc mô phỏng Channel\n";
}

// -------------------- 6. Giả lập SemaphoreSlim (C++14) --------------------
class SimpleSemaphore {
    mutex mtx;
    condition_variable cv;
    int permits;
public:
    explicit SimpleSemaphore(int count) : permits(count) {}
    void acquire() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return permits > 0; });
        --permits;
    }
    void release() {
        unique_lock<mutex> lock(mtx);
        ++permits;
        cv.notify_one();
    }
};

void demoSemaphore() {
    cout << "Demo 6: SemaphoreSlim (giả lập)\n";
    SimpleSemaphore sem(3);
    vector<thread> tasks;

    for (int i = 1; i <= 10; ++i) {
        tasks.emplace_back([i, &sem]() {
            sem.acquire();
            cout << "Tác vụ " << i << " bắt đầu\n";
            this_thread::sleep_for(chrono::milliseconds(700 + i * 50));
            cout << "Tác vụ " << i << " hoàn thành\n";
            sem.release();
        });
    }

    for (auto& t : tasks) t.join();
    cout << "Tất cả tác vụ đã hoàn thành\n";
}

// -------------------- 7. CancellationToken --------------------
void demoCancellation() {
    cout << "Demo 7: Hủy tác vụ bằng cờ atomic\n";
    atomic<bool> cancel{false};

    thread longTask([&]() {
        for (int i = 1; i <= 20; ++i) {
            if (cancel) {
                cout << "Tác vụ bị hủy tại bước " << i << endl;
                return;
            }
            cout << "Đang xử lý bước " << i << endl;
            this_thread::sleep_for(chrono::milliseconds(700));
        }
        cout << "Tác vụ hoàn thành bình thường.\n";
    });

    cout << "Nhấn Enter để hủy trong 5 giây...\n";
    auto start = chrono::steady_clock::now();
    while (chrono::steady_clock::now() - start < chrono::seconds(5)) {
        if (cin.peek() != EOF) {
            cancel = true;
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    cancel = true;
    longTask.join();
}

// -------------------- MAIN MENU --------------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    while (true) {
        cout << "\n--- Mô phỏng các kỹ thuật bất đồng bộ trong C++ ---\n";
        cout << "1) Async/await (I/O)\n";
        cout << "2) Parallel.For (CPU)\n";
        cout << "3) Task.WhenAll (đồng thời)\n";
        cout << "4) Producer-Consumer\n";
        cout << "5) Channel bất đồng bộ\n";
        cout << "6) SemaphoreSlim (giả lập)\n";
        cout << "7) CancellationToken\n";
        cout << "0) Thoát\n";
        cout << "Lựa chọn: ";

        string key;
        cin >> key;
        cout << endl;

        if (key == "1") demoAsyncAwait();
        else if (key == "2") demoParallelFor();
        else if (key == "3") demoTasksWhenAll();
        else if (key == "4") demoProducerConsumer();
        else if (key == "5") demoChannelAsync();
        else if (key == "6") demoSemaphore();
        else if (key == "7") demoCancellation();
        else if (key == "0") break;
        else cout << "Lựa chọn không hợp lệ!\n";
    }
    return 0;
}
