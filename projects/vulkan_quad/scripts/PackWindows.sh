#Used to easily package the binaries and their libraries
echo "Packing everything needed to distribute on Windows . . ."
rm -r Exports/Windows/*
rm Exports/Windows.zip

cp -r data Exports/Windows/data

cp bin/Windows/Release/$1.exe Exports/Windows/Release.exe
cp bin/Windows/Debug/$1.exe Exports/Windows/Debug.exe

cp lib/libgcc_s_sjlj-1.dll Exports/Windows

cd Exports
echo "Compressing into a zip archive . . ."
zip -r Windows.zip Windows
echo "Done!"
