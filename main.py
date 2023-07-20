import gradio as gr
import openai, subprocess, webbrowser
import paho.mqtt.client as mqtt

openai.api_key = "********************" #填入自己的openai api

messages = [{"role": "system", "content": '你是一个智能指令中转机器人,你会根据我的语句做出相应的命令回复,比如我说前进或者向前,你只需要回复forward给我就行了,然后你只需要回复“forward,left,right,back,stop,如果你听不懂我的话语就直接回复stop就行了”'}]


def publish_message(message):
    client = mqtt.Client()
    client.connect("localhost", 1883, 60)

    topic = "6414651136eaa44b9c900dd0_Hi3861_0_0_2023032014"
    client.publish(topic, message)

    client.disconnect()

def transcribe(audio):
    global messages

    audio_file = open(audio, "rb")
    transcript = openai.Audio.transcribe("whisper-1", audio_file)

    messages.append({"role": "user", "content": transcript["text"]})
    
   
        
    response = openai.ChatCompletion.create(model="gpt-3.5-turbo", messages=messages)

    system_message = response["choices"][0]["message"]
    
    if "forward" in system_message['content']:
        publish_message("forward")
    elif "back" in system_message['content']:
        publish_message("back")
    elif "left" in system_message['content']:
        publish_message("left")
    elif "right" in system_message['content']:
        publish_message("right")
    elif "stop" in system_message['content']:
        publish_message("stop")
    else:
        # 如果消息中不包含任何特定字符串，则不执行任何操作或显示相应的错误信息。
        pass
        
    messages.append(system_message)

    subprocess.call(["wsay", system_message['content']])

    chat_transcript = ""
    for message in messages:
        if message['role'] != 'system':
            chat_transcript += message['role'] + ": " + message['content'] + "\n\n"

    return chat_transcript

ui = gr.Interface(fn=transcribe, inputs=gr.Audio(source="microphone", type="filepath"), outputs="text").launch()
ui.launch()