fn mod_op(x: i64, y: i64) -> i64 {
    ((x % y) + y) % y
}

fn parse(line: &str) -> i64 {
    if line.is_empty() {
        return 0;
    }
    let val: i64 = line[1..].parse().unwrap();
    if line.as_bytes()[0] == b'L' { -val } else { val }
}

fn part_1(nums: &[i64]) {
    let mut dial = 50i64;
    let mut count = 0i64;
    for &x in nums {
        dial = mod_op(dial + x, 100);
        if dial == 0 { count += 1; }
    }
    println!("part_1: {count}");
}

fn part_2(nums: &[i64]) {
    let mut dial = 50i64;
    let mut count = 0i64;
    for &x in nums {
        if dial > 0 && x < 0 && dial + x < 0 { count += 1; }
        let adj = (dial + x).abs() - 1;
        count += adj / 100;
        dial = mod_op(dial + x, 100);
        if dial == 0 { count += 1; }
    }
    println!("part_2: {count}");
}

fn main() {
    let content = std::fs::read_to_string("input.txt").unwrap();
    let numbers: Vec<i64> = content.split('\n').map(parse).collect();
    part_1(&numbers);
    part_2(&numbers);
}
