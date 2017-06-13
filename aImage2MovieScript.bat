ffmpeg -start_number 100000 -framerate 25 -i Fugue%%06d.png  -i Fugue.wav -c:v libx264 -c:a aac -b:a 192k -pix_fmt yuv420p -strict -2  out.mp4
pause