i32 fd mem 
i32 size mem
i64 buf mem

"Opening file examples!" println

0 "./test.txt" open . 0 ; < if
	"Failed to open file!" println
	1 exit
else
	fd w_mem
endif

2 0 i32 fd deref lseek size w_mem 
0 0 i32 fd deref lseek ,

i32 size deref malloc buf w64_mem

i32 size deref i64 buf deref i32 fd deref read

i32 size deref i64 buf deref as_str print

i32 fd deref close

i64 buf deref free
