rd /s ..\%1
mkdir ..\%1\pics

copy release\*.exe ..\%1
copy "%COMMONCPP%\bin\ccxx32.dll" ..\%1
copy *.html ..\%1
copy *.css ..\%1
copy docs\* ..\%1
copy docs\pics\*.jpg ..\%1\pics
copy docs\pics\*.gif ..\%1\pics
copy perl\*.pl ..\%1
copy *.wad ..\%1

pushd ..\%1
zip -9 -R %1.zip *
popd

copy release\*.map ..\%1
