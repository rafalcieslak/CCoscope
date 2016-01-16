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
fun expr() : int{
    return (10 + 40 * 50)/10 + 110 + 4 + 100 * (1 <= 5 && 4 >= 0);
}
./build$ <b>./CCoscope < ../examples/simple3.cco</b>
KEYWORD_FUN(  ) IDENTIFIER( expr ) LPAR(  ) RPAR(  ) COLON(  ) TYPE( int ) LBRACKET(  ) KEYWORD_RETURN(  ) LPAR(  ) LITERAL_INT( 10 ) ADD(  ) LITERAL_INT( 40 ) MULT(  ) LITERAL_INT( 50 ) RPAR(  ) DIV(  ) LITERAL_INT( 10 ) ADD(  ) LITERAL_INT( 110 ) ADD(  ) LITERAL_INT( 4 ) ADD(  ) LITERAL_INT( 100 ) MULT(  ) LPAR(  ) LITERAL_INT( 1 ) LESSEQ(  ) LITERAL_INT( 5 ) AND(  ) LITERAL_INT( 4 ) GREATEREQ(  ) LITERAL_INT( 0 ) RPAR(  ) SEMICOLON(  ) RBRACKET(  )
Parsing...
The parser was successful!
Found 0 prototypes and 1 function definitions.

<b>define i32 @expr() {
entry:
  ret i32 415
}</b>

</pre>