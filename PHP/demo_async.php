<?php
// demo_async.php
// Chạy: php demo_async.php
// Mô phỏng kỹ thuật bất đồng bộ trong PHP thuần

echo "\n--- MÔ PHỎNG KỸ THUẬT BẤT ĐỒNG BỘ TRONG PHP ---\n";

function showMenu() {
    echo "\n1) Async I/O (giả lập tải file)\n";
    echo "2) Multiple Tasks đồng thời (simulation)\n";
    echo "3) Producer-Consumer Queue\n";
    echo "4) Semaphore (giới hạn số tác vụ)\n";
    echo "5) Cancellation/Timeout\n";
    echo "0) Thoát\n";
    echo "Lựa chọn: ";
}

while (true) {
    showMenu();
    $choice = trim(fgets(STDIN));

    switch ($choice) {
        case "1": demoAsyncIO(); break;
        case "2": demoMultipleTasks(); break;
        case "3": demoProducerConsumer(); break;
        case "4": demoSemaphore(); break;
        case "5": demoCancellation(); break;
        case "0": exit;
        default: echo "Lựa chọn không hợp lệ\n";
    }
}

// ------------------- Demo functions -------------------

// 1. Async I/O (giả lập tải file)
function demoAsyncIO() {
    echo "--- Demo Async I/O ---\n";
    $tasks = [];
    for ($i=1; $i<=5; $i++) {
        $tasks[] = function() use ($i) {
            $wait = rand(300,1000);
            usleep($wait*1000);
            echo "[OK] File $i tải xong (wait $wait ms)\n";
        };
    }
    // Chạy tất cả "đồng thời" (simulated)
    foreach ($tasks as $task) $task();
    echo "Hoàn thành Async I/O\n";
}

// 2. Multiple Tasks đồng thời
function demoMultipleTasks() {
    echo "--- Demo Multiple Tasks ---\n";
    $tasks = [];
    for ($i=1; $i<=5; $i++) {
        $tasks[] = function() use ($i) {
            $wait = rand(100,500);
            usleep($wait*1000);
            echo "Task $i done (wait $wait ms)\n";
        };
    }
    foreach ($tasks as $task) $task();
    echo "Hoàn thành tất cả Tasks\n";
}

// 3. Producer-Consumer Queue
function demoProducerConsumer() {
    echo "--- Demo Producer-Consumer ---\n";
    $queue = new SplQueue();

    // Producer
    for ($i=1; $i<=10; $i++) {
        $queue->enqueue($i);
        echo "Produced $i\n";
        usleep(100*1000); // 100ms
    }

    // Consumer
    $consumer_count = 2;
    for ($c=1; $c<=$consumer_count; $c++) {
        foreach ($queue as $item) {
            echo "Consumer $c processed $item\n";
            usleep(200*1000);
        }
    }
    echo "Hoàn thành Producer-Consumer\n";
}

// 4. Semaphore (giới hạn số tác vụ đồng thời)
function demoSemaphore() {
    echo "--- Demo Semaphore (giới hạn 3 tác vụ đồng thời) ---\n";
    $tasks = [];
    for ($i=1; $i<=6; $i++) {
        $tasks[] = $i;
    }

    $semaphore = 3;
    $running = 0;

    foreach ($tasks as $task) {
        if ($running >= $semaphore) {
            usleep(500*1000); // chờ slot trống
            $running--;
        }
        echo "Task $task bắt đầu\n";
        usleep(rand(200,500)*1000);
        echo "Task $task hoàn thành\n";
        $running++;
    }
    echo "Hoàn thành Semaphore demo\n";
}

// 5. Cancellation / Timeout
function demoCancellation() {
    echo "--- Demo Cancellation / Timeout ---\n";
    $timeout = 3; // giây
    $start = time();

    for ($i=1; $i<=10; $i++) {
        if ((time() - $start) >= $timeout) {
            echo "Hủy tác vụ tại bước $i do timeout\n";
            break;
        }
        echo "Đang xử lý bước $i\n";
        usleep(700*1000); // 700ms
    }
    echo "Hoàn thành Cancellation demo\n";
}
