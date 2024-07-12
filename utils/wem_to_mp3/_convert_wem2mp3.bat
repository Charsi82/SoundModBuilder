for %%f in (*.wem) do ww2ogg.exe %%f --pcb packed_codebooks_aoTuV_603.bin
for %%f in (*.ogg) do revorb.exe %%f
for %%f in (*.ogg) do "ffmpeg.exe" -i %%f -acodec libmp3lame -b:a 256k %%~nf.mp3
del *.ogg
del *.wem
REM pause