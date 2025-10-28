# async_demo.py
"""
Demo: so sánh tuần tự vs asyncio (I/O-bound) vs ThreadPool (blocking I/O) vs ProcessPool (CPU-bound).
Chạy: python async_demo.py
"""
import asyncio
import time
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
import multiprocessing
import random
import math

# --- parameters
NUM_TASKS = 100      # số tác vụ đồng thời mô phỏng
IO_SLEEP = 0.05      # mỗi tác vụ I/O giả chờ bao nhiêu giây (asyncio.sleep)
BLOCK_SLEEP = 0.05   # mỗi tác vụ blocking I/O dùng time.sleep
CPU_WORK = 100000    # độ lớn công việc CPU cho mô phỏng

# --- helper functions
def blocking_io_task(idx):
    """Hàm blocking mô phỏng I/O (time.sleep)"""
    import time
    t0 = time.time()
    time.sleep(BLOCK_SLEEP)
    return (idx, time.time() - t0)

async def async_io_task(idx):
    """I/O bất đồng bộ giả (asyncio.sleep)"""
    t0 = time.time()
    await asyncio.sleep(IO_SLEEP)
    return (idx, time.time() - t0)

def cpu_bound_task(n):
    """Mô phỏng công việc CPU-bound: tính tổng sin cos"""
    s = 0.0
    for i in range(n):
        s += math.sin(i) * math.cos(i/2.0)
    return s

# --- runners / benchmarks
def run_sequential_io(num_tasks):
    t0 = time.time()
    results = []
    for i in range(num_tasks):
        _, dt = blocking_io_task(i)
        results.append(dt)
    return time.time() - t0

async def run_asyncio_io(num_tasks):
    t0 = time.time()
    tasks = [asyncio.create_task(async_io_task(i)) for i in range(num_tasks)]
    res = await asyncio.gather(*tasks)
    return time.time() - t0

def run_threadpool_io(num_tasks, max_workers=20):
    t0 = time.time()
    with ThreadPoolExecutor(max_workers=max_workers) as ex:
        futures = [ex.submit(blocking_io_task, i) for i in range(num_tasks)]
        # đợi tất cả
        for f in futures:
            f.result()
    return time.time() - t0

def run_process_pool_cpu(num_tasks, per_task_work=CPU_WORK, max_workers=None):
    t0 = time.time()
    if max_workers is None:
        max_workers = multiprocessing.cpu_count()
    with ProcessPoolExecutor(max_workers=max_workers) as ex:
        futures = [ex.submit(cpu_bound_task, per_task_work) for _ in range(num_tasks)]
        for f in futures:
            f.result()
    return time.time() - t0

def run_sequential_cpu(num_tasks, per_task_work=CPU_WORK):
    t0 = time.time()
    for _ in range(num_tasks):
        cpu_bound_task(per_task_work)
    return time.time() - t0

# --- main demo
def main():
    print("Async programming demo (Python asyncio)")
    print(f"NUM_TASKS={NUM_TASKS}, IO_SLEEP={IO_SLEEP}, BLOCK_SLEEP={BLOCK_SLEEP}, CPU_WORK={CPU_WORK}\n")

    print("1) Sequential blocking I/O (time.sleep)")
    t_seq_io = run_sequential_io(NUM_TASKS)
    print(f"  Time: {t_seq_io:.3f}s")

    print("\n2) ThreadPool (parallel blocking I/O)")
    t_thread_io = run_threadpool_io(NUM_TASKS, max_workers=50)
    print(f"  Time: {t_thread_io:.3f}s")

    print("\n3) asyncio (non-blocking I/O)")
    t_asyncio_io = asyncio.run(run_asyncio_io(NUM_TASKS))
    print(f"  Time: {t_asyncio_io:.3f}s")

    print("\n--- I/O results summary ---")
    print(f"Seq I/O: {t_seq_io:.3f}s | ThreadPool I/O: {t_thread_io:.3f}s | asyncio I/O: {t_asyncio_io:.3f}s")

    # CPU-bound
    small = max(1, int(NUM_TASKS/4))
    print("\n4) Sequential CPU-bound")
    t_seq_cpu = run_sequential_cpu(small, CPU_WORK)
    print(f"  Time (sequential for {small} tasks): {t_seq_cpu:.3f}s")

    print("\n5) ProcessPool CPU-bound (multi-process)")
    t_proc_cpu = run_process_pool_cpu(small, CPU_WORK, max_workers=min(multiprocessing.cpu_count(), 8))
    print(f"  Time (process pool for {small} tasks): {t_proc_cpu:.3f}s")

    print("\n--- CPU results summary ---")
    print(f"Seq CPU: {t_seq_cpu:.3f}s | ProcessPool CPU: {t_proc_cpu:.3f}s")

    print("\nGhi chú: với I/O-bound, asyncio thường nhanh hơn vì không tạo thread/overhead. Với CPU-bound, process pool tận dụng nhiều CPU core.")

if __name__ == "__main__":
    main()
