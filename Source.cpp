#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

using namespace std;
using namespace std::chrono;

const int N = 100000000;       // ������ �������
const int MIN = 100000;        // ������ ������� ��������� �����
const int MAX = 1000000;       // ������� ������� ��������� �����

// ��������� ������� ��������� �����
vector<int> generate_array() {
    vector<int> a(N);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(MIN, MAX);
    for (int& x : a) x = dis(gen);
    return a;
}

// �������� �������� �����
bool is_prime(int x) {
    if (x < 2) return false;
    for (int i = 2; i * 1LL * i <= x; ++i)
        if (x % i == 0) return false;
    return true;
}

// ������������ �������
int count_primes_single(const vector<int>& a) {
    int cnt = 0;
    for (int x : a)
        if (is_prime(x)) ++cnt;
    return cnt;
}

// ������������� �������
int count_primes_multi(const vector<int>& a, int threads) {
    vector<thread> ts;
    vector<int> results(threads);
    int len = a.size() / threads;

    for (int t = 0; t < threads; ++t) {
        ts.emplace_back([&, t]() {
            int start = t * len;
            int end = (t == threads - 1) ? a.size() : start + len;
            int local_count = 0;
            for (int i = start; i < end; ++i)
                if (is_prime(a[i])) ++local_count;
            results[t] = local_count;
            });
    }

    for (auto& th : ts) th.join();
    int total = 0;
    for (int x : results) total += x;
    return total;
}

// ����� ���� �������
class ThreadPool {
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool stop = false;

public:
    ThreadPool(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(mtx);
                        cv.wait(lock, [this]() { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    void enqueue(function<void()> task) {
        {
            unique_lock<mutex> lock(mtx);
            tasks.push(move(task));
        }
        cv.notify_one();
    }

    void shutdown() {
        {
            unique_lock<mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        for (thread& worker : workers)
            worker.join();
    }
};

// ������� ������� ����� � �������������� ���� �������
int count_primes_pool(const vector<int>& a, int threads) {
    ThreadPool pool(threads);
    mutex result_mutex;
    int result = 0;
    int len = a.size() / threads;

    for (int t = 0; t < threads; ++t) {
        int start = t * len;
        int end = (t == threads - 1) ? a.size() : start + len;
        pool.enqueue([&, start, end]() {
            int local = 0;
            for (int i = start; i < end; ++i)
                if (is_prime(a[i])) ++local;
            lock_guard<mutex> lock(result_mutex);
            result += local;
            });
    }

    pool.shutdown();
    return result;
}

int main() {
    system("chcp 65001 > nul");


    cout << "��������� �������...\n";
    auto a = generate_array();

    cout << "������������ �������...\n";
    auto t1 = steady_clock::now();
    int single = count_primes_single(a);
    auto t2 = steady_clock::now();
    cout << "������� ����� (���� �����): " << single << ", �����: "
        << duration_cast<milliseconds>(t2 - t1).count() << " ��\n";

    cout << "������������� �������...\n";
    int threads = thread::hardware_concurrency();
    t1 = steady_clock::now();
    int multi = count_primes_multi(a, threads);
    t2 = steady_clock::now();
    cout << "������� ����� (����� �������): " << multi << ", �����: "
        << duration_cast<milliseconds>(t2 - t1).count() << " ��\n";

    cout << "������� � �������������� ���� �������...\n";
    t1 = steady_clock::now();
    int pooled = count_primes_pool(a, threads);
    t2 = steady_clock::now();
    cout << "������� ����� (��� �������): " << pooled << ", �����: "
        << duration_cast<milliseconds>(t2 - t1).count() << " ��\n";

    return 0;
}
