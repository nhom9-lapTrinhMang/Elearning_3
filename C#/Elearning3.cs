using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Channels;
using System.Threading.Tasks;

Console.OutputEncoding = System.Text.Encoding.UTF8;

await lapMang();

async Task lapMang()
{
    while (true)
    {
        Console.WriteLine("\n--- Mo phong cac ky thuat bat dong bo trong C# ---");
        Console.WriteLine("1) Async/await (mo phong tac vu I/O)");
        Console.WriteLine("2) Parallel.For (tac vu CPU)");
        Console.WriteLine("3) Tasks + Task.WhenAll (chay dong thoi)");
        Console.WriteLine("4) Producer - Consumer (BlockingCollection)");
        Console.WriteLine("5) Channel + IAsyncEnumerable (dong du lieu bat dong bo)");
        Console.WriteLine("6) Gioi han tac vu bang SemaphoreSlim");
        Console.WriteLine("7) Huy tac vu voi CancellationToken");
        Console.WriteLine("0) Thoat");
        Console.Write("Lua chon: ");
        var key = Console.ReadLine();
        Console.WriteLine();

        switch (key)
        {
            case "1": await demoAsyncAwait(); break;
            case "2": demoParallelFor(); break;
            case "3": await demoTasksWhenAll(); break;
            case "4": demoProducerConsumer(); break;
            case "5": await demoChannelAsync(); break;
            case "6": await demoSemaphore(); break;
            case "7": await demoCancellation(); break;
            case "0": return;
            default: Console.WriteLine("Lua chon khong hop le"); break;
        }
    }
}

// 1. Async/Await (mo phong I/O)
async Task demoAsyncAwait()
{
    Console.WriteLine("Demo: async/await — mo phong goi API nhieu lan (I/O-bound)");
    var dsUrl = Enumerable.Range(1, 5).Select(i => $"https://api/{i}").ToArray();

    var dongHo = System.Diagnostics.Stopwatch.StartNew();

    var congViec = dsUrl.Select(u => giaLapTaiDuLieuAsync(u)).ToArray();
    var ketQua = await Task.WhenAll(congViec);

    dongHo.Stop();
    Console.WriteLine($"Hoan thanh {ketQua.Length} tac vu trong {dongHo.ElapsedMilliseconds} ms");
    foreach (var kq in ketQua)
        Console.WriteLine(kq);
}

async Task<string> giaLapTaiDuLieuAsync(string url)
{
    var rd = new Random(Guid.NewGuid().GetHashCode());
    int cho = rd.Next(300, 1000);
    await Task.Delay(cho);
    return $"[OK] {url} (cho {cho} ms)";
}

// 2. Parallel.For (CPU-bound)
void demoParallelFor()
{
    Console.WriteLine("Demo: Parallel.For — chay nhieu tac vu CPU song song");
    int n = 20_000_000;
    Console.WriteLine($"Tinh tong sqrt(i) voi i = 1..{n}");

    var dongHo = System.Diagnostics.Stopwatch.StartNew();
    double tong = 0.0;
    object khoa = new object();

    Parallel.For(1, n + 1, () => 0.0, (i, state, cucBo) =>
    {
        cucBo += Math.Sqrt(i);
        return cucBo;
    }, cucBo =>
    {
        lock (khoa) tong += cucBo;
    });

    dongHo.Stop();
    Console.WriteLine($"Ket qua ~ {tong:N0} (mat {dongHo.ElapsedMilliseconds} ms)");
}

// 3. Task.WhenAll (chay nhieu tac vu)
async Task demoTasksWhenAll()
{
    Console.WriteLine("Demo: Task.WhenAll — tao nhieu tac vu chay dong thoi");
    var dongHo = System.Diagnostics.Stopwatch.StartNew();

    var dsTacVu = new List<Task<int>>();
    for (int i = 0; i < 15; i++)
    {
        int id = i;
        dsTacVu.Add(Task.Run(() => congViecGiaLap(id)));
    }

    int[] kq = await Task.WhenAll(dsTacVu);
    dongHo.Stop();
    Console.WriteLine($"Hoan thanh {kq.Length} tac vu trong {dongHo.ElapsedMilliseconds} ms");
    Console.WriteLine("Ket qua: " + string.Join(", ", kq));
}

int congViecGiaLap(int id)
{
    Thread.SpinWait(150_000);
    Thread.Sleep(50);
    return id * id;
}

// 4. Producer-Consumer (BlockingCollection)
void demoProducerConsumer()
{
    Console.WriteLine("Demo: Producer - Consumer voi BlockingCollection");
    var boDem = new BlockingCollection<int>(10);

    var nguoiTao = Task.Run(() =>
    {
        for (int i = 1; i <= 20; i++)
        {
            boDem.Add(i);
            Console.WriteLine($"Tao {i}");
            Thread.Sleep(70);
        }
        boDem.CompleteAdding();
    });

    var nguoiTieuThu = Enumerable.Range(1, 3).Select(id => Task.Run(() =>
    {
        foreach (var so in boDem.GetConsumingEnumerable())
        {
            Console.WriteLine($"  Nguoi {id} xu ly {so}");
            Thread.Sleep(200);
        }
        Console.WriteLine($"  Nguoi {id} xong viec");
    })).ToArray();

    Task.WaitAll(new[] { nguoiTao }.Concat(nguoiTieuThu).ToArray());
    Console.WriteLine("Ket thuc mo phong Producer-Consumer");
}

// 5. Channel + IAsyncEnumerable
async Task demoChannelAsync()
{
    Console.WriteLine("Demo: Channel + IAsyncEnumerable — mo phong luong du lieu bat dong bo");
    var kenh = Channel.CreateBounded<int>(new BoundedChannelOptions(5)
    {
        SingleWriter = true,
        SingleReader = false
    });

    var ghiKenh = Task.Run(async () =>
    {
        for (int i = 1; i <= 15; i++)
        {
            await kenh.Writer.WriteAsync(i);
            Console.WriteLine($"[Ghi] {i}");
            await Task.Delay(100);
        }
        kenh.Writer.Complete();
    });

    await foreach (var v in docDuLieuAsync(kenh.Reader))
    {
        Console.WriteLine($"[Doc] {v}");
        await Task.Delay(250);
    }

    await ghiKenh;
    Console.WriteLine("Ket thuc mo phong Channel");
}

async IAsyncEnumerable<int> docDuLieuAsync(ChannelReader<int> doc)
{
    while (await doc.WaitToReadAsync())
    {
        while (doc.TryRead(out var muc))
            yield return muc;
    }
}

// 6. SemaphoreSlim
async Task demoSemaphore()
{
    Console.WriteLine("Demo: SemaphoreSlim — gioi han so luong tac vu chay dong thoi");
    var semaphore = new SemaphoreSlim(3);
    var ds = new List<Task>();
    for (int i = 1; i <= 10; i++)
    {
        int id = i;
        await semaphore.WaitAsync();
        ds.Add(Task.Run(async () =>
        {
            try
            {
                Console.WriteLine($"Tac vu {id} bat dau");
                await Task.Delay(700 + id * 50);
                Console.WriteLine($"Tac vu {id} hoan thanh");
            }
            finally
            {
                semaphore.Release();
            }
        }));
    }
    await Task.WhenAll(ds);
    Console.WriteLine("Tat ca tac vu da hoan thanh");
}

// 7. CancellationToken
async Task demoCancellation()
{
    Console.WriteLine("Demo: Huy tac vu bang CancellationToken");
    using var cts = new CancellationTokenSource();

    var tacVuDai = tacVuDaiHanAsync(cts.Token);

    Console.WriteLine("Nhan 'c' de huy trong 5 giay...");
    var phim = Task.Run(() =>
    {
        var k = Console.ReadKey(intercept: true);
        if (k.KeyChar == 'c' || k.KeyChar == 'C')
            cts.Cancel();
    });

    cts.CancelAfter(TimeSpan.FromSeconds(5));

    try
    {
        await tacVuDai;
        Console.WriteLine("Tac vu da hoan thanh binh thuong");
    }
    catch (OperationCanceledException)
    {
        Console.WriteLine("Tac vu da bi huy");
    }
}

async Task tacVuDaiHanAsync(CancellationToken token)
{
    for (int i = 1; i <= 20; i++)
    {
        token.ThrowIfCancellationRequested();
        Console.WriteLine($"Dang xu ly buoc {i}");
        await Task.Delay(700, token);
    }
}
