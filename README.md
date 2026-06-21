# clape

Clape is a very small functional language built around *composable shapes*.

Clape is learnable in under an hour. You can execute the interpreter binary directly or embed it in any project, Clape is a STL-style single-header C library. If you want to try Clape out, the easiest approach is to clone the project, but just copying the `main.c` and `clape.h` files is plenty too. You just need to compile the `main.c` file and tada, you now have a working Clape interpreter.

# Documentation

## Keywords

Clape only has 8 keywords:
- `use`
- `let`
- `if`
- `else`
- `match`
- `and`
- `or`
- `not`

The most important keyword, and the one you will use most, is the `let` keyword. Clape is based on SSA (Static Single Assignment). This means that everyrhing is immutable, you cannot mutate values directly.

Let's write the simplest Clape program, hello world:

```ocaml
use Print

let _ = print "Hello, World!"
```

You can run this program using `clape main.clape` and it will output this line:

> ```
> Hello, World!
> ```

The `use Print` line tells Clape include the builtin `print` function. Function calls use the juxtaposition notation, this means that the function arguments directly follow the called function name, separated by spaces.

The `let _ =` part means that we discard the return type of the rhs expression and do not store it on a "variable". IO functions like `print` return the `Unit` type. Any function returning this type is considered to have side effects. A side-effect is any IO operation, like printing to the console.

## Types

Clape has exactly 10 types:
- `Int`: An integer value (always i64)
- `Float`: A floating point value (double precision)
- `Bool`: true or false
- `Char`: A single character
- `String`: A string
- `Unit`: ("void") For marking side effects
- Functions (`(T -> U)`)
- Lists (`[T]`)
- Product Types
- Sum Types

Even if you define types yourself, every Clape type will be one of the above types. You already saw a `String` value in the example above, and the `print` function above also returns a `Unit`. Here is a small program using Integer and Float types respectively:

```ocaml
use Print

let x = 10
let y = 20
let z = x + y
let _ = print z
```

This program will simply print `30` to the console. As you now see a bit more clear is that `let` is intended to **name values**. Another small note: Every expression which can be evaluated, will be evaluated when assigning it to a "variable".
It is impossible to modify the value of `x` after it has been assigned, its value is immutable.

## Functions

Functions, like any other value in Clape, are expressions. You can assign functions to a "variable" just like you can assign other values. Parameter types and the return type of functions need to be explicitly typed in Clape:

```ocaml
use Print

let add = (x: Int, y: Int) -> Int {
    x + y
}

let _ = print {add 10 20}

let add5 = add 5
let _ = print {add5 3}
```

This program prints these lines to the console:

> ```
> 30
> 8
> ```

You define a function by notating it's parameter names and types, followed by an arrow and the return type. After the return type, a **Block** must follow. A block, as you will see soon, is an optional List of "statements" (an SSA assignment is technically a "statement") followed by a single expression. The last expression in a block is the value the entire block expression evaluates to.

So, in the easy example above `x + y` is the last expression in the block, so when the block is evaluated, this value is retuend.

We need to add another block to the `print` function call because if we would write `print add 10 20` the `add` function would be a *parameter* of the print function. That's not what we want.

You can also see a small example of partial application here too. Here, we applied the value `5` to the `add` function, but it still is missing a value, so the function is not called yet. Partially applied functions count as functions. When we then later call `add5 3` the function is properly evaluated.

## If expression

Clape has only one form in which ifs are used, as an expression, e.g. a ternary. The structure is like this:
```
<TRUE_EXPR> if <CONDITION> else <FALSE_EXPR>
```

for example:

```ocaml
use Print

let result = 3.14 if 1 > 2 else 6.9
let _ = print result
```

Which prints `3.14` to the console. You can optionally enclose any of the expressions in a block if you want to, to make it visually clearer for example:
```ocaml
use Print

let result = {
        3.14
    } if {
        1 > 2
    } else {
        6.9
    }
let _ = print result
```
Clape is parsable entirely line-independent, so xou could write the whole program into a single line or every token into its own line, that's up to you to decide.

With conditional branching, we now can implement a fibonnacci function:
```ocaml
use Print

let fib = (n: Int) -> Int {
    n if n < 2 else {
        fib {n - 1} + fib {n - 2}
    }
}
let _ = print {fib 10}
```
This program will print the value`55` to the console.

## Lists

Very central to basically any functional programming language are Lists, and Clape is no different. Let's start with the easiest example:

```ocaml
use Print

let list = [1, 2, 3]
let _ = print list

let new_list = 0 :: list
let _ = print new_list
```
This program prints these lines to the console:
> ```
> [1, 2, 3]
> [0, 1, 2, 3]
> ```

The `::` operator is called the `cons` operator and it's an industry standard for almost every functional programming language. It is used to insert a value at the very front of a list. In Clape you can only insert a value of a matching type into a list.

Lets continue on now with a bit more complex function, append, which appends a value to a list:

```ocaml
use Print

let append = (list: [Int], value: Int) -> [Int] {
    match list {
        [] => [value];
        head :: tail => head :: append tail value
    }
}

let list = [1, 2, 3]
let new = append list 4
let _ = print new
```
This code prints `[1, 2, 3, 4]` to the console. There are quite a few new parts to look at here. The `match` expression is used for pattern-matching on a given value, in our case a list of integers.
For the case of an empty list, e.g. when we reached the end of the list, we return a list with one value, the value to append. So our new list now is `[4]`. Since the last element of the list contained a value, we matched the `head :: tail` branch, where `head` is the value `3` and `tail` is an empty list (since nothing follows after the 3).
We then apply the cons operator on the value `3` (the head) and the newly created list, `[4]` and thus put the 3 in front, so the list now becomes `[3, 4]`. The same repeats for 2 and 1 and now er have appended a value to a list.
If you ever saw any functional language, the append function will likely have looked almost certainly very similar to this one above.

## Products

Now we finally come to the meat of Clape, products and sums. A product type is, at it's core, a collection of fields. It's like a `struct` in C, but it's not nominally typed. This means that we don't "create a new type" but rather describe the **shape** of that type. It may sound weird at first, but bear with me, it's a pretty cool concept actually.

```ocaml
use Print

let Point = x(Float) & y(Float)

let add = (p1: Point, p2: Point) -> Point {
    .x(p1.x + p2.x) & .y(p1.y + p2.y)
}

let p1 = .x(3.4) & .y(2.2)
let p2 = p1 & .y(3.5)

let _ = print {add p1 p2}
```

This program prints

> ```
> .x(6.8) & .y(5.7)
> ```

to the console. It's quite a dense example with lots of new concepts, but it will all make sense soon. The value `x(Float)` describes a **Product Type**. A product is a collection of fields, and a single field already is a collection, it just contains one element.
The `&` operator is *exclusively* used to combine **Product Types** and **Product Values**. So, when we write `x(Float) & y(Float)` we describe a new product type which contains the fields `x` and `y`, both of which are of type `Float`.
We then "store" that product type in the SSA value `Point`. But `Point` is *not* a "new type" like it would be in C if we would define a struct type, for example. `Point` is simply a *named description of the shape*. More on that shortly.

The `.x(expr)` expression describes a **Product Value**. A product value is, in it's essence, again just a simple field with a type and a value attached to it. We need the leading dot to differentiate between types and values, this differentiation makes it visually more clear whether we are dealing with types or values, and it makes the code easy to grep for.
We can combine multiple product values, e.g. collections of field values, into larger product values using the `&` operator again.

But what's the `p1 & .y(3.5)` doing? This is where this whole concept gets interesting. The value `p1` is *not* "of type" `Point`. The value `p1` is of type "Product" and contains a few fields and values. As a result, we can combine the value with any field we want. We also could write `p1 & .wololo("Wahoo")` and the result would be a product value with 3 fields; `x`, `y` and `wololo`, even though we *never* defined that type anywhere.
When combining product values, two simple rules apply:
1. Product value & type combination happens from left to right
2. Matching fields are overwritten

So, when we write `p1 & .y(3.5)` we take the product value `p1` and apply the product value `.y(3.5)` to it. `p1` already contains a value `y` with the same type as the "new" value, which means the value `y` is being "overwritten". But everything is immutable, so we don't modify `p1` directly but create a new value, `.x(3.4) & .y(3.5)` instead!

The same application logic of fields also applies to "types" too:

```ocaml
use Print

let Point2D = x(Float) & y(Float)
let Point3D = Point2D & z(Float)

let add = (p1: Point2D, p2: Point2D) -> Point2D {
    .x(p1.x + p2.x) & .y(p1.y + p2.y)
}

let get_x = (p: x(Float)) -> Float {
    p.x
}

let p1 = .x(2.3) & .y(3.4)
let p2 = p1 & .z(4.4)
let p3 = p2 & {add p1 p2}
let _ = print p3

let _ = print {get_x p1}
let _ = print {get_x p2}
let _ = print {get_x p3}
```

This program prints these lines to the console:

> ```
> .x(4.6) & .y(6.8) & .z(4.4)
> 2.3
> 2.3
> 4.6
> ```

Here, we essentially describe that `Point3D` is the same *shape* as `Point2D`, but it adds one field, `z`, of type `Float` to it.

We then construct `p1`, a product value with *two* fields, and `p2`, a product value with *three* fields by combining `p1` with a new field, `z`. And now comes the really special part about this all. A function describes ehich shape a given parameter must *at least* have. This means that we can essentially pass *any* product value to a function as long as that value contains *at least* the required fields with the required types.
Combining this with immutable updates gives us the new `p3` value. The line
```ocaml
let p3 = p2 & {add p1 p2}
```
essentially says "take the old value of p2 and combine it with the addition of all it's fields compatible with p1". So we take `x` and `y` from `p2` and add them with `x` and `y` of `p1`. This operation returns a new product value with fields `x` and `y`. We then take the 3d point `p2` and update it's `x` and `y` fields with the result of the addition, the `z` stays untouched. We store the result of this whole operation on a new SSA value, `p3`.

When we combine the type combination capabilities with the "field overwrite" capabilities we get a *very* powerful system! Tinker around a bit, this takes some time to get used to, but when you do get used to it you will realize it's potential!

One **very** important note here: product types and values **must** start with a lowercase character and are **not allowed** to start with either an uppercase or underscore character. If you write them with an uppercase character you write a *Sum* value / type instead!

## Sums

Which brings us to Sum types. While product types describe a collection of values, a sum type describes the possibility of one of many types. They are the counter-part to variants / unions of other languages like C. Let's start with a very simple example first:

```ocaml
use Print

let OptInt = Some(Int) | None

let get_first = (list: [Int]) -> OptInt {
    match list {
        [] => .None;
        head :: tail => .Some(head)
    }
}

let empty = []
let list = [1, 2, 3]

let _ = print {get_first empty)
let _ = print {get_first list}
```

This program prints these lines to the console:

> ```
> .None
> .Some(1)
> ```

As you can see, we use the sum combination operator `|` to combine single sum types to larger sum types. The exact same rules regarding shape composition apply as for product types. So, writing `OptInt | Maybe(Float)` describes a sum shape with three possibilities.

While product type compatibility is determined by a "superset" relationship between the product value and the function parameter (the value must at least contain the fields the function requires), sum type compatibility is determined by a "in-set" relationship. The function parameter describes which possibilities are allowed, and the sum value passed to the function must be *one of* the required variations.

Unlike product values, sum values can *not* be combined. So, writing `.Some(Int) | .None` is not allowed. If you think about it for a second it also makes sense, because how can something that's *either* the one *or* the other be both at the same time?

We can check which variation a value holds by applying pattern matching on it:

```ocaml
use Print

let OptInt = Some(Int) | None

let get_value = (value: OptInt) -> Int {
    match value {
        .Some(v) => v;
        .None => 0
    }
}

let _ = print {get_value None}
let _ = print {get_value Some(69)}
```

This program will print these lines to the console:

> ```
> 0
> 69
> ```

While sum types are useful as this too, their real potential is only unlocked through generics.

## Generics

This is the last and final concept of Clape. Generic types can be attached to either *function definitions* or *type definitions*. So, when the RHS of a let binding is either a function or a "type", a shape. I think the simplest example would be to start with generic sum types:

```ocaml
use Print

let Option<T> = Some(T) | None

let get_first<T> (list: [T]) -> Option<T> {
    match list {
        [] => .None;
        head :: tail => .Some(head)
    }
}

let _ = print {get_first []}
let _ = print {get_first [1, 2]}
let _ = print {get_first ["hi", "there"]}
```

This program prints these lines to the console:

> ```
> .None
> .Some(1)
> .Some("hi")
> ```

As you can see, we add a generic type to a shape or to a function using the `<T>` syntax. `T` is just the *name* of the type, a placeholder. Generic shapes need to be "resolved" by passing an actual or other generic type into them using the same `<T>` syntax. As you can see, we do *not* need to resolve the type when calling a function. The type is inferred from the context. For example we are only allowed to pass lists to the `get_first` function, but which elements the list contains is generic / unknown and none of the functions business.

Here is another example for some error handling:

```ocaml
use Print

let Result<T, E> = Ok(T) | Err(E)

let div = (x: Int, y: Int) -> Result<Int, String> {
    {
        .Ok(x / y) 
    } if y != 0 else {
        .Err("Division by zero detected")
    }
}

let _ = print {div 10 2}
let _ = print {div 10 0}
```

This program will print these lines to the console:

> ```
> .Ok(5)
> .Err("Division by zero detected")
> ```

And here is an example using generic product types:

```ocaml
use Print

let Pair<T, U> = first(T) & second(U)

let pack<T, U> = (f: T, s: U) -> Pair<T, U> {
    .first(f) & .second(s)
}

let _ = print {pack 10 "Wohoo"}
let _ = print {pack 'A' 3.14}
```

This program will print these lines to the console:

> ```
> .first(10) & .second("Wohoo")
> .first('A') & .second(3.14)
> ```

And one final example for showcasing maps and partial generic type application:

```ocaml
use Print

let Map<K, V> = [key(K) & value(V)]
let StringMap<T> = Map<String, T>

let emplace<T> = (map: StringMap<T>, key: String, value: T) -> StringMap<T> {
    match map {
        [] => [.key(key) & .value(value)];
        head :: tail => {
            { .key(key) & .value(value) } :: tail
        } if head.key == key else {
            head :: {emplace tail key value}
        }
    }
}

let make_value<T> (key: String, value: T) -> key(String) & value(T) {
    .key(key) & .value(value)
}

let map = [make_value "x" 10]
let _ = print map

let added = emplace map "y" 20
let _ = print added

let overwritten = emplace added "x" 30
let _ = print overwritten
```

This program will print these lines to the console:

> ```
> [.key("x") & .value(10)]
> [.key("x") & .value(10), .key("y") & .value(20)]
> [.key("x") & .value(30), .key("y") & .value(20)]
> ```

## That's it

These were basically all features the Clape language has to offer. If you want to look at a few practical examples, look at the `examples` directory.
