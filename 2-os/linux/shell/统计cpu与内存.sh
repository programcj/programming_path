############################
### 每隔5秒统计cpu使用情况 ###
#!/bin/bash
PROGRAM="test.exe"
CURTM=$(date "+%Y-%m-%d %H:%M:%S")
echo $CURTM
echo $CURTM > test.log
echo "CPU MemUse" >>test.log
while true; do 
	cpu=$(ps aux | grep $PROGRAM | head -n 1 | awk '{print $3}')
	memuse=$(free -k| grep Mem | awk '{print $3}')
	
	echo "$cpu $memuse"
	echo $cpu $memuse >> test.log
	sleep 5
done
