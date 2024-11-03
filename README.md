# C-imple

*Modern C++ is amazing, if you know what to use.*

This is a toy language that aims to transpile itself into
safe yet performant C++ code that is then compiled with
a system compiler, like GCC. Currently, the transpilation
is a simple code transformation that has not been fully
finished.


## Syntax

You can declare functions on the globaal scope
with the `fn` keyword. The return type of these
is automatically inferred based on the returned
value type. Function declarations require some
comma-separated arguments and an explicit prefix
of those with their type. You may have polymorphic
functions (that is, the same function name defined
for different types). 
Each program's entry point is the `main` method. 
Here is an example:

```c++
// main.cm
fn add(double x, double y) {
    return x+y;
}
fn main() {
    print(add(1,2));
}
```

```bash
./cimply main.cm
3
```

C-imple also follows the C syntax for declaring and scoping
variables. However, it uses the `var` instead of the `auto`
keyword because there are some slight differences in behavior
for safe pointers and we are trying to avoid confusion.
By the way, do prefer usage of this keyword instead of 
explicitly stating type. Here is an example:

```c++
// main.cm
fn main() {
    var str = "Hello world!";
    print(str);
}
```

```bash
./cimply main.cm
Hello world!
```

The main data structure is a C-like struct.
This cannot have any private fields. Always
use `self.` to access struct members. Also notice
the fullstop instead of `->`; the latter is not allowed.
Here is an example of declaring a 2D point:

```c++
// main.cm (first part)
struct Point {
    double x;
    double y;
    Point(double x, double y) {
        self.x = x;
        self.y = y;
    }
}
```

When declaring methods that take structs as inputs,
use the struct name by itself. All method execution
is by reference, and therefore allows modification of 
public fields if needed. Futhermore, there is no
copying involved. Here is a follow-up to the above
example.

```c++
// main.cm (second part)
fn adder(Point a, Point b) {
    double x = a.x+b.x;
    double y = a.y+b.y;
    return Point(x,y);
}
fn main() {
    var a = Point(1, 2);
    var b = Point(1, 2);
    var c = add(a, b);
    print(c.x);
    print(c.y);
    return 0;
}
```

For typical structs, the stack unwinding of 
C++ is used so that everything is deallocated once
you leave the current scope. To create objects that 
live beyond their generating scope, you need to evoke
a memory handler. Memory handlers create a variation
of base types with the syntax `handler[type]` and the
same constructor arguments. This is often similar to 
pointers with various methods of handling them safety 
attached.

Currently, there is only the `shared` handler implemented.
THis makes use of shared pointer semantics under the hood.
That is, it allows the safe creation of pointers
and their automatic deletion once they are no longer bound
to anything. In this strategy, there may be circular references
(which would be memory leaks) and therefore usage of the 
`unbind varname;`syntax is necessary to break those loops.
This remains memory safe, with a clear exception being
caught if an unbound variable is being accessed. However,
deletion timing may occur at any point in the code.


The following snippet demonstrates usage of the 
`shared` handler. To begin with, shared struct instances
are explicitly declared.


```cpp
// main.cm
struct LinkedNode {
    double value;
    shared[LinkedNode] next;
    shared[LinkedNode] prev;
    LinkedNode(double value) {
        self.value = value;
    }
};

fn set_next(shared[LinkedNode] from, shared[LinkedNode] to) {
    from.next = to;
    to.prev = from;
}

fn main() {
    var node1 = shared[LinkedNode](1);
    var node2 = shared[LinkedNode](2);
    var node3 = shared[LinkedNode](3);

    set_next(node1, node2);
    set_next(node2, node3);
    try {
        print(node1.value);
        print(node1.next.value);
        print(node1.next.next.value);
        print(node1.next.next.next.value);
    }
    catch(std@runtime_error) {
        print("runtime error");
    }

    return 0;
}
```

```bash
./cimply main.cm
File processed and saved as: main.cpp
1
2
3
runtime error
Execution finished
```