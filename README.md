# CCoscope
Compiler Construction, mixed with a custom language and LLVM Kaleidoscope, brewed together and still not working.

### Building

```
mkdir build && cd build
cmake ..
make
```

Note: On debian-like systems, compilation may fail due to missing `-ledit`. This should be detected and reported by cmake's LLVM detection package, but for some reason it it's not, or maybe the debian package for `llvm-dev` is missing a dependency. Either way installing `libedit-dev` solves the issue.

### Example

<pre>
.$ cd build/
./build$ <b>cat ../examples/gcd.cco </b>
fun gcd (a : int, b : int) : int {
    var c : int;
    c := 0;
    while ( a != 0 ) {
        c := a;
        a := b % a;
        b := c;
    }
    return b;
}
./build$ <b>cat ../examples/gcd_main.cco </b>
extern fun gcd(a : int, b : int) : int;

fun main() : int{
    print(gcd(160,200));
    print(gcd(160,250));
    print(gcd(100,295));
}./build$ <b>./CCoscope ../examples/gcd_main.cco ../examples/gcd.cco -o gcd </b>
Parsing ../examples/gcd_main.cco
Found 1 prototypes and 1 function definitions.
Writing out IR for module ../examples/gcd_main.cco to /tmp/fileEQzyoS.ll
Executing: '/usr/bin/llc-3.6 /tmp/fileEQzyoS.ll -o /tmp/fileEH2ljC.s'...
Executing: '/usr/bin/clang-3.6 -c /tmp/fileEH2ljC.s -o /tmp/fileQwySfm.o'...
Parsing ../examples/gcd.cco
Found 0 prototypes and 1 function definitions.
Writing out IR for module ../examples/gcd.cco to /tmp/fileJN5jg6.ll
Executing: '/usr/bin/llc-3.6 /tmp/fileJN5jg6.ll -o /tmp/filepZ2UgQ.s'...
Executing: '/usr/bin/clang-3.6 -c /tmp/filepZ2UgQ.s -o /tmp/filelJfWiA.o'...
Executing: '/usr/bin/clang-3.6 /tmp/fileQwySfm.o /tmp/filelJfWiA.o -o gcd'...
./build$ <b>./gcd </b>
40
10
5
./build$
</pre>