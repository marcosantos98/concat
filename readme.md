# concat

### How to:

1. Compile concat using `make`
2. Use `./main <source>` to use concat in interpet mode
3. [Optional] Give `gen` arg to generate qbe IR and clang to compile to native binary. `./main <source> gen`
    - This is very WIP.

### Operators:

- t: value on the top of the stack
- ut: value under the top
- sp: stack pointer
- bst: value on the top of the back stack

| Syntax | Job | Stack manipulation | Consume n |
| --- | --- | --- | --- |
| //... | comment | - | 0 |
| 0-9 | push number stack | t = value | 0 |
| "..." | push string to storage and push idx to stack | t = strIdx | 0 |
| . | dup | t = ut | 0 |
| : | dup two values | ut = t - 1, t = t | 0 |
| , | drop | sp-- | 1 |
| ; | swap | t = ut; ut = t | 0 |
| <- | stash n elements in back stack | for n in stack: bst = t;| n |
| -> | pop n elements from back stack | for n in backStack: t = bst | n |
| + | plus | t = t + ut | 2 |
| - | minus | t = t - ut | 2 |
| * | mult | t = t * ut | 2 |
| / | plus | t = t / ut | 2 |
| % | mod | t = t % ut | 2 |
| < | less than | t = t < ut | 2 |
| > | greater than | t = t > ut | 2 |
| = | equality | t = t == ut | 2 |
| sout | print | out(t); sp-- | 1 |
| do | start loop | t != 0 | 0 |
| loop | check loop condition | t == 1 | 1 | 
| end | end body | - | 0 |
| putc | print char | out(t); sp-- | 1 |
| print | print string | out(t); sp-- | 1 |
| println | print string with new line | out("<t>\n"); sp-- | 1 |
| if | if start | t != 0 | 1 |
| else | execute body if not true | - | 0 |
| endif | end if body | - | 0 |

### Memory:

- mp: memory pointer

| Syntax | Job | Memory manipulation | Consume n |
| --- | --- | --- | --- |
| mem | Assign defined word to pointer in memory with size | mp += size | 1 |
| w_mem | Write value into word previously defined | mem @ word = value | 2 |
| w64_mem | Write 64bit value into world previously defined | mem @ word = value | 3 |
| deref | Dereference word to stack with given size | t = * word as size | i32 ? 2 : 3 |
| as_str | Takes ptr and casts as string with size | t = strIdx | 2 |

### Current LIBC bindings

| name | Return | Consume |
| --- | --- | --- |
| open | fd | 3 (path, mode, open keyword) |
| lseek | size | 4 (SEEK_X, off, fd, lseed keyword) |
| read | none (TODO) | 4 (fd, ptr, size, read keyword) |
| malloc | ptr as two i32 | 2 (size, read keyword) |
| free | none | 2 (ptr, read keyword) |
| close | none | 2 (fd, close keyword) |
| exit | none | 2 (code, exit keyword) |

