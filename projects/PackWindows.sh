#Used to easily package the binaries and their libraries
echo "Packing everything needed to distribute on Windows . . ."
if [ -e Exports ]
then
    rm -r Exports/Windows*
fi

mkdir -p Exports/Windows

if [ -e data ]
then
    cp -r data Exports/Windows/data
fi

cp bin/Windows/Release/$1.exe Exports/Windows/Release.exe
cp bin/Windows/Debug/$1.exe Exports/Windows/Debug.exe

cp ../../base/lib/libgcc_s_sjlj-1.dll Exports/Windows

if [ -e lib ]
then
    for filename in lib/*.dll; do
        cp "$filename" Exports/Windows
    done
fi

cd Exports
echo "Compressing into a zip archive . . ."
zip -r Windows.zip Windows
echo "Done!"
