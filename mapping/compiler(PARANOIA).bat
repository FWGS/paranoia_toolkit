@echo off 
set MAPSPATH=D:\Mapping\maps
set MAPNAME=1 
set CSPATH=D:\Games\Half-Life\paranoia\maps
 
D:\Mapping\Zhlt\hlcsg.exe -texdata 8192 D:\Mapping\maps\1.map 
D:\Mapping\Zhlt\hlbsp.exe -texdata 8192 -estimate D:\Mapping\maps\1.map
D:\Mapping\Zhlt\hlvis.exe -texdata 8192 -full -estimate D:\Mapping\maps\1.map
D:\Mapping\Zhlt\hlrad.exe -texdata 8192 -extra -estimate D:\Mapping\maps\1.map  
D:\Mapping\Zhlt\bumprad.exe -texdata 8192 -estimate D:\Mapping\maps\1.map  

echo COMPILE END
