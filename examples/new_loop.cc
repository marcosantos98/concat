0 loop . 3 > do
	. sout 10 putc
	"Begin inner loop" println
	0 loop . 2 > do
		61 putc 32 putc
		. sout 10 putc
		1 +
	end ,
	"End inner loop" println
	1 +
end ,
