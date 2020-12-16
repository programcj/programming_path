## PS码流说明
分割 00 00 01 开头
以 0x000001BA 开始，以 0x000001B9 结束（一般不存在）

PS头：

![图片文字](https://img-blog.csdn.net/20170612185121706)
```
`pack_start_code`字段：起始码，占位32bit，标识PS包的开始，固定为0x000001BA；
`01`字段：占位2bit；
`SCR`字段：占位46bit，其中包含42bit的SCR值和4个bit的marker_bit值；其中SCR值由system_clock_reference_base和`system_clock_reference_extension`两部分组成；字节顺序依次是：
`system_clock_reference_base` [32..30]：占位3bit；
`marker_bit`：占位1bit；
`system_clock_reference_base` [29..15]：占位15bit；
`marker_bit`：占位1bit；
`system_clock_reference_base` [14..0]：占位15bit；
`marker_bit`：占位1bit；
`system_clock_reference_extension`：占位9bit；
`marker_bit`：占位1bit；
`program_mux_rate`字段：速率值字段，占位22bit，正整数，表示P-STD接收此字段所在包的PS流的速率；这个值以每秒50字节作为单位；禁止0值；
`Marker_bit`：标记字段，占位1bit，固定为’1’；
`Marker_bit`：标记字段，占位1bit，固定为’1’；
`Reserved`字段：保留字段，占位5bit；
`pack_stuffing_length`字段：长度字段，占位3bit；规定了此字段之后填充字段的长度；
`stuffing_byte`：填充字段，固定为0xFF；长度由前一字段确定；
```
- 整个长度为：sizeof(ps_head_t)+ ps_pack.pack_stuffing_length

SH头(System header): 以 0x000001BB 开头

![图片文字](https://img-blog.csdn.net/20170612185255307)
```
system_header_start_code 字段：系统头部起始码，占位32bit，值固定为0x000001BB，标志系统首部的开始；
header_length字段：头部长度字段，占位16bit，表示此字段之后的系统首部字节长度；
Marker_bit 字段：占位1bit，固定值为1；
rate_bound 字段：整数值，占位22bit，为一个大于或等于PS流所有PS包中的最大program_mux_rate值的整数；可以被解码器用来判断是否可以对整个流进行解码；
Marker_bit 字段：占位1bit，固定值为1；
audio_bound 字段：占位6bit；取值范围0到32间整数；大于或等于同时进行解码处理的PS流中的音频流的最大数目；
fixed_flag 字段：标志位，占位1bit；置位1表示固定比特率操作，置位0则为可变比特率操作；
CSPS_flag 字段：CSPS标志位，占位1bit；置位1表示此PS流满足标准的限制；
system_audio_lock_flag 字段：标志位，占位1bit，表示音频采样率和STD的system_clock_frequency之间有一特定常数比例关系；
system_video_lock_flag 字段：标志位，占位1bit，表示在系统目标解码器system_clock_frequency和视频帧速率之间存在一特定常数比例关系；
Marker_bit 字段：占位1bit，固定值为1；
video_bound 字段：整数，占位5bit，取值范围0到16；大于或等于同时进行解码处理的PS流中的视频流的最大数目；
packet_rate_restriction_flag 字段：分组速率限制标志字段，占位1bit，若CSPS_flag == 1，则此字段表示哪种限制适用于分组速率；若CSPS_flag == 0，则此字段无意义；
reserved_bits 字段：保留字段，占位7bit，固定为’1111111’；
```
- 整个长度 4+2+sh_pack.header_length

![PSM帧](https://img-blog.csdn.net/20170612190035859)
PES帧(PSM)(PS payload 部分):
stream_id == 0xBC 时，说明此PES包是一个PSM，也就是 0x000001BC 开头

`stream_type`：类型字段，占位8bit；表示原始流ES的类型；这个类型只能标志包含在PES包中的ES流类型；值0x05是被禁止的；常见取值类型有MPEG-4 视频流：0x10；H.264 视频流：0x1B；G.711 音频流：0x90；因为PSM只有在关键帧打包的时候，才会存在，所以如果要判断PS打包的流编码类型，就根据这个字段来判断；

`elementary_stream_id`：流ID字段，占位8bit；表示此ES流所在PES分组包头中的stream_id字段的值；其中0x(C0~DF)指音频，0x(E0~EF)为视频；

- PSM头长度： 4 + 2 + psm_pack.program_stream_map_length ，有些网络需要网络字节序转换 ntohs(length), 如：x86环境下

接下来都是PES帧：

stream_id==0xE0 为视频帧：即0x000001E0开头

