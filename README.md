# C-imple

*A memory safe C++ transpiler.*

This is a simple language (pun intended) 
that aims to transpile itself into
safe yet performant C++ code that is then compiled with
a system compiler, like GCC. Currently, the transpilation
is a code transformation.

**Main features are still under development.**

## Quickstart

Cimple is compiled like below. Otherwise, grab an executable from this repository.
GCC with support for C++23 is needed to run. It is recommended to use a Rust highlighter.

```bash
g++ src/cimple.cpp -o cimple -O2 -std=c++23
```

Here is a first program that is memory safe runs as fastly as C++ can go.

```rust
// main.cm
func add(vector[double] x, vector[double] y) {
  var z = vector[double]();
  z.reserve(x.size());
  for(var [xvalue, yvalue] in zip(x, y)) // zip runs to the lower list size
    z.push(xvalue+yvalue);
  return z;
}
func main() {
  var x = vector[double]({1,2,3,4, 5});
  var y = vector[double]({1,2,3,4});
  var z = add(x,y);
  print(z[2]);
  return 0;
}
```

Exexute and run it like in the next snippet. Notice the various preparation
steps as your code is transpiled in C++ and compiled with GCC.

```bash
./cimple main.cm
--------------- Cimple v0.1 ----------------
  Building: main.cm
  Compiling: main.cpp
  Running: ./main
--------------------------------------------
6
```


## Material 

[User guide](https://maniospas.github.io/c-imple/)