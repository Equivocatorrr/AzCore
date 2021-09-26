#Used to easily package the binaries and their libraries
echo "Packing everything needed to distribute on Linux . . ."
EXPORT_DIR=Exports/$1_Linux
if [ -e Exports ]
then
	rm -r $EXPORT_DIR*
fi

mkdir -p $EXPORT_DIR

if [ -e data ]
then
	cp -r data $EXPORT_DIR/data
fi

cp bin/Linux/Release/$1 $EXPORT_DIR/$1
cp bin/Linux/Debug/$1 $EXPORT_DIR/$1_Debug

if [ -e lib ]
then
	mkdir -p $EXPORT_DIR/lib
	for filename in lib/*.so*; do
		cp "$filename" $EXPORT_DIR/lib
	done
fi

cd Exports
echo "Compressing into a tar ball . . ."
tar -czvf $1.tar.gz $1_Linux
echo "Done!"
