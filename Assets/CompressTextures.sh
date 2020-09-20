cd $(dirname "$BASH_SOURCE")/textures

for f in *.png
do
	filename=$(basename -- "$f")
	filename="${filename%.*}"

	python ../../../../Rayne/Tools/TextureCompression/convert.py $f ../../Resources/models/$filename.astc
	python ../../../../Rayne/Tools/TextureCompression/convert.py $f ../../Resources/models/$filename.dds
done
