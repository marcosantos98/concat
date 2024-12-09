"./std.cc" include

i64 a mem
i64 b mem
i64 c mem
i64 i mem

1 b w64_mem
1 i w64_mem

0 sout 10 putc
1 sout 10 putc

i64 i deref loop . 20 > do

	i64 a deref
	i64 b deref
	+
	. sout 10 putc
	c w64_mem

	i64 b deref
	a w64_mem
	i64 c deref
	b w64_mem
	

	1 +
	i w64_mem
	i64 i deref 
end ,
