#　　$^    代表所有的依赖文件
#　　$@  代表所有的目标文件 
#　　$<   代表第一个依赖文件 

time_test_exe: time_test.c
	$(CC) $< -o time_test.exe

openlog_test_exe: openlog_test.c
	$(CC) $< -o openlog_test.exe

open_test.exe: open_test.c
	$(CC) $< -o open_test.exe

printf_test.exe: printf_test.c
	$(CC) $< -o printf_test.exe

sscanf_test.exe: sscanf_test.c
	$(CC) sscanf_test.c -o sscanf_test.exe

getcwd_test.exe: getcwd_test.c
	$(CC) getcwd_test.c -o getcwd_test.exe
	
OUT_FILES = sscanf_test.exe
OUT_FILES += printf_test.exe
OUT_FILES += open_test.exe
OUT_FILES += openlog_test_exe
OUT_FILES += time_test_exe
OUT_FILES += getcwd_test.exe

all: $(OUT_FILES)
		
clean:
	rm *.exe