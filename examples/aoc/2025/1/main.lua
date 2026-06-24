local function mod(x, y)
    return (x % y + y) % y
end

local function part_1(numbers)
    local dial = 50
    local count = 0
    for _, num in ipairs(numbers) do
        dial = mod(dial + num, 100)
        if dial == 0 then count = count + 1 end
    end
    print("part_1: " .. count)
end

local function part_2(numbers)
    local dial = 50
    local count = 0
    for _, num in ipairs(numbers) do
        if dial > 0 and num < 0 and dial + num < 0 then
            count = count + 1
        end
        local adj = math.abs(dial + num) - 1
        count = count + (adj >= 0 and adj // 100 or 0)
        dial = mod(dial + num, 100)
        if dial == 0 then count = count + 1 end
    end
    print("part_2: " .. count)
end

local function parse(line)
    if line == "" then return 0 end
    local val = tonumber(line:sub(2))
    if line:sub(1, 1) == "L" then return -val else return val end
end

local function main()
    local f = io.open("input.txt", "r")
    local content = f:read("*a")
    f:close()
    local numbers = {}
    for line in content:gmatch("[^\n]*") do
        table.insert(numbers, parse(line))
    end
    part_1(numbers)
    part_2(numbers)
end

main()
