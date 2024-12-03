0 . sout 10 putc 
1 . sout 10 putc

1 loop . 20 > do
	1 <- // Save i
	: // dup n1 and n2
	+ // next f
	. sout 10 putc // print f
	1 <- // save f
	; , // n1 = n2
	1 -> // n2 = saved f
	1 -> // pop i
	1 + // i++ 
end ,,,
