0 O_RONLY def

0 SEEK_SET def
2 SEEK_END def

8 i64 def

i64 fd mem 
i64 size mem
i64 buf mem

"Opening file examples!" println

O_RONLY "./test.txt" open . 0 ; < if
	"Failed to open file!" println
	1 exit
else
	fd w64_mem
endif

SEEK_END 0 i64 fd deref lseek size w64_mem 
SEEK_SET 0 i64 fd deref lseek ,

i64 size deref malloc buf w64_mem

i64 size deref i64 buf deref i64 fd deref read

i64 size deref i64 buf deref as_str print

i64 fd deref close

i64 buf deref free
