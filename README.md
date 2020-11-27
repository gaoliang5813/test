# ffmpeg video processing interface

FilterVideo是FFmpeg filter类，filter不只包含滤镜功能，还包括裁剪、缩放、添加水印等功能，后续可以扩展更多自定义滤镜功能。

## Introduction of interface

- init_filter_drawtext

  把FilterVideo的实例对象初始化为添加字幕滤镜。

- update_filter_drawtext_text

  更改字幕的文字内容，字幕其它参数保持不变。

- update_filter_drawtext_x_y

  更改字幕的位置，字幕其它参数保持不变。

- init_filter_crop

  把FilterVideo的实例对象初始化为裁剪滤镜。

- init_filter_scale

  把FilterVideo的实例对象初始化为缩放滤镜。

- init_filter_add_picture

  把FilterVideo的实例对象初始化为添加图片滤镜。

- init_filter_xfade

  把FilterVideo的实例对象初始化为转场滤镜。

- filter_operate
  
  对一帧画面执行滤镜操作。


## How to use

1. 先实例化FilterVideo类的对象：

   ```
   FilterVideo filter_drawtext;
   FilterVideo filter_add_picture;
   FilterVideo filter_xfade;
   FilterVideo filter_scale;
   FilterVideo filter_crop;
   ```

2. 获取视频流video source的time_base等信息，存入char *args中。

3. 把FilterVideo对象初始化为相应的filter，如：

   ```
   filter_scale.init_filter_scale(args, 1280, 720)
   ```
   
   第一个参数为上一步提取的视频流args。

4. 从视频流中获取一帧一帧的frame，调用filter_operate函数：

   ```
   frame = filter_scale.filter_operate(frame)
   ```

   得到经过相应滤镜处理后的frame。