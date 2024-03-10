# concat

### Operators:

- t: value on the top of the stack
- ut: value under the top
- sp: stack pointer

| Syntax | Job | Stack manipulation | Consume n |
| --- | --- | --- | --- |
| //... | comment | - | 0 |
| 0-9 | push number stack | t = value | 0 |
| "..." | push string to storage and push idx to stack | t = strIdx | 0 |
| . | dup | t = ut | 0 |
| , | drop | sp-- | 1 |
| ; | swap | t = ut; ut = t | 0 |
| + | plus | t = t + ut | 2 |
| - | minus | t = t - ut | 2 |
| * | mult | t = t * ut | 2 |
| / | plus | t = t / ut | 2 |
| % | mod | t = t % ut | 2 |
| < | less than | t = t < ut | 2 |
| > | greater than | t = t > ut | 2 |
| sout | print | out(t); sp-- | 1 |
| do | start loop | t != 0 | 0 |
| loop | check loop condition | t == 1 | 1 | 
| end | end body | - | 0 |
| putc | print char | out(t); sp-- | 1 |
| print | print string | out(t); sp-- | 1 |
| println | print string with new line | out("<t>\n"); sp-- | 1 |
