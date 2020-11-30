## FFmpeg命令行指令集

### 1 转码





### 2 针对单路视频流滤镜、操作

#### 模糊 avgblur

```
ffmpeg -i testvideo_1.mp4 -vf avgblur=sizeX=10:sizeY=10 result.mp4 
```



### 曲线调整 curves

```
ffmpeg -i testvideo_1.mp4 -vf curves=vintage result.mp4
```



#### 添加矩形框 drawbox

```
ffmpeg -i testvideo_1.mp4 -vf drawbox=x=10:y=10:w=100:h=100:color=pink@0.5:t=fill result.mp4
```





### 3 针对多路视频流操作

#### 