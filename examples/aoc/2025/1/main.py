def mod(x: int, y: int) -> int:
    return (x % y + y) % y


def part_1(numbers: list[int]) -> None:
    dial = 50
    count = 0
    for num in numbers:
        dial = mod(dial + num, 100)
        if dial == 0:
            count += 1
    print(f"part_1: {count}")


def part_2(numbers: list[int]) -> None:
    dial = 50
    count = 0
    for num in numbers:
        if dial > 0 and num < 0 and dial + num < 0:
            count += 1
        count += max(abs(dial + num) - 1, 0) // 100
        dial = mod(dial + num, 100)
        if dial == 0:
            count += 1
    print(f"part_2: {count}")


def parse(line: str) -> int:
    if not line:
        return 0
    val = int(line[1:])
    return val * -1 if line[0] == "L" else val


def main():
    with open("input.txt") as f:
        numbers = [parse(line) for line in f.read().split("\n")]
    part_1(numbers)
    part_2(numbers)


if __name__ == "__main__":
    main()
