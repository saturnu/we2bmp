# we2bmp
we2bmp - a ngc winning eleven bitmap reinjector

this is a tool was created as part of a winning eleven 6 translation project for the nintendo game cube.

usage:

convert texture to bitmap:
we2bmp -i texture_file.bin -o bitmap.bmp

reinjection of the translated bitmap into the texture file:
we2bmp -s translated_bitmap.bmp -d texture_file.bin
now a patched_texture_file.bin is created, that you can reinsert into the game's txs-file.

you can use the 'afs explorer' and its included zlib manager for the ps2, to extract the txs files.
if you want to reinsert the patched txs files with the zlib manager of the afs explorer, there is a program called txsfix, which you have to use first.

-if the bitmap isn't usable, play with the tilesize argument
-double check with an hex editor if your edited bmp has the exact same header as before, many bitmap editors mangle with the header
-the color palette of the bitmap isn't right, but it isn't changed either, if you reinsert it. just use a palette you like while editing

it uses the gopt library for parsing command line arguments
https://www.purposeful.co.uk/gopt/
