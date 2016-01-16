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
./build$ cat ../examples/simple3.cco
fun expr(x : int, y : int) : int{
    return (10 + 40 * 50)/10 + x + 4 + 100 * (1 <= y && 4 >= 0);
}
./build$ <b>./CCoscope < ../examples/simple3.cco</b>
KEYWORD_FUN(  ) IDENTIFIER( expr ) LPAR(  ) IDENTIFIER( x ) COLON(  ) TYPE( int ) COMMA(  ) IDENTIFIER( y ) COLON(  ) TYPE( int ) RPAR(  ) COLON(  ) TYPE( int ) LBRACKET(  ) KEYWORD_RETURN(  ) LPAR(  ) LITERAL_INT( 10 ) ADD(  ) LITERAL_INT( 40 ) MULT(  ) LITERAL_INT( 50 ) RPAR(  ) DIV(  ) LITERAL_INT( 10 ) ADD(  ) IDENTIFIER( x ) ADD(  ) LITERAL_INT( 4 ) ADD(  ) LITERAL_INT( 100 ) MULT(  ) LPAR(  ) LITERAL_INT( 1 ) LESSEQ(  ) IDENTIFIER( y ) AND(  ) LITERAL_INT( 4 ) GREATEREQ(  ) LITERAL_INT( 0 ) RPAR(  ) SEMICOLON(  ) RBRACKET(  )
Parsing...
The parser was successful!
Found 0 prototypes and 1 function definitions.

<b>define i32 @expr(i32 %x, i32 %y) {
entry:
  %addtmp = add i32 201, %x
  %addtmp1 = add i32 %addtmp, 4
  %cmptmp = icmp ule i32 1, %y
  %multmp = mul i32 100, i1 %cmptmp
  %addtmp2 = add i32 %addtmp1, %multmp
  ret i32 %addtmp2
}</b>


</pre>