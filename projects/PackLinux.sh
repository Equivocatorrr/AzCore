#Used to easily package the binaries and their libraries
echo "Packing everything needed to distribute on Linux . . ."
if [ -e Exports ]
then
    rm -r Exports/Linux*
fi

mkdir -p Exports/Linux

if [ -e data ]
then
    cp -r data Exports/Linux/data
fi

cp bin/Linux/Release/$1 Exports/Linux/Release
cp bin/Linux/Debug/$1 Exports/Linux/Debug

if [ -e lib ]
then
    mkdir -p Exports/Linux/lib
    for filename in lib/*.so*; do
        cp "$filename" Exports/Linux/lib
    done
fi

cd Exports
echo "Compressing into a tar ball . . ."
tar -czvf Linux.tar.gz Linux
echo "Done!"
