#Used to easily package the binaries and their libraries
echo "Packing everything needed to distribute on Windows . . ."
EXPORT_DIR=Exports/$1_Win
if [ -e Exports ]
then
	rm -r $EXPORT_DIR*
fi

mkdir -p $EXPORT_DIR

if [ -e data ]
then
	cp -r data $EXPORT_DIR/data
fi

cp bin/Windows/Release/$1.exe $EXPORT_DIR/$1.exe
cp bin/Windows/Debug/$1.exe $EXPORT_DIR/$1_Debug.exe

# cp ../../base/lib/libgcc_s_sjlj-1.dll Exports/Windows

if [ -e lib ]
then
	for filename in lib/*.dll; do
		cp "$filename" $EXPORT_DIR
	done
fi

cd Exports
echo "Compressing into a zip archive . . ."
zip -r $1.zip $1_Win
echo "Done!"
