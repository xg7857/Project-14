#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

typedef enum { FIRST, RANDOM, MEDIAN3 } PivotType;

static long long cmp_count = 0;
static int track_stack = 0;
static uintptr_t stack_base = 0;
static uintptr_t stack_min = 0;
static int *tmp_buf = NULL;

static void track(void) {
    char marker;
    uintptr_t addr = (uintptr_t)&marker;
    if (addr < stack_min) stack_min = addr;
}

static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }

static int select_pivot(int a[], int lo, int hi, PivotType type) {
    if (type == FIRST) return lo;
    if (type == RANDOM) return lo + rand() % (hi - lo + 1);

    int mid = lo + (hi - lo) / 2;
    int x = a[lo], y = a[mid], z = a[hi];
    cmp_count++;
    if (x < y) {
        cmp_count++;
        if (y < z) return mid;
        cmp_count++;
        if (x < z) return hi;
        return lo;
    } else {
        cmp_count++;
        if (x < z) return lo;
        cmp_count++;
        if (y < z) return hi;
        return mid;
    }
}

static int partition(int a[], int lo, int hi, PivotType type) {
    int p = select_pivot(a, lo, hi, type);
    swap(&a[lo], &a[p]);
    int pivot = a[lo];
    int i = lo;
    for (int j = lo + 1; j <= hi; j++) {
        cmp_count++;
        if (a[j] < pivot) swap(&a[++i], &a[j]);
    }
    swap(&a[lo], &a[i]);
    return i;
}

static void quick_sort(int a[], int lo, int hi, PivotType type) {
    if (track_stack) track();
    if (lo >= hi) return;
    int p = partition(a, lo, hi, type);
    quick_sort(a, lo, p - 1, type);
    quick_sort(a, p + 1, hi, type);
}

static void merge(int a[], int lo, int mid, int hi) {
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi) {
        cmp_count++;
        if (a[i] <= a[j]) tmp_buf[k++] = a[i++];
        else tmp_buf[k++] = a[j++];
    }
    while (i <= mid) tmp_buf[k++] = a[i++];
    while (j <= hi) tmp_buf[k++] = a[j++];
    for (k = lo; k <= hi; k++) a[k] = tmp_buf[k];
}

static void merge_sort(int a[], int lo, int hi) {
    if (track_stack) track();
    if (lo >= hi) return;
    int mid = lo + (hi - lo) / 2;
    merge_sort(a, lo, mid);
    merge_sort(a, mid + 1, hi);
    merge(a, lo, mid, hi);
}

static void sift_down(int a[], int start, int size) {
    if (track_stack) track();
    int i = start;
    while (2 * i + 1 < size) {
        int c = 2 * i + 1;
        if (c + 1 < size) {
            cmp_count++;
            if (a[c + 1] > a[c]) c++;
        }
        cmp_count++;
        if (a[c] > a[i]) {
            swap(&a[c], &a[i]);
            i = c;
        } else {
            break;
        }
    }
}

static void heap_sort(int a[], int n) {
    if (track_stack) track();
    for (int i = n / 2 - 1; i >= 0; i--) sift_down(a, i, n);
    for (int end = n - 1; end > 0; end--) {
        swap(&a[0], &a[end]);
        sift_down(a, 0, end);
    }
}

static void run(int algo, int a[], int n) {
    if (algo == 0) heap_sort(a, n);
    else if (algo == 1) merge_sort(a, 0, n - 1);
    else quick_sort(a, 0, n - 1, (PivotType)(algo - 2));
}

static int is_sorted(const int a[], int n) {
    for (int i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0;
    return 1;
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void) {
    int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    const char *names[] = {"Heap Sort", "Merge Sort", "Quick (First)", "Quick (Random)", "Quick (Median3)"};
    const int reps = 200;
    srand(12345);

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        int *orig = malloc(sizeof(int) * n);
        int *work = malloc(sizeof(int) * n);
        tmp_buf = malloc(sizeof(int) * n);
        for (int i = 0; i < n; i++) orig[i] = rand() % 1000000;

        printf("Data Size : %d\n\n", n);
        printf("%-15s %-14s %-11s %s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        printf("--------------------------------------------------------\n");

        for (int algo = 0; algo < 5; algo++) {
            memcpy(work, orig, sizeof(int) * n);
            cmp_count = 0;
            track_stack = 1;
            char base;
            stack_base = (uintptr_t)&base;
            stack_min = stack_base;
            run(algo, work, n);
            track_stack = 0;
            long long comparisons = cmp_count;
            long long stack_bytes = (long long)(stack_base - stack_min);
            long long extra = stack_bytes + (algo == 1 ? (long long)sizeof(int) * n : 0);
            if (!is_sorted(work, n)) { printf("ERROR: not sorted\n"); return 1; }

            double total = 0;
            for (int r = 0; r < reps; r++) {
                memcpy(work, orig, sizeof(int) * n);
                double st = now_ms();
                run(algo, work, n);
                total += now_ms() - st;
                if (!is_sorted(work, n)) { printf("ERROR: not sorted\n"); return 1; }
            }
            printf("%-15s %-14lld %-11.4f %lld\n", names[algo], comparisons, total / reps, extra);
        }
        printf("\n");
        free(orig); free(work); free(tmp_buf);
    }
    return 0;
}
