cd /D "%~dp0\textures"

for %%f in (*.png) do (
	Rem python ../../../../Rayne/Tools/TextureCompression/convert.py %%f ../../Resources/models/%%~nf.astc
	python ../../../../Rayne/Tools/TextureCompression/convert.py %%f ../../Resources/models/%%~nf.dds
)
