# concat

### Operators:

- t: value on the top of the stack
- ut: value under the top
- sp: stack pointer

| Syntax | Job | Stack manipulation |
| --- | --- | --- |
| . | dup | t = ut |
| , | drop | sp-- |
| + | plus | t = t + ut |
| - | minus | t = t - ut |
| * | mult | t = t * ut |
| / | plus | t = t / ut |
| % | mod | t = t % ut |
| sout | print | out(t); sp-- |