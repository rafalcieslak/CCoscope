# CCoscope
Compiler Construction, mixed with a custom language and LLVM Kaleidoscope, brewed together and still not working.

### Building

```
mkdir build && cd build
cmake ..
make
```

Note: On debian-like systems, compilation may fail due to missing `-ledit`. This should be detected and reported by cmake's LLVM detection package, but for some reason it it's not, or maybe the debian package for `llvm-dev` is missing a dependency. Either way installing `libedit-dev` solves the issue.

### Running

```
./CCoscope
```

### Example

<pre>
.$ cd build/
./build$ cat ../examples/gcd.cco
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
./build$ <b>./CCoscope < ../examples/gcd.cco</b>
KEYWORD_FUN(  ) IDENTIFIER( gcd ) LPAR(  ) IDENTIFIER( a ) COLON(  ) TYPE( int ) COMMA(  ) IDENTIFIER( b ) COLON(  ) TYPE( int ) RPAR(  ) COLON(  ) TYPE( int ) LBRACKET(  ) KEYWORD_VAR(  ) IDENTIFIER( c ) COLON(  ) TYPE( int ) SEMICOLON(  ) IDENTIFIER( c ) ASSIGN(  ) LITERAL_INT( 0 ) SEMICOLON(  ) KEYWORD_WHILE(  ) LPAR(  ) IDENTIFIER( a ) NEQUAL(  ) LITERAL_INT( 0 ) RPAR(  ) LBRACKET(  ) IDENTIFIER( c ) ASSIGN(  ) IDENTIFIER( a ) SEMICOLON(  ) IDENTIFIER( a ) ASSIGN(  ) IDENTIFIER( b ) MOD(  ) IDENTIFIER( a ) SEMICOLON(  ) IDENTIFIER( b ) ASSIGN(  ) IDENTIFIER( c ) SEMICOLON(  ) RBRACKET(  ) KEYWORD_RETURN(  ) IDENTIFIER( b ) SEMICOLON(  ) RBRACKET(  )
Parsing...
The parser was successful!
Found 0 prototypes and 1 function definitions.

<b>
define i32 @gcd(i32 %a, i32 %b) {
entry:
  br label %header

header:                                           ; preds = %body, %entry
  %a_addr.0 = phi i32 [ %a, %entry ], [ %modtmp, %body ]
  %b_addr.0 = phi i32 [ %b, %entry ], [ %a_addr.0, %body ]
  %cmptmp = icmp eq i32 %a_addr.0, 0
  br i1 %cmptmp, label %postwhile, label %body

body:                                             ; preds = %header
  %modtmp = srem i32 %b_addr.0, %a_addr.0
  br label %header

postwhile:                                        ; preds = %header
  ret i32 %b_addr.0
}
</b>
</pre>