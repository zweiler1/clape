#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int64_t mod(int64_t x, int64_t y) {
    return ((x % y + y) % y);
}

int main(void) {
    FILE *f = fopen("input.txt", "rb");
    if (!f) { fprintf(stderr, "Cannot open input.txt\n"); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)size + 1);
    fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[size] = '\0';

    int cap = 0;
    for (char *p = buf; *p; p++)
        if (*p == '\n') cap++;
    if (size > 0 && buf[size - 1] == '\n') cap++;

    int64_t *nums = (int64_t *)malloc((size_t)cap * sizeof(int64_t));
    int n = 0;
    char *line = buf;
    while (*line) {
        char *end = strchr(line, '\n');
        if (!end) end = line + strlen(line);

        if (line == end) {
            nums[n++] = 0;
        } else {
            int64_t sign = (*line == 'L') ? -1 : 1;
            int64_t val = 0;
            for (char *p = line + 1; p < end; p++)
                val = val * 10 + (*p - '0');
            nums[n++] = val * sign;
        }
        if (*end == '\0') break;
        line = end + 1;
    }
    if (size > 0 && buf[size - 1] == '\n')
        nums[n++] = 0;

    int64_t dial = 50, count = 0;
    for (int i = 0; i < n; i++) {
        dial = mod(dial + nums[i], 100);
        if (dial == 0) count++;
    }
    printf("part_1: %ld\n", (long)count);

    dial = 50;
    count = 0;
    for (int i = 0; i < n; i++) {
        int64_t x = nums[i];
        if (dial > 0 && x < 0 && dial + x < 0) count++;
        int64_t adj = llabs(dial + x) - 1;
        count += adj / 100;
        dial = mod(dial + x, 100);
        if (dial == 0) count++;
    }
    printf("part_2: %ld\n", (long)count);

    free(nums);
    free(buf);
    return 0;
}
