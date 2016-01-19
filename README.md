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
./build$ cat ../examples/gcd3.cco
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

fun main() : int{
    print(gcd(160,200));
    print(gcd(160,250));
    print(gcd(100,295));
}
./build$ <b>./CCoscope < ../examples/gcd3.cco</b>
KEYWORD_FUN(  ) IDENTIFIER( gcd ) LPAR(  ) IDENTIFIER( a ) COLON(  ) TYPE( int ) COMMA(  ) IDENTIFIER( b ) COLON(  ) TYPE( int ) RPAR(  ) COLON(  ) TYPE( int ) LBRACKET(  ) KEYWORD_VAR(  ) IDENTIFIER( c ) COLON(  ) TYPE( int ) SEMICOLON(  ) IDENTIFIER( c ) ASSIGN(  ) LITERAL_INT( 0 ) SEMICOLON(  ) KEYWORD_WHILE(  ) LPAR(  ) IDENTIFIER( a ) NEQUAL(  ) LITERAL_INT( 0 ) RPAR(  ) LBRACKET(  ) IDENTIFIER( c ) ASSIGN(  ) IDENTIFIER( a ) SEMICOLON(  ) IDENTIFIER( a ) ASSIGN(  ) IDENTIFIER( b ) MOD(  ) IDENTIFIER( a ) SEMICOLON(  ) IDENTIFIER( b ) ASSIGN(  ) IDENTIFIER( c ) SEMICOLON(  ) RBRACKET(  ) KEYWORD_RETURN(  ) IDENTIFIER( b ) SEMICOLON(  ) RBRACKET(  ) KEYWORD_FUN(  ) IDENTIFIER( main ) LPAR(  ) RPAR(  ) COLON(  ) TYPE( int ) LBRACKET(  ) IDENTIFIER( print ) LPAR(  ) IDENTIFIER( gcd ) LPAR(  ) LITERAL_INT( 160 ) COMMA(  ) LITERAL_INT( 200 ) RPAR(  ) RPAR(  ) SEMICOLON(  ) IDENTIFIER( print ) LPAR(  ) IDENTIFIER( gcd ) LPAR(  ) LITERAL_INT( 160 ) COMMA(  ) LITERAL_INT( 250 ) RPAR(  ) RPAR(  ) SEMICOLON(  ) IDENTIFIER( print ) LPAR(  ) IDENTIFIER( gcd ) LPAR(  ) LITERAL_INT( 100 ) COMMA(  ) LITERAL_INT( 295 ) RPAR(  ) RPAR(  ) SEMICOLON(  ) RBRACKET(  )
Parsing...
The parser was successful!
Found 0 prototypes and 2 function definitions.

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
  ret i32 0
}


define i32 @main() {
entry:
  %calltmp = call i32 @gcd(i32 160, i32 200)
  %calltmp1 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @printf_number, i32 0, i32 0), i32 %calltmp)
  %calltmp2 = call i32 @gcd(i32 160, i32 250)
  %calltmp3 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @printf_number1, i32 0, i32 0), i32 %calltmp2)
  %calltmp4 = call i32 @gcd(i32 100, i32 295)
  %calltmp5 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @printf_number2, i32 0, i32 0), i32 %calltmp4)
  ret i32 0
}

Found llc at /usr/bin/llc-3.6
Executing: '/usr/bin/llc-3.6 out.ll -o out.s'...
Found clang at /usr/bin/clang-3.6
Executing: '/usr/bin/clang-3.6 out.s -o out.a'...
$<b> ./out.a
40
10
5
</b>
</pre>