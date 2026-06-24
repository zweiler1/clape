package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"
)

func mod(x, y int64) int64 {
	return ((x % y + y) % y)
}

func parse(line string) int64 {
	if line == "" {
		return 0
	}
	val, _ := strconv.ParseInt(line[1:], 10, 64)
	if line[0] == 'L' {
		return -val
	}
	return val
}

func part1(nums []int64) {
	dial := int64(50)
	count := int64(0)
	for _, x := range nums {
		dial = mod(dial+x, 100)
		if dial == 0 {
			count++
		}
	}
	fmt.Printf("part_1: %d\n", count)
}

func abs(x int64) int64 {
	if x < 0 {
		return -x
	}
	return x
}

func part2(nums []int64) {
	dial := int64(50)
	count := int64(0)
	for _, x := range nums {
		if dial > 0 && x < 0 && dial+x < 0 {
			count++
		}
		adj := abs(dial+x) - 1
		count += adj / 100
		dial = mod(dial+x, 100)
		if dial == 0 {
			count++
		}
	}
	fmt.Printf("part_2: %d\n", count)
}

func main() {
	data, _ := os.ReadFile("input.txt")
	parts := strings.Split(string(data), "\n")
	nums := make([]int64, len(parts))
	for i, line := range parts {
		nums[i] = parse(line)
	}
	part1(nums)
	part2(nums)
}
