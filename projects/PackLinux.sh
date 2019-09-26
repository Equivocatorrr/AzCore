#Used to easily package the binaries and their libraries
echo "Packing everything needed to distribute on Linux . . ."
rm -r Exports/Linux*

mkdir -p Exports/Linux

cp -r data Exports/Linux/data

cp bin/Linux/Release/$1 Exports/Linux/Release
cp bin/Linux/Debug/$1 Exports/Linux/Debug

cd Exports
echo "Compressing into a tar ball . . ."
tar -czvf Linux.tar.gz Linux
echo "Done!"
