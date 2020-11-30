

```
ffmpeg -i testvideo_1.mp4 -vf avgblur=sizeX=10:sizeY=10 result.mp4 
```



```
ffmpeg -i testvideo_1.mp4 -vf curves=vintage result.mp4
```



```
ffmpeg -i testvideo_1.mp4 -vf drawbox=x=10:y=10:w=100:h=100:color=pink@0.5:t=fill result.mp4
```

