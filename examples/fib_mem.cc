i32 a mem
i32 b mem
i32 c mem
i32 i mem

1 b w_mem
1 i w_mem

0 sout 10 putc
1 sout 10 putc

i32 i deref loop . 20 > do

	i32 a deref
	i32 b deref
	+
	. sout 10 putc
	c w_mem

	i32 b deref
	a w_mem
	i32 c deref
	b w_mem
	

	1 +
	i w_mem
	i32 i deref
end ,
