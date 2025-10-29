import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

public class AsyncDemo {
    private static final Scanner scanner = new Scanner(System.in);

    public static void main(String[] args) throws Exception {
        while (true) {
            System.out.println("\n--- Mô phỏng các kỹ thuật bất đồng bộ trong Java ---");
            System.out.println("1) CompletableFuture (mô phỏng I/O-bound)");
            System.out.println("2) Parallel Stream (tác vụ CPU)");
            System.out.println("3) CompletableFuture.allOf (chạy đồng thời)");
            System.out.println("4) Producer - Consumer (BlockingQueue)");
            System.out.println("5) BlockingQueue + stream (luồng dữ liệu bất đồng bộ)");
            System.out.println("6) Semaphore (giới hạn tác vụ song song)");
            System.out.println("7) Hủy tác vụ (Future + cancel)");
            System.out.println("0) Thoát");
            System.out.print("Lựa chọn: ");

            String key = scanner.nextLine();
            System.out.println();

            switch (key) {
                case "1": demoAsyncAwait(); break;
                case "2": demoParallelFor(); break;
                case "3": demoWhenAll(); break;
                case "4": demoProducerConsumer(); break;
                case "5": demoAsyncStream(); break;
                case "6": demoSemaphore(); break;
                case "7": demoCancellation(); break;
                case "0": return;
                default: System.out.println("Lựa chọn không hợp lệ."); break;
            }
        }
    }

    // 1. Async/Await mô phỏng I/O
    static void demoAsyncAwait() {
        System.out.println("Demo: CompletableFuture — mô phỏng tải dữ liệu I/O");

        List<String> urls = IntStream.rangeClosed(1, 5)
                .mapToObj(i -> "https://api/" + i)
                .collect(Collectors.toList());

        long start = System.currentTimeMillis();

        List<CompletableFuture<String>> futures = urls.stream()
                .map(AsyncDemo::giaLapTaiDuLieuAsync)
                .collect(Collectors.toList());

        List<String> results = futures.stream()
                .map(CompletableFuture::join)
                .collect(Collectors.toList());

        long end = System.currentTimeMillis();

        System.out.printf("Hoàn thành %d tác vụ trong %d ms%n", results.size(), (end - start));
        results.forEach(System.out::println);
    }

    static CompletableFuture<String> giaLapTaiDuLieuAsync(String url) {
        return CompletableFuture.supplyAsync(() -> {
            try {
                int cho = ThreadLocalRandom.current().nextInt(300, 1000);
                Thread.sleep(cho);
                return "[OK] " + url + " (chờ " + cho + " ms)";
            } catch (InterruptedException e) {
                return "[Lỗi] " + url;
            }
        });
    }

    // 2. Parallel Stream (CPU-bound)
    static void demoParallelFor() {
        System.out.println("Demo: Parallel Stream — tính toán CPU song song");
        int n = 20_000_000;
        System.out.printf("Tính tổng sqrt(i) với i = 1..%d%n", n);

        long start = System.currentTimeMillis();
        double sum = IntStream.rangeClosed(1, n)
                .parallel()
                .mapToDouble(Math::sqrt)
                .sum();
        long end = System.currentTimeMillis();

        System.out.printf("Kết quả ~ %.0f (mất %d ms)%n", sum, (end - start));
    }

    // 3. CompletableFuture.allOf
    static void demoWhenAll() {
        System.out.println("Demo: CompletableFuture.allOf — chạy nhiều tác vụ đồng thời");
        long start = System.currentTimeMillis();

        List<CompletableFuture<Integer>> futures = new ArrayList<>();
        for (int i = 0; i < 15; i++) {
            int id = i;
            futures.add(CompletableFuture.supplyAsync(() -> congViecGiaLap(id)));
        }

        CompletableFuture.allOf(futures.toArray(new CompletableFuture[0])).join();

        List<Integer> results = futures.stream().map(CompletableFuture::join).toList();

        long end = System.currentTimeMillis();
        System.out.printf("Hoàn thành %d tác vụ trong %d ms%n", results.size(), (end - start));
        System.out.println("Kết quả: " + results);
    }

    static int congViecGiaLap(int id) {
        for (int i = 0; i < 150_000; i++); // spin
        try { Thread.sleep(50); } catch (InterruptedException ignored) {}
        return id * id;
    }

    // 4. Producer - Consumer (BlockingQueue)
    static void demoProducerConsumer() throws InterruptedException {
        System.out.println("Demo: Producer - Consumer với BlockingQueue");
        BlockingQueue<Integer> queue = new ArrayBlockingQueue<>(10);

        ExecutorService executor = Executors.newCachedThreadPool();

        // Producer
        executor.submit(() -> {
            try {
                for (int i = 1; i <= 20; i++) {
                    queue.put(i);
                    System.out.println("Tạo " + i);
                    Thread.sleep(70);
                }
            } catch (InterruptedException ignored) {}
        });

        // 3 Consumers
        for (int id = 1; id <= 3; id++) {
            int cid = id;
            executor.submit(() -> {
                try {
                    while (true) {
                        Integer x = queue.poll(1, TimeUnit.SECONDS);
                        if (x == null) break;
                        System.out.printf("  Người %d xử lý %d%n", cid, x);
                        Thread.sleep(200);
                    }
                    System.out.printf("  Người %d xong việc%n", cid);
                } catch (InterruptedException ignored) {}
            });
        }

        executor.shutdown();
        executor.awaitTermination(5, TimeUnit.SECONDS);
        System.out.println("Kết thúc mô phỏng Producer-Consumer");
    }

    // 5. Async Stream (giả lập Channel)
    static void demoAsyncStream() throws InterruptedException {
        System.out.println("Demo: BlockingQueue + Stream — mô phỏng luồng dữ liệu bất đồng bộ");
        BlockingQueue<Integer> queue = new ArrayBlockingQueue<>(5);
        AtomicBoolean done = new AtomicBoolean(false);

        ExecutorService executor = Executors.newCachedThreadPool();

        // Writer
        executor.submit(() -> {
            try {
                for (int i = 1; i <= 15; i++) {
                    queue.put(i);
                    System.out.println("[Ghi] " + i);
                    Thread.sleep(100);
                }
            } catch (InterruptedException ignored) {
            } finally {
                done.set(true);
            }
        });

        // Reader
        executor.submit(() -> {
            try {
                while (!done.get() || !queue.isEmpty()) {
                    Integer v = queue.poll(1, TimeUnit.SECONDS);
                    if (v != null) {
                        System.out.println("[Đọc] " + v);
                        Thread.sleep(250);
                    }
                }
            } catch (InterruptedException ignored) {}
        });

        executor.shutdown();
        executor.awaitTermination(5, TimeUnit.SECONDS);
        System.out.println("Kết thúc mô phỏng Channel");
    }

    // 6. Semaphore (giới hạn song song)
    static void demoSemaphore() throws InterruptedException {
        System.out.println("Demo: Semaphore — giới hạn số tác vụ chạy đồng thời");
        Semaphore semaphore = new Semaphore(3);
        ExecutorService executor = Executors.newCachedThreadPool();

        for (int i = 1; i <= 10; i++) {
            int id = i;
            executor.submit(() -> {
                try {
                    semaphore.acquire();
                    System.out.println("Tác vụ " + id + " bắt đầu");
                    Thread.sleep(700 + id * 50);
                    System.out.println("Tác vụ " + id + " hoàn thành");
                } catch (InterruptedException ignored) {
                } finally {
                    semaphore.release();
                }
            });
        }

        executor.shutdown();
        executor.awaitTermination(10, TimeUnit.SECONDS);
        System.out.println("Tất cả tác vụ đã hoàn thành");
    }

    // 7. Cancellation (Future.cancel)
    static void demoCancellation() throws Exception {
        System.out.println("Demo: Hủy tác vụ bằng Future.cancel()");
        ExecutorService executor = Executors.newSingleThreadExecutor();

        Future<?> task = executor.submit(() -> {
            try {
                for (int i = 1; i <= 20; i++) {
                    System.out.println("Đang xử lý bước " + i);
                    Thread.sleep(700);
                }
                System.out.println("Tác vụ đã hoàn thành bình thường");
            } catch (InterruptedException e) {
                System.out.println("Tác vụ bị hủy!");
            }
        });

        System.out.println("Nhấn ENTER để hủy trong 5 giây...");
        CompletableFuture.runAsync(() -> {
            try {
                System.in.read();
                task.cancel(true);
            } catch (Exception ignored) {}
        });

        executor.shutdown();
        executor.awaitTermination(10, TimeUnit.SECONDS);
    }
}
