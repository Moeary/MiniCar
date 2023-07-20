小车控制c代码主要在 src\applications\sample\wifi-iot\app\car_mqtt-master\include 和 src\applications\sample\wifi-iot\app\car_mqtt-master\src 里面 <br>
主界面那里还有个Python文件用来语音转文字 文字再丢给gpt处理 转成命令发给mqtt客户端（mosquitto） mqtt发送命令给小车 小车就能跟随命令在动了<br>
需要安装FFmpeg，whisper，openai，mqtt等一一系列库，没有的根据错误提示自己pip install<br>
